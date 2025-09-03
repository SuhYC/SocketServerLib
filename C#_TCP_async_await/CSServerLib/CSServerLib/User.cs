using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CSServerLib
{
    internal class User
    {
        private uint m_ZoneIndex;
        private uint m_Index;

        public void Init(uint uIndex_)
        {
            m_Index = uIndex_;
        }

        public void Clear()
        {

        }

        public void SetZoneIndex(uint uZoneIndex_)
        {
            m_ZoneIndex = uZoneIndex_;
        }

        public uint GetZoneIndex() { return m_ZoneIndex; }
        public uint GetIndex() { return m_Index; }
    }
}
