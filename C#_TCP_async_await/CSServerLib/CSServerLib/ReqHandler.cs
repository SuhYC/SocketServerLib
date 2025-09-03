using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace CSServerLib
{
    internal class ReqHandler
    {
        private List<Func<int, GameServer.DIStruct, ArraySegment<byte>, Task<ResultCode>>> ReqHandleFuncs;

        public enum ReqType
        {
            EchoJob,
            Last
        }


        public enum ResultCode
        {
            Success,
            Fail,
            NotFinished,
            Rejected
        }

        public ReqHandler()
        {
            int size = (int)ReqType.Last;
            ReqHandleFuncs = new List<Func<int, GameServer.DIStruct, ArraySegment<byte>, Task<ResultCode>>>(Enumerable.Repeat<Func<int, GameServer.DIStruct, ArraySegment<byte>, Task<ResultCode>>>(null!, size));

            ReqHandleFuncs[(int)ReqType.EchoJob] = HandleEchoJob;

        }

        private static unsafe T BytesToStruct<T>(ReadOnlySpan<byte> data_) where T : unmanaged
        {
            if (data_.Length < sizeof(T))
            {
                throw new ArgumentException($"Buffer too small for {typeof(T)}");
            }

            fixed (byte* p = data_)
            {
                return *(T*)p;
            }
        }

        private static unsafe bool StructToBytes<T>(ref T data_, ref byte[] buffer_) where T : unmanaged
        {
            if(buffer_.Length < sizeof(T))
            {
                Console.WriteLine($"ReqHandler::StructToByte : Buffer Too Small for {typeof(T)}");
                return false;
            }

            fixed(byte* p = buffer_)
            {
                *(T*)p = data_;
            }

            return true;
        }

        public async Task<ResultCode> HandleReq(int nUserIndex_, GameServer.DIStruct DIs_, ArraySegment<byte> msg_)
        {
            int HeaderSize = Marshal.SizeOf<ReqHeader>();

            if(msg_.Count < HeaderSize || msg_.Array == null)
            {
                // 헤더보다도 짧은 메시지
                return ResultCode.Rejected;
            }

            ReqHeader req = BytesToStruct<ReqHeader>(msg_);

            if(req.ReqType >= (int)ReqType.Last || req.ReqType < 0)
            {
                // 잘못된 요청타입
                return ResultCode.Rejected;
            }

            ArraySegment<byte> data = new ArraySegment<byte>(msg_.Array!, msg_.Offset + HeaderSize, msg_.Count - HeaderSize);

            var func = ReqHandleFuncs[req.ReqType];

            if(func == null)
            {
                Console.WriteLine($"ReqHandler::HandleReq : HandleFunc Not Allocated. ReqType : {req.ReqType}");
                return ResultCode.Fail;
            }

            while(true)
            {
                ResultCode resCode = await func(nUserIndex_, DIs_, data);

                if(resCode == ResultCode.NotFinished)
                {
                    // L7버퍼가 가득차서 처리가 안되는 것이므로 잠시 후 재시도
                    await Task.Delay(20);
                    continue;
                }

                return resCode;
            }
        }

        private async Task<ResultCode> HandleEchoJob(int nUserIndex_, GameServer.DIStruct DIs, ArraySegment<byte> msg_)
        {
            EchoParameter param;
            try
            {
                param = BytesToStruct<EchoParameter>(msg_);
            }
            catch(ArgumentException)
            {
                Console.WriteLine($"ReqHandler::HandleEchoJob : Not Enough Len on binary.");
                return ResultCode.Rejected;
            }

            byte[] buffer = MakeMsg(ResultCode.Success, 0, param);

            bool bRet = await DIs.SendFunc(nUserIndex_, buffer, (uint)buffer.Length);

            if (bRet)
            {
                return ResultCode.Success;
            }
            else
            {
                return ResultCode.NotFinished;
            }
        }

        private unsafe byte[] MakeMsg<T>(ResultCode res_, uint ReqNo_, T data) where T : unmanaged
        {
            int dataSize = Marshal.SizeOf<T>();
            int headerSize = Marshal.SizeOf<InfoHeader>();

            byte[] buffer = new byte[dataSize + headerSize];

            InfoHeader header = new InfoHeader((uint)dataSize + (uint)headerSize, (int)res_, ReqNo_);

            fixed(byte* p = buffer)
            {
                *(InfoHeader*)p = header;

                byte* dest = p + headerSize;

                *(T*)dest = data;
            }

            return buffer;
        }
    }
}
