using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CSServerLib
{
    internal class ZoneManager
    {
        //private Dictionary<int, Zone> m_Zones = new Dictionary<int, Zone>();
        private ConcurrentDictionary<int, Zone> m_Zones = new ConcurrentDictionary<int, Zone>();


        public Zone? GetZone(int nZoneIndex_)
        {
            try
            {
                Zone zone = m_Zones.GetOrAdd(nZoneIndex_, _ => new Zone());

                return zone;
            }
            catch(OutOfMemoryException)
            {
                Console.WriteLine($"ZoneManager::GetZone : Failed to Allocate New Zone.");
                return null;
            }
            catch (Exception e)
            {
                Console.WriteLine($"ZoneManager::GetZone : Exception : {e.Message}");
                return null;
            }
        }
    }
}
