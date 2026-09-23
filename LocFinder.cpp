#include <plugin.h> // Plugin-SDK version 1005 from 2026-08-14 08:10:00
#include <CMessages.h>
#include <unordered_set>
#include <fstream>

using namespace plugin;

std::unordered_set<std::string> loggedDoors;

struct Main
{
    Main()
    {
        // register event callbacks
        Events::gameProcessEvent += []{ gInstance.OnGameProcess(); };
    }

    std::unordered_set<int> interiorDoors = {
    1491, 1492, 1494, 1495, 1497, 1498, 1499, 1500, 1501, 1502,
    1504, 1505, 1506, 1507, 1523, 1532, 1533, 1535, 1536, 1537,
    1538, 1555, 1556, 1557, 1560, 1561, 1566, 1567, 1569
    };

    int locations = 0;

    void OnGameProcess()
    {
        CPlayerPed* player = FindPlayerPed();
        if (!player) return;

        if (!player->m_nAreaCode == 0) return;

        static char msg[255];

        CMessages::AddMessageJumpQ(msg, 5000, 0);

        std::ofstream outFile(PLUGIN_PATH("locations.txt"), std::ios::app);

        CMessages::AddMessageJumpQ(msg, 500, 0);
        for (CDummy* object : CPools::ms_pDummyPool) {
            if (interiorDoors.count(object->m_nModelIndex) && object->m_nAreaCode == 0) {
                static char line[1024];
                CVector doorCenter = object->GetPosition() + (object->GetMatrix().GetRight() * 1.0f);
                CVector destination = (doorCenter + (object->GetMatrix().GetForward() * 2.5));
                char uniqueId[128];
                sprintf_s(uniqueId, "%d_%d_%d_%d", (int)destination.x, (int)destination.y, (int)destination.z, object->m_nModelIndex);
                if (loggedDoors.find(uniqueId) == loggedDoors.end()) {
                    loggedDoors.insert(uniqueId);
                    sprintf_s(line, "%f %f %f 20.0 %d 30000 100.0 2.5 60000 0 -1 -1 1 15.0 0 40.0 0", destination.x, destination.y, destination.z, object->m_nModelIndex);
                    if (outFile.is_open()) {
                        outFile << line << "\n";
                        locations++;
                        static char msg[255];
                        sprintf_s(msg, "Added location ID: %d AT: %f %f %f", locations, destination.x, destination.y, destination.z);

                        CMessages::AddMessageJumpQ(msg, 500, 0);
                    }
                }
            }
        }
        sprintf_s(msg, "Found %d objects", locations);

        CMessages::AddMessageJumpQ(msg, 50000, 0);
        outFile.close();
    }
} gInstance;
