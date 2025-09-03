using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices.Marshalling;
using System.Runtime.CompilerServices;
using System.Collections.Concurrent;

namespace CSServerLib
{
     abstract internal class CSSocketServer
    {
        const uint SOCK_BUF = 1024;
        const uint MAX_CLIENT_COUNT = 1024;

        private bool m_IsRun = false;

        private ConcurrentQueue<int> m_ClientIndexQueue = new ConcurrentQueue<int>();

        private Socket? m_ListenSocket;

        private ConcurrentDictionary<int, Task> m_RecvTasks = new ConcurrentDictionary<int, Task>();
        private ConcurrentDictionary<int, ClientContext> m_Clients = new ConcurrentDictionary<int, ClientContext>();

        private Task? AcceptTask;

        protected bool Start(int port)
        {
            CreateIndexPool();

            bool bRet = Init(port);
            
            if (!bRet)
            {
                Console.WriteLine($"CSSocketServer::Start : Failed to Init");
                return false;
            }

            Console.WriteLine($"Server Started.");

            return true; 
        }

        protected async Task Close() 
        {
            m_IsRun = false;

            m_ListenSocket?.Close();
            m_ListenSocket?.Dispose();

            if (AcceptTask != null)
            {
                await AcceptTask;
            }

            await Task.WhenAll(m_RecvTasks.Values.ToArray());

            return;
        }

        protected async Task<bool> SendMsg(int nClientIndex_, byte[] msg_, uint size_)
        {
            ClientContext? cc;
            if(!m_Clients.TryGetValue(nClientIndex_, out cc))
            {
                return false;
            }

            if(cc == null)
            {
                return false;
            }

            return await cc.SendMsg(msg_, size_);
        }

        private bool Init(int port)
        {
            try
            {
                m_ListenSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            }
            catch (OutOfMemoryException)
            {
                Console.WriteLine($"CSSocketServer::Init : Failed to Allocate ListenSocket.");
                return false;
            }

            try
            {
                m_ListenSocket.Bind(new IPEndPoint(IPAddress.Any, port));
            }
            catch (SocketException ex)
            {
                Console.WriteLine($"CSSocketServer::Init : Failed to Bind. {ex.ErrorCode}");
                return false;
            }
            
            m_ListenSocket.Listen(5);

            m_IsRun = true;

            AcceptTask = AcceptLoop();

            return true;
        }

        private async Task AcceptLoop()
        {
            if(m_ListenSocket == null)
            {
                Console.WriteLine($"CSSocketServer::AcceptLoop : ListenSocket Not Allocated.");
                return;
            }

            while (m_IsRun)
            {
                try
                {
                    if(!m_ClientIndexQueue.TryDequeue(out int idx))
                    {
                        await Task.Delay(20);
                        continue;
                    }

                    Socket client = await m_ListenSocket.AcceptAsync();

                    ClientContext cc = new ClientContext(idx, client);

                    Task task = RecvLoop(cc);

                    m_Clients.TryAdd(cc.m_Index, cc);
                    m_RecvTasks.TryAdd(cc.m_Index, task);

                    _ = task.ContinueWith(t =>
                    {
                        m_RecvTasks.Remove(cc.m_Index, out _);
                        m_Clients.Remove(cc.m_Index, out _);

                        m_ClientIndexQueue.Enqueue(cc.m_Index);
                    });
                }
                catch(SocketException ex)
                {
                    Console.WriteLine($"CSSocketServer::AcceptLoop : SocketErr. {ex.ErrorCode}");
                }
                catch(ObjectDisposedException)
                {
                    Console.WriteLine($"CSSocketServer::AcceptLoop : ListenSocket Discarded.");
                    return;
                }
                catch(OverflowException)
                {
                    Console.WriteLine($"CSSocketServer::AcceptLoop : Container Count Overflowed.");

                }
            }
        }

        private async Task RecvLoop(ClientContext cc)
        {
            ArraySegment<byte> view = new ArraySegment<byte>();

            await OnConnect(cc.m_Index);

            try
            {
                while (m_IsRun)
                {
                    if(!await cc.RecvAsync())
                    {
                        break;
                    }

                    while(cc.GetMsg(ref view))
                    {
                        await OnReceive(cc.m_Index, view);

                        await cc.PopMsg(view);
                    }
                }
            }
            catch(SocketException ex)
            {
                Console.WriteLine($"CSSocketServer::RecvLoop : Socket Err. {ex.SocketErrorCode}");
            }
            catch(ObjectDisposedException)
            {
                Console.WriteLine($"CSSocketServer::RecvLoop : Socket Discarded.");
            }
            finally
            {
                await OnDisconnect(cc.m_Index);
                cc.Release();
            }
        }

        private void CreateIndexPool()
        {
            m_ClientIndexQueue.Clear();

            for(int i = 0; i < MAX_CLIENT_COUNT; i++)
            {
                m_ClientIndexQueue.Enqueue(i);
            }

            return;
        }

        protected abstract Task OnReceive(int nUserIndex_, ArraySegment<byte> msg_);
        protected abstract Task OnConnect(int nUserIndex_);
        protected abstract Task OnDisconnect(int nUserIndex_);
    }
}
