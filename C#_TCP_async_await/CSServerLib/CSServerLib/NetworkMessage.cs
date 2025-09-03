using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CSServerLib
{
    struct ReqHeader
    {
        public uint MsgSize;
        public int ReqType;
        public uint ReqNo;
    }

    struct InfoHeader
    {
        public InfoHeader(uint msgSize_, int resCode_, uint reqNo_)
        {
            MsgSize = msgSize_;
            ResCode = resCode_;
            ReqNo = reqNo_;
        }

        public uint MsgSize;
        public int ResCode;
        public uint ReqNo;
    }

    unsafe struct EchoParameter
    {
        public uint PayloadSize;
        public fixed byte Payload[80];
    }


}
