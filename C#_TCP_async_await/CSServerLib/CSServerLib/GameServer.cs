using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace CSServerLib
{
    internal class GameServer : CSSocketServer
    {
        private ReqHandler m_ReqHandler;

        private UserManager m_UserManager;
        private ZoneManager m_ZoneManager;

        public struct DIStruct
        {
            public DIStruct(UserManager um_, ZoneManager zm_, Func<int, byte[], uint, Task<bool>> sendFunc_)
            {
                um = um_;
                zm = zm_;
                SendFunc = sendFunc_;
            }

            public UserManager um;
            public ZoneManager zm;
            public Func<int, byte[], uint, Task<bool>> SendFunc;
        }

        private DIStruct DIs;

        public GameServer() 
        {
            m_ReqHandler = new ReqHandler();

            m_UserManager = new UserManager();
            m_ZoneManager = new ZoneManager();

            DIs = new DIStruct(m_UserManager, m_ZoneManager, SendMsg);
        }

        public bool Run(int port)
        {
            return base.Start(port);
        }

        public async Task End()
        {
            await base.Close();

            return;
        }

        protected override async Task OnConnect(int nUserIndex_) 
        {
            Console.WriteLine($"Client[{nUserIndex_}] Connected.");

            await Task.CompletedTask;
        }

        protected override async Task OnDisconnect(int nUserIndex_)
        {
            Console.WriteLine($"Client[{nUserIndex_}] Disconnected.");

            await Task.CompletedTask;
        }

        protected override async Task OnReceive(int nUserIndex_, ArraySegment<byte> msg_)
        {
            await m_ReqHandler.HandleReq(nUserIndex_, DIs, msg_);
        }
    }
}
