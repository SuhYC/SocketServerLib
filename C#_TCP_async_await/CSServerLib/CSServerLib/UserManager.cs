using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CSServerLib
{
    internal class UserManager
    {
        private Queue<User> m_UserPool = new Queue<User>();
        private Dictionary<int, User> m_Users = new Dictionary<int, User>();

        const int MAX_USER_COUNT = 1024;

        public UserManager()
        {
            try
            {
                for(uint i = 0; i < MAX_USER_COUNT; i++)
                {
                    User user = new User();
                    m_UserPool.Enqueue(user);
                }
            }
            catch(OutOfMemoryException)
            {
                Console.WriteLine($"UserManager::Constructor : Failed to Allocate User Object.");
            }
        }

        public void ReleaseUser(int nUserIndex_)
        {
            bool bRet = false;
            User? user = null;
            lock (m_Users)
            {
                if (bRet = m_Users.TryGetValue(nUserIndex_, out user))
                {
                    m_Users.Remove(nUserIndex_);
                }
            }

            if(bRet && user != null)
            {
                lock (m_UserPool)
                {
                    m_UserPool.Enqueue(user);
                }
            }
        }

        public bool AddUser(int nUserIndex_)
        {
            User? user = null;

            lock (m_UserPool)
            {
                if (m_UserPool.Count == 0)
                {
                    return false;
                }
                user = m_UserPool.Dequeue();
            }

            if (user == null)
            {
                return false;
            }

            bool bRet = false;
            lock(m_Users)
            {
                bRet = m_Users.TryAdd(nUserIndex_, user);
            }

            if (!bRet)
            {
                lock(m_UserPool)
                {
                    m_UserPool.Enqueue(user);
                }
            }

            return bRet;
        }
    }
}
