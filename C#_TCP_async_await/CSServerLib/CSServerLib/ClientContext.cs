using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net.Sockets;

namespace CSServerLib
{
    internal class ClientContext
    {
        const uint MAX_SEGMENT_SIZE = 1024;
        const uint SEND_BUFFER_SIZE = MAX_SEGMENT_SIZE * 4;
        const uint RECV_BUFFER_SIZE = SEND_BUFFER_SIZE * 4;

        public ClientContext(int idx_, Socket socket)
        {
            m_Index = idx_;
            m_Socket = socket;

            m_buffer = new byte[MAX_SEGMENT_SIZE];

            m_SendBuffer = new SlideBuffer(SEND_BUFFER_SIZE);
            m_RecvBuffer = new SlideBuffer(RECV_BUFFER_SIZE);
        }

        public int m_Index;
        public Socket? m_Socket;
        private SlideBuffer m_SendBuffer;

        // 수신한 메시지를 일단 담고, 이후 메시지 단위로 가져갈 수 있게.
        private SlideBuffer m_RecvBuffer;

        // ReceiveAsync를 직접 걸 때 쓸 버퍼 (재사용 가능)
        private byte[] m_buffer;

        private readonly AsyncLock m_SendLock = new AsyncLock();

        public async Task<bool> SendMsg(byte[] msg, uint size_)
        {
            if (m_Socket == null)
            {
                Console.WriteLine($"ClientContext[{m_Index}]::SendMsg : Socket Not Allocated.");
                return false;
            }

            bool bRet = await m_SendBuffer.Enqueue(msg, size_);

            if (!bRet)
            {
                return false;
            }

            var handle = m_SendLock.TryLockAsync();
            if (handle == null)
            {
                // other thread already sending. but Enqueued Data Successfully.
                return true;
            }

            using (handle)
            {
                while (true)
                {
                    uint msgSize = m_SendBuffer.GetSize();

                    if (msgSize == 0)
                    {
                        // No More Msg.
                        break;
                    }

                    if (msgSize > MAX_SEGMENT_SIZE)
                    {
                        msgSize = MAX_SEGMENT_SIZE;
                    }

                    var segment = new ArraySegment<byte>(m_SendBuffer.GetBuf(), 0, (int)msgSize);

                    try
                    {
                        int sent = await m_Socket.SendAsync(segment);

                        if (sent > 0)
                        {
                            bRet = await m_SendBuffer.Pop((uint)sent);

                            if (!bRet)
                            {
                                Console.WriteLine($"ClientContext[{m_Index}]::SengMsg : buffer pop failed.");
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        if (!HandleSendException(ex))
                        {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// 수신하여 수신 슬라이드버퍼에 담고 리턴.
        /// 
        /// 예외가 발생하거나 연결이 종료되면 false를 반환한다.
        /// </summary>
        /// <returns></returns>
        public async Task<bool> RecvAsync()
        {
            if(m_Socket == null)
            {
                return false;
            }

            try
            {
                int recved = await m_Socket.ReceiveAsync(m_buffer);

                if (recved == 0)
                {
                    // Graceful Closure
                    return false;
                }

                await m_RecvBuffer.Enqueue(m_buffer, (uint)recved);

                return true;
            }
            catch (Exception ex)
            {
                HandleRecvException(ex);
            }

            return false;
        }

        public bool GetMsg(ref ArraySegment<byte> msg)
        {
            return m_RecvBuffer.GetMsg(ref msg);
        }

        public async Task<bool> PopMsg(ArraySegment<byte> msg)
        {
            return await m_RecvBuffer.Pop((uint)msg.Count());
        }

        private bool HandleSendException(Exception ex)
        {
            switch (ex)
            {
                case SocketException s:
                    Console.WriteLine($"ClientContext::RecvAsync : Socket Err : {s.SocketErrorCode}");
                    return true; // 일단은 연결 유지
                case ArgumentException:
                    Console.WriteLine($"ClientContext::SendMsg : ArraySegment null ref.");
                    return false;
                case ObjectDisposedException:
                    Console.WriteLine($"ClientContext::SendMsg : Socket Closed.");
                    return false;
                default:
                    Console.WriteLine($"ClientContext::SendMsg : Unexpected Err : {ex.Message}");
                    return false;
            }
        }

        private void HandleRecvException(Exception ex)
        {
            switch (ex)
            {
                case SocketException s:
                    Console.WriteLine($"ClientContext::RecvAsync : Socket Err : {s.SocketErrorCode}");
                    return;
                case ObjectDisposedException:
                    Console.WriteLine($"ClientContext::RecvAsync : Socket Closed");
                    return;
                default:
                    Console.WriteLine($"ClientContext::RecvAsync : {ex.Message}");
                    return;
            }
        }

        public void Release()
        {
            m_Socket?.Close();
            m_Socket?.Dispose();
            m_Socket = null;

            return;
        }
    }
}
