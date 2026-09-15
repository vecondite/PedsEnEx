#include <plugin.h> // Plugin-SDK version 1005 from 2026-08-14 08:10:00
#include <ranges>
#include <CMessages.h>
#include <CClock.h>
#include <CStreaming.h>
#include <CCivilianPed.h>
#include <CWorld.h>
#include <CCheat.h>
#include <extensions/ScriptCommands.h>

using namespace plugin;

std::string trim(const std::string& str) {
    int beginning = str.find_first_not_of(" \t\r\n");
    if (beginning == std::string::npos) return "";
    int ending = str.find_last_not_of(" \t\r\n");
    return str.substr(beginning, (ending - beginning + 1));
}

std::vector<std::string> split(std::string text, char delimiter) {
    std::vector<std::string> output;
    std::string chunk;
    std::stringstream ss(text);

    if (std::isspace(static_cast<unsigned char>(delimiter))) {
        while (ss >> chunk) {
            std::string trimmedChunk = trim(chunk);
            if (!trimmedChunk.empty()) {
                output.push_back(trimmedChunk);
            }
        }
    } else {
        while (std::getline(ss, chunk, delimiter)) {
            std::string trimmedChunk = trim(chunk);
            if (!trimmedChunk.empty()) {
                output.push_back(trimmedChunk);
            }
        }
    }

    return output;
}

struct TimeRange {
    unsigned int lowEnd;
    unsigned int highEnd;
};

struct Location {
    float x;
    float y;
    float z;
    float radiusSqr;
    unsigned int model;
    unsigned int interval;
    unsigned int nextRing;
    float probability;
    float walkDist;
    int walkTime;
    bool invertWalk;
    int pedsId;
    int timeRangeId;
    int cops;
    float copProb;
    int copsOnly;
    float leaveProb;
    bool lfb;
    std::vector<unsigned int> pedModels;
    std::vector<TimeRange> timeRanges;
    std::vector<unsigned int> copModels;
};

std::vector<Location> loadedLocations;
std::unordered_map<int, std::vector<unsigned int>> locationPeds;
std::unordered_map<int, std::vector<TimeRange>> locationTimeRanges;
std::unordered_map<int, std::vector<unsigned int>> locationCopPeds;
std::vector<eWeaponType> copWeapons = {WEAPONTYPE_PISTOL, WEAPONTYPE_NIGHTSTICK};
std::vector<int> spawnedPeds;
std::vector<unsigned int> lastPedModels;

struct Main
{
    CPed* createPed(int pedModelId, CVector locationCoords, CObject* closest, ePedType pedType) {
        static char msg[255];
        if (!CStreaming::HasModelLoaded(pedModelId)) CStreaming::RequestModel(pedModelId, 0);
        CStreaming::LoadAllRequestedModels(false);
        if (!CStreaming::HasModelLoaded(pedModelId)) {
            sprintf_s(msg, "~r~~h~Failed to load ped model: %d", pedModelId);
            CMessages::AddMessageJumpQ(msg, 10000, 0);
            return nullptr;
            //break;
        }
        auto ped = new CCivilianPed(pedType, pedModelId);
        if (!ped)
        {
            sprintf_s(msg, "~r~~h~Failed to spawn ped model: %d", pedModelId);
            CMessages::AddMessageJumpQ(msg, 10000, 0);
            CStreaming::SetModelIsDeletable(pedModelId);
            return nullptr;
            //break;
        }
        //sprintf_s(msg, "Created Ped of model: %d", pedModelId);
        CMessages::AddMessageJumpQ(msg, 1000, 0);
        ped->SetPosition(locationCoords);
        float pedHeading = closest->GetHeading() + DegToRad(180.0f);
        ped->SetHeading(pedHeading);
        ped->m_fHeadingCurrent = ped->m_fHeadingGoal = pedHeading;
        ped->GetMatrix().UpdateRW();

        if (ped->m_pRwClump) {
            RpAnimBlendClumpUpdateAnimations(ped->m_pRwClump, CTimer::ms_fTimeStep);
        }

        #if defined(GTASA)
                ped->m_fContactSurfaceBrightness = 0.5f;
        #endif

        CWorld::Add(ped);

        return ped;
    }
    void loadLocations() {
        lastPedModels.clear();

        std::ifstream locations;
        std::ifstream pedModels;
        std::ifstream timeRanges;
        std::ifstream copPeds;

        loadedLocations.clear();
        locationPeds.clear();
        locationTimeRanges.clear();
        locationCopPeds.clear();

        locations.open(PLUGIN_PATH("locations.txt"));
        pedModels.open(PLUGIN_PATH("pedModels.txt"));
        timeRanges.open(PLUGIN_PATH("timeRanges.txt"));
        copPeds.open(PLUGIN_PATH("copModels.txt"));

        if (pedModels.is_open()) {
            std::string line;

            while (std::getline(pedModels, line)) {
                if (line.empty() || line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/')) continue;
                std::vector<std::string> values = split(line, '=');
                if (values.size() != 2) continue;
                std::vector<std::string> rawModelsFound = split(values[1], ',');
                std::vector<unsigned int> modelsFound;
                modelsFound.reserve(rawModelsFound.size());
                for (std::string rawModel : rawModelsFound) {
                    modelsFound.push_back(std::stoi(rawModel));
                }
                locationPeds[std::stoi(values[0])] = modelsFound;
            }
        }

        pedModels.close();

        if (timeRanges.is_open()) {
            std::string line;
            while (std::getline(timeRanges, line)) {
                if (line.empty() || line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/')) continue;
                std::vector<std::string> values = split(line, '=');
                if (values.size() != 2) continue;
                std::vector<std::string> times = split(values[1], ',');
                std::vector<TimeRange> parsedRanges;
                for (std::string time : times) {
                    std::vector<std::string> timeLimits = split(time, '-');
                    std::vector<std::string> lowLimit = split(timeLimits[0], ':');
                    std::vector<std::string> highLimit = split(timeLimits[1], ':');

                    int lowLimitMins = std::stoi(lowLimit[0]) * 60 + std::stoi(lowLimit[1]);
                    int highLimitMins = std::stoi(highLimit[0]) * 60 + std::stoi(highLimit[1]);

                    TimeRange range;
                    range.lowEnd = lowLimitMins;
                    range.highEnd = highLimitMins;

                    parsedRanges.push_back(range);
                }
                locationTimeRanges[std::stoi(values[0])] = parsedRanges;
            }
        }

        timeRanges.close();

        if (copPeds.is_open()) {
            std::string line;

            while (std::getline(copPeds, line)) {
                if (line.empty() || line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/')) continue;
                std::vector<std::string> values = split(line, '=');
                if (values.size() != 2) continue;
                std::vector<std::string> rawModelsFound = split(values[1], ',');
                std::vector<unsigned int> modelsFound;
                modelsFound.reserve(rawModelsFound.size());
                for (std::string rawModel : rawModelsFound) {
                    modelsFound.push_back(std::stoi(rawModel));
                }
                locationCopPeds[std::stoi(values[0])] = modelsFound;
            }
        }

        copPeds.close();

        if (locations.is_open()) {
            std::string line;
            while (std::getline(locations, line)) {
                if (line.empty() || line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/')) continue;
                std::vector<std::string> values = split(line, ' ');

                if (values.size() != 17) continue;

                Location loaded;
                loaded.x = std::stof(values[0]);
                loaded.y = std::stof(values[1]);
                loaded.z = std::stof(values[2]);
                loaded.radiusSqr = std::stof(values[3]) * std::stof(values[3]);
                loaded.model = std::stoi(values[4]);
                loaded.interval = std::stoi(values[5]);
                loaded.nextRing = CTimer::m_snTimeInMilliseconds + loaded.interval;
                loaded.probability = stof(values[6]);
                loaded.walkDist = stof(values[7]);
                loaded.walkTime = stoi(values[8]);
                loaded.invertWalk = stoi(values[9]);
                loaded.pedsId = stoi(values[10]);
                loaded.timeRangeId = stoi(values[11]);
                loaded.cops = stoi(values[12]);
                loaded.copProb = stof(values[13]);
                loaded.copsOnly = stoi(values[14]);
                loaded.leaveProb = stof(values[15]);
                loaded.lfb = stoi(values[16]);

                if (locationPeds.count(loaded.pedsId) > 0) {
                    loaded.pedModels = locationPeds[loaded.pedsId];
                }
                else {
                    if (loaded.copsOnly) {
                        std::vector<unsigned int> emptyPeds;
                        emptyPeds.push_back(0);
                        loaded.pedModels = emptyPeds;
                    } else {
                        loaded.pedModels = locationPeds[-1];
                    }
                }

                if (locationTimeRanges.count(loaded.timeRangeId) > 0) {
                    loaded.timeRanges = locationTimeRanges[loaded.timeRangeId];
                }
                else {
                    loaded.timeRanges = locationTimeRanges[-1];
                }

                if (locationCopPeds.count(loaded.cops) > 0) {
                    loaded.copModels = locationCopPeds[loaded.cops];
                }
                else {
                    loaded.copModels = locationCopPeds[-1];
                }


                loadedLocations.push_back(loaded);
            }
        }

        locations.close();
    }

    void spawnPed(Location currentLocation, bool copTime, CVector locationCoords, CObject* closest, int wantedLevel, bool civTime, CVector destination, CPlayerPed* player) {
        CPed* ped = nullptr;
        bool cop = false;
        float copRoll = static_cast<float>(rand()) / RAND_MAX * 100.0;
        bool spawnCop = currentLocation.copProb * wantedLevel >= copRoll;
        if (copTime && spawnCop) {
            int copModelIndex = rand() % currentLocation.copModels.size();
            int copModelId = currentLocation.copModels[copModelIndex];
            ped = createPed(copModelId, locationCoords, closest, PED_TYPE_COP);
            cop = true;
        }
        else if (civTime && !currentLocation.copsOnly) {
            int pedModelIndex = rand() % currentLocation.pedModels.size();
            int pedModelId = currentLocation.pedModels[pedModelIndex];
            ped = createPed(pedModelId, locationCoords, closest, PED_TYPE_CIVMALE);
        }

        if (!ped) {
            return;
        }

        int taskSequence = 0;

        if (cop) {
            eWeaponType copWeapon = copWeapons[rand() % copWeapons.size()];
            plugin::Command<0x01B2>(ped, copWeapon, 9999);

            plugin::Command<0x0615>(&taskSequence);
            plugin::Command<0x05D3>(-1, destination.x, destination.y, destination.z, 4, currentLocation.walkTime);
            plugin::Command<0x05E2>(-1, player);
            plugin::Command<0x0616>(taskSequence);
            plugin::Command<0x0618>(ped, taskSequence);

            plugin::Command<0x060B>(ped, 65537);
        }
        else {
            plugin::Command<0x0615>(&taskSequence);
            plugin::Command<0x05D3>(-1, destination.x, destination.y, destination.z, 4, currentLocation.walkTime);
            plugin::Command<0x05DE>(-1);
            plugin::Command<0x0616>(taskSequence);
            plugin::Command<0x0618>(ped, taskSequence);

            ped->SetPedDefaultDecisionMaker();
        }

        ped->SetCharCreatedBy(1);
        plugin::Command<0x01C2>(ped);
        CStreaming::SetModelIsDeletable(ped->m_nModelIndex);
        plugin::Command<0x061B>(taskSequence);

        if (lastPedModels.size() >= 3) {
            lastPedModels.erase(lastPedModels.begin());
        }
        lastPedModels.push_back(ped->m_nModelIndex);
    }

    Main()
    {
        srand(static_cast<unsigned int>(time(nullptr)));
        // register event callbacks
        Events::gameProcessEvent += [] { gInstance.OnGameProcess(); };
        loadLocations();
    }

    void OnGameProcess()
    {
        if (strncmp(CCheat::m_CheatString, "XENEP", 5) == 0) {
            static char msg[255];
            loadLocations();
            sprintf_s(msg, "Loaded %d Locations!", loadedLocations.size());
            CMessages::AddMessageJumpQ(msg, 10000, 0);
            CCheat::m_CheatString[0] = '\0';
        }
        for (auto it = spawnedPeds.begin(); it != spawnedPeds.end();) {
            CPed* oldPed = CPools::GetPed(*it);
            if (!oldPed) {
                it = spawnedPeds.erase(it);
                continue;
            }
            if (oldPed && oldPed->m_pIntelligence) {
                CTask* pedTask = oldPed->m_pIntelligence->m_TaskMgr.GetActiveTask();
                if (!pedTask || pedTask->GetId() == TASK_COMPLEX_WANDER || pedTask->GetId() == TASK_SIMPLE_STAND_STILL) {
                    oldPed->bFadeOut = true;
//                        plugin::Command<0x009B>(oldPed);
                    it = spawnedPeds.erase(it);
                    continue;
                }
            }
            ++it;
        }
        CPlayerPed* player = FindPlayerPed();
        if (!player) return;

        char inGameHours = CClock::ms_nGameClockHours;
        char inGameMinutes = CClock::ms_nGameClockMinutes;

        unsigned int inGameTime = inGameHours * 60 + inGameMinutes;

        static char msg[255];
        //sprintf_s(msg, "Hello from '%s' plugin! V2!", TARGET_NAME);

        for (Location& currentLocation : loadedLocations) {
            CVector locationCoords(currentLocation.x, currentLocation.y, currentLocation.z);

            if ((player->GetPosition() - locationCoords).MagnitudeSqr() < currentLocation.radiusSqr) {
                unsigned int currentTime = CTimer::m_snTimeInMilliseconds;

                bool copTime = false;
                bool civTime = false;

                unsigned int wantedLevel = player->m_pPlayerData->m_pWanted->m_nWantedLevel;

                if (currentTime > currentLocation.nextRing) {
                    if (currentLocation.cops > 0 && wantedLevel > 0) {
                        for (const TimeRange& copTimeRange : locationTimeRanges[0]) {
                            if (copTimeRange.lowEnd <= inGameTime && inGameTime <= copTimeRange.highEnd) copTime = true;
                        }
                    }
                    for (const TimeRange& timeRange : currentLocation.timeRanges) {
                        if (timeRange.lowEnd <= inGameTime && inGameTime <= timeRange.highEnd) civTime = true;
                    }
                }

                if(civTime || copTime) {
                    currentLocation.nextRing = currentTime + currentLocation.interval;

                    float randomVal = static_cast<float>(rand()) / RAND_MAX * 100.0;

                    if (currentLocation.probability >= randomVal) {
                        CObject* closest = nullptr;
                        float dist = currentLocation.radiusSqr;

                        for (CObject* object : CPools::ms_pObjectPool) {
                            if (!object || !object->m_pRwObject || object->m_nModelIndex != currentLocation.model) continue;
                            float squaredObjDist = (object->GetPosition() - locationCoords).MagnitudeSqr();
                            if (squaredObjDist < dist) {
                                dist = squaredObjDist;
                                closest = object;
                            }
                        }

                        if (closest) {
                            CVector doorCenter = closest->GetPosition() + (closest->GetMatrix().GetRight() * 1.0f);
                            CVector destination = currentLocation.invertWalk
                                ? (doorCenter + (closest->GetMatrix().GetForward() * currentLocation.walkDist))
                                : (doorCenter - (closest->GetMatrix().GetForward() * currentLocation.walkDist));
                            float leaveVal = static_cast<float>(rand()) / RAND_MAX * 100.0;

                            plugin::Command<0x0905>(closest, 0);

                            if (currentLocation.leaveProb >= leaveVal) {
                                CPed* closestPed = nullptr;
                                float maxPedDist = currentLocation.radiusSqr;
                                for (CPed* randPed : CPools::ms_pPedPool) {
                                    if (!randPed || randPed->m_nModelIndex == 0 || std::ranges::contains(lastPedModels, randPed->m_nModelIndex) || !randPed->IsAlive() || randPed->m_pVehicle) continue;
                                    if (std::ranges::contains(currentLocation.pedModels, randPed->m_nModelIndex)) {
                                        float squaredPedDist = (randPed->GetPosition() - locationCoords).MagnitudeSqr();
                                        if (squaredPedDist < maxPedDist) {
                                            maxPedDist = squaredPedDist;
                                            closestPed = randPed;
/*                                            float breakRoll = static_cast<float>(rand()) / RAND_MAX * 100.0;
                                            if (breakRoll <= 20.0f) {
                                                break;
                                            }*/
                                        }
                                    }
                                }

                                if (closestPed) {
                                    if (lastPedModels.size() >= 3) {
                                        lastPedModels.erase(lastPedModels.begin());
                                    }
                                    lastPedModels.push_back(closestPed->m_nModelIndex);
//                                    CVector finalDest = currentLocation.invertWalk
                                        //? (doorCenter - (closest->GetMatrix().GetForward() * currentLocation.walkDist))
                                        //: (doorCenter + (closest->GetMatrix().GetForward() * currentLocation.walkDist));
                                    int taskSequence = 0;
                                    plugin::Command<0x0615>(&taskSequence);
                                    plugin::Command<0x05D3>(-1, destination.x, destination.y, destination.z, 4, currentLocation.walkTime);
                                    plugin::Command<0x05D3>(-1, currentLocation.x, currentLocation.y, currentLocation.z, 4, currentLocation.walkTime);
                                    plugin::Command<0x0616>(taskSequence);
                                    plugin::Command<0x0618>(closestPed, taskSequence);
                                    plugin::Command<0x061B>(taskSequence);
                                    
                                    closestPed->SetCharCreatedBy(1);
                                    plugin::Command<0x01C2>(closestPed);
                                    CStreaming::SetModelIsDeletable(closestPed->m_nModelIndex);

                                    spawnedPeds.push_back(CPools::GetPedRef(closestPed));

                                    continue;
                                }
                                else {
                                    if (currentLocation.lfb) {
                                        spawnPed(currentLocation, copTime, locationCoords, closest, wantedLevel, civTime, destination, player);
                                    }
                                }
                            } else {
                                spawnPed(currentLocation, copTime, locationCoords, closest, wantedLevel, civTime, destination, player);
                            }
                        }
                    }
                }
            }
        }
    }
} gInstance;
