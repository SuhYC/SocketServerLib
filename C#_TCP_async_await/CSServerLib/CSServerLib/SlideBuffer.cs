using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CSServerLib
{
    internal class SlideBuffer
    {
        public SlideBuffer(uint cap_)
        {
            if(cap_ == 0)
            {
                Console.WriteLine($"SlideBuffer::Constructor : Zero Size?");
            }

            buffer = new byte[cap_];
            cap = cap_;
            size = 0;
            m_Lock = new AsyncLock();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="msg_">삽입할 메시지</param>
        /// <param name="size_">메시지의 크기</param>
        /// <returns></returns>
        public async Task<bool> Enqueue(byte[] msg_, uint size_)
        {
            if (size_ < 1)
            {
                // Zero Push?
                return true;
            }

            if (msg_.Length < size_)
            {
                Console.WriteLine($"SlideBuffer::Enqueue : Index Error.");
                return false;
            }

            using(await m_Lock.LockAsync())
            {
                if (cap - size < size_)
                {
                    // Buffer Has Not Enough Space. (Not Critical.)
                    return false;
                }

                Array.Copy(msg_, 0, buffer, size, size_);
                size += size_;
            }

            return true;
        }


        /// <summary>
        /// 
        /// </summary>
        /// <param name="buf_">메시지를 가져가 담을 버퍼</param>
        /// <param name="size_">가져갈 메시지의 크기</param>
        /// <returns></returns>
        public async Task<bool> Dequeue(byte[] buf_, uint size_)
        {
            if (size_ < 1)
            {
                return true;
            }

            if (buf_.Length < size_)
            {
                Console.WriteLine($"SlideBuffer::Dequeue : Index Error.");
                return false;
            }

            using (await m_Lock.LockAsync())
            {
                if (size < size_)
                {
                    Console.WriteLine($"SlideBuffer::Dequeue : Not Enough Msg.");
                    return false;
                }

                Array.Copy(buffer, 0, buf_, 0, size_);
                Array.Copy(buffer, size_, buffer, 0, size - size_);
                size -= size_;
            }

            return true;
        }

        public async Task<bool> Pop(uint size_)
        {
            if (size_ < 1)
            {
                return true;
            }

            using (await m_Lock.LockAsync())
            {
                if (size < size_)
                {
                    Console.WriteLine($"SlideBuffer::Dequeue : Not Enough Msg.");
                    return false;
                }

                Array.Copy(buffer, size_, buffer, 0, size - size_);
                size -= size_;
            }

            return true;
        }

        /// <summary>
        /// 버퍼의 상위 4바이트를 uint타입으로 변환하여 반환
        /// </summary>
        /// <returns></returns>
        public uint Peek()
        {
            if(size < 4)
            {
                return 0;
            }

            uint value = BitConverter.ToUInt32(buffer, 0);
            return value;
        }

        public bool GetMsg(ref ArraySegment<byte> buf)
        {
            uint header = Peek();

            if(header == 0)
            {
                return false;
            }

            if(header > GetSize())
            {
                return false;
            }

            buf = new ArraySegment<byte>(buffer, 0, (int)header);

            return true;
        }

        public bool Empty()
        {
            return size == 0;
        }

        public uint GetSize()
        {
            return size;
        }

        public byte[] GetBuf() { return buffer; }

        private byte[] buffer;
        private uint cap;
        private uint size;
        private AsyncLock m_Lock;
    }
}
