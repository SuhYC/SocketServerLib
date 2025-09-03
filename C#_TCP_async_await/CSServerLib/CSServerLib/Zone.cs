using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CSServerLib
{
    internal class Zone
    {
        private HashSet<int> m_Users = new HashSet<int>(); // 일단 작성. 나중에 너무 무거우면 인원제한이나 컨테이너 교체 고려.

        public bool Enter(int nUserIndex_)
        {
            lock(m_Users)
            {
                return m_Users.Add(nUserIndex_);
            }
        }

        public bool Exit(int nUserIndex_)
        {
            lock(m_Users)
            {
                return m_Users.Remove(nUserIndex_);
            }
        }

        public IEnumerable<int> GetAllUser()
        {
            return m_Users;
        }
    }
}
