#include <plugin.h> // Plugin-SDK version 1005
#include <CMessages.h>
#include <CFont.h>
#include <CGame.h>
#include <CPools.h>
#include <CDummy.h>
#include <CPlayerPed.h>
#include <CStreaming.h>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <exception>

using namespace plugin;

// --- CONFIGURATION ---
constexpr bool AVOID_ALREADY_MAPPED = true;

struct DoorData {
    int modelIndex;
    int areaCode;
    CVector doorCenter;
    CVector forwardVec;
};

struct LocationInfo {
    CVector pos;
    int areaCode;
};

struct Main {
    std::vector<DoorData> doorsToReview;
    std::unordered_set<std::string> skippedItems;
    int currentIndex = 0;

    // State flags
    bool isPreWarming = true;
    bool isReviewing = false;

    // Pre-warm teleporter state
    size_t preWarmIndex = 0;
    DWORD lastPreWarmTick = 0;
    const DWORD PREWARM_DELAY_MS = 200;

    // Toggle states
    int iw = 0;
    int destMode = 0;

    // Key debouncing
    bool backspacePressed = false;
    bool bPressed = false;
    bool iPressed = false;
    bool pPressed = false;
    bool gravePressed = false;

    std::unordered_set<int> interiorDoors = {
        1491, 1492, 1494, 1495, 1497, 1498, 1499, 1500, 1501, 1502,
        1504, 1505, 1506, 1507, 1523, 1532, 1533, 1535, 1536, 1537,
        1538, 1555, 1556, 1557, 1560, 1561, 1566, 1567, 1569
    };

    // Plane interiors (Shamal and Andromada) have been removed from this list
    const std::vector<LocationInfo> preWarmLocations = {
        { { -25.8845f, -185.869f, 1003.547f }, 17 },
        { { 6.0912f, -29.2719f, 1003.549f }, 10 },
        { { -30.9467f, -89.6096f, 1003.547f }, 18 },
        { { -25.1326f, -139.067f, 1003.547f }, 16 },
        { { -27.3123f, -29.2776f, 1003.557f }, 4 },
        { { -26.6916f, -55.7149f, 1003.547f }, 6 },
        { { -1827.147f, 7.2074f, 1061.144f }, 14 },
        { { -1861.937f, 54.9081f, 1061.144f }, 14 },
        { { 286.149f, -40.6444f, 1001.516f }, 1 },
        { { 286.801f, -82.5476f, 1001.516f }, 4 },
        { { 296.92f, -108.072f, 1001.516f }, 6 },
        { { 314.821f, -141.432f, 999.6016f }, 7 },
        { { 316.525f, -167.707f, 999.5938f }, 6 },
        { { 302.2929f, -143.1391f, 1004.063f }, 7 },
        { { 298.5079f, -141.647f, 1004.055f }, 7 },
        { { 1038.531f, 0.111f, 1001.284f }, 3 },
        { { 444.6469f, 508.239f, 1001.419f }, 12 },
        { { 2215.4551f, -1147.476f, 1025.797f }, 15 },
        { { 833.2698f, 10.5884f, 1004.18f }, 3 },
        { { -103.5592f, -24.2256f, 1000.719f }, 3 },
        { { 963.4188f, 2108.292f, 1011.03f }, 1 },
        { { -2240.469f, 137.0604f, 1035.4139f }, 6 },
        { { 663.8362f, -575.6054f, 16.3433f }, 0 },
        { { 2169.4609f, 1618.798f, 999.9766f }, 1 },
        { { 1889.953f, 1017.438f, 31.8828f }, 10 },
        { { -2159.123f, 641.5175f, 1052.382f }, 1 },
        { { 207.738f, -109.02f, 1005.133f }, 15 },
        { { 204.333f, -166.695f, 1000.523f }, 14 },
        { { 207.055f, -138.805f, 1003.508f }, 3 },
        { { 203.778f, -48.4924f, 1001.805f }, 1 },
        { { 226.294f, -7.4315f, 1002.211f }, 5 },
        { { 161.391f, -93.1592f, 1001.805f }, 18 },
        { { 493.391f, -22.7228f, 1000.68f }, 17 },
        { { 501.981f, -69.1502f, 998.7578f }, 11 },
        { { -227.028f, 1401.23f, 27.7656f }, 18 },
        { { 457.3047f, -88.4285f, 999.5547f }, 4 },
        { { 454.9739f, -110.105f, 1000.077f }, 5 },
        { { 435.2713f, -80.9589f, 999.5547f }, 6 },
        { { 452.49f, -18.1797f, 1001.133f }, 1 },
        { { 681.5579f, -455.6801f, -25.6099f }, 1 },
        { { 375.9625f, -65.8168f, 1001.508f }, 10 },
        { { 369.5795f, -4.4873f, 1001.859f }, 9 },
        { { 373.8257f, -117.2709f, 1001.5f }, 5 },
        { { 381.1692f, -188.803f, 1000.633f }, 17 },
        { { 223.432f, 1872.4f, 13.7344f }, 0 },
        { { 772.112f, -3.8986f, 1000.729f }, 5 },
        { { 774.214f, -48.9243f, 1000.586f }, 6 },
        { { 773.58f, -77.0967f, 1000.655f }, 7 },
        { { 244.412f, 305.033f, 999.1484f }, 1 },
        { { 271.885f, 306.632f, 999.1484f }, 2 },
        { { 291.283f, 310.032f, 999.1484f }, 3 },
        { { 302.181f, 300.723f, 999.1484f }, 4 },
        { { 322.198f, 302.498f, 999.1484f }, 5 },
        { { 346.87f, 309.259f, 999.1557f }, 6 },
        { { -959.5644f, 1848.577f, 9.0f }, 17 },
        { { 384.8086f, 173.805f, 1008.383f }, 3 },
        { { 1527.23f, -11.5745f, 1002.097f }, 3 },
        { { 1523.51f, -47.8212f, 1002.131f }, 2 },
        { { 2496.05f, -1695.238f, 1014.742f }, 3 },
        { { 1267.663f, -781.3232f, 1091.906f }, 5 },
        { { 513.8825f, -11.27f, 1001.565f }, 3 },
        { { 2454.717f, -1700.8719f, 1013.515f }, 2 },
        { { 2527.6541f, -1679.3879f, 1015.499f }, 1 },
        { { 2543.4629f, -1308.38f, 1026.728f }, 2 },
        { { 1212.02f, -28.6631f, 1000.953f }, 3 },
        { { 761.413f, 1440.192f, 1102.703f }, 6 },
        { { 1204.8101f, -11.5868f, 1000.922f }, 2 },
        { { 1204.8101f, 13.8972f, 1000.922f }, 2 },
        { { 942.172f, -16.5428f, 1000.93f }, 3 },
        { { 964.107f, -53.2055f, 1001.125f }, 3 },
        { { -2640.7629f, 1406.682f, 906.4609f }, 3 },
        { { -729.276f, 503.0869f, 1371.972f }, 1 },
        { { -794.8064f, 497.738f, 1376.1949f }, 1 },
        { { 2350.3401f, -1181.65f, 1027.9771f }, 5 },
        { { 2807.6201f, -1171.9f, 1025.5699f }, 8 },
        { { 318.565f, 1118.21f, 1083.8831f }, 5 },
        { { 1412.64f, -1.7875f, 1000.924f }, 1 },
        { { 1302.52f, -1.7875f, 1001.028f }, 2 },
        { { -221.0591f, 1408.984f, 27.7734f }, 18 },
        { { 2324.4199f, -1145.568f, 1050.71f }, 12 },
        { { -975.9757f, 1060.983f, 1345.672f }, 10 }
    };

    Main() {
        ProcessCrashRecovery();

        Events::gameProcessEvent += [this] { OnGameProcess(); };
        Events::drawHudEvent += [this] { OnDrawHud(); };
    }

    // --- CRASH RECOVERY SYSTEM ---
    void ProcessCrashRecovery() {
        std::ifstream inFile(PLUGIN_PATH("last_attempt.txt"));
        std::string crashedId;
        if (inFile.is_open()) {
            std::getline(inFile, crashedId);
            inFile.close();
            remove(PLUGIN_PATH("last_attempt.txt")); // Removed the invalid .c_str() 
        }

        // If the file existed and had content, the game crashed during the last teleport attempt.
        if (!crashedId.empty()) {
            std::ofstream skipFile(PLUGIN_PATH("skipped_locations.txt"), std::ios::app);
            skipFile << crashedId << "\n";
            skipFile.close();
        }

        // Load all permanently skipped doors/pre-warms into memory
        std::ifstream skipIn(PLUGIN_PATH("skipped_locations.txt"));
        if (skipIn.is_open()) {
            std::string line;
            while (std::getline(skipIn, line)) {
                if (!line.empty()) skippedItems.insert(line);
            }
            skipIn.close();
        }
    }

    void WriteLastAttempt(const std::string& id) {
        std::ofstream outFile(PLUGIN_PATH("last_attempt.txt"));
        outFile << id;
        outFile.close();
    }

    std::string GetDoorUniqueId(CVector center, int model) {
        char buf[128];
        sprintf_s(buf, "%d_%d_%d_%d", (int)center.x, (int)center.y, (int)center.z, model);
        return "door_" + std::string(buf);
    }
    // ----------------------------

    std::unordered_set<std::string> LoadExistingMappedLocations() {
        std::unordered_set<std::string> existingCoords;
        try {
            std::ifstream inFile(PLUGIN_PATH("locations.txt"));
            if (!inFile.is_open()) return existingCoords;

            std::string line;
            while (std::getline(inFile, line)) {
                if (line.empty() || line[0] == '/' || line[0] == '#') continue;
                std::stringstream ss(line);
                float x, y, z;
                if (ss >> x >> y >> z) {
                    char coordKey[128];
                    sprintf_s(coordKey, "%.1f_%.1f_%.1f", x, y, z);
                    existingCoords.insert(coordKey);
                }
            }
            inFile.close();
        }
        catch (...) {}
        return existingCoords;
    }

    std::unordered_set<std::string> GetExistingQueueIdentifiers() {
        std::unordered_set<std::string> queueSet;
        try {
            for (const auto& door : doorsToReview) {
                queueSet.insert(GetDoorUniqueId(door.doorCenter, door.modelIndex));
            }
        }
        catch (...) {}
        return queueSet;
    }

    void PerformDoorScan() {
        try {
            if (!CPools::ms_pDummyPool) return;

            std::unordered_set<std::string> existingMapped;
            if (AVOID_ALREADY_MAPPED) {
                existingMapped = LoadExistingMappedLocations();
            }

            std::unordered_set<std::string> queuedDoors = GetExistingQueueIdentifiers();

            for (CDummy* object : CPools::ms_pDummyPool) {
                if (!object) continue;

                if (interiorDoors.count(object->m_nModelIndex) && object->m_nAreaCode > 0) {
                    CVector doorCenter;
                    CVector forwardVec;

                    if (object->m_matrix) {
                        doorCenter = object->GetPosition() + (object->m_matrix->GetRight() * 1.0f);
                        forwardVec = object->m_matrix->GetForward();
                    }
                    else {
                        float heading = object->GetHeading();
                        doorCenter = object->GetPosition() + CVector(cosf(heading), sinf(heading), 0.0f) * 1.0f;
                        forwardVec = CVector(-sinf(heading), cosf(heading), 0.0f);
                    }

                    // Ensure this door hasn't been manually skipped or crashed previously
                    std::string fullSkipId = GetDoorUniqueId(doorCenter, object->m_nModelIndex);
                    if (skippedItems.count(fullSkipId)) continue;

                    CVector destMinus = doorCenter - (forwardVec * 2.0f);
                    CVector destPlus = doorCenter + (forwardVec * 2.0f);

                    char keyMinus[128], keyPlus[128];
                    sprintf_s(keyMinus, "%.1f_%.1f_%.1f", destMinus.x, destMinus.y, destMinus.z);
                    sprintf_s(keyPlus, "%.1f_%.1f_%.1f", destPlus.x, destPlus.y, destPlus.z);

                    if (AVOID_ALREADY_MAPPED &&
                        (existingMapped.count(keyMinus) || existingMapped.count(keyPlus))) {
                        continue;
                    }

                    if (queuedDoors.find(fullSkipId) == queuedDoors.end()) {
                        queuedDoors.insert(fullSkipId);
                        doorsToReview.push_back({ object->m_nModelIndex, object->m_nAreaCode, doorCenter, forwardVec });
                    }
                }
            }
        }
        catch (...) {
            CMessages::AddMessageJumpQ("Unknown Error in Door Scan!", 2000, 0);
        }
    }

    void ProcessPreWarm() {
        try {
            DWORD currentTick = GetTickCount();
            if (currentTick - lastPreWarmTick < PREWARM_DELAY_MS) return;
            lastPreWarmTick = currentTick;

            CPlayerPed* player = FindPlayerPed();
            if (!player) return;

            // Automatically skip any pre-warms that caused crashes in previous sessions
            while (preWarmIndex < preWarmLocations.size()) {
                std::string prewarmId = "prewarm_" + std::to_string(preWarmIndex);
                if (skippedItems.count(prewarmId)) {
                    preWarmIndex++;
                }
                else {
                    break;
                }
            }

            if (preWarmIndex < preWarmLocations.size()) {
                // Write attempt file before teleporting
                WriteLastAttempt("prewarm_" + std::to_string(preWarmIndex));

                auto& loc = preWarmLocations.at(preWarmIndex);
                player->m_nAreaCode = loc.areaCode;
                CGame::currArea = loc.areaCode;
                player->SetPosn(loc.pos.x, loc.pos.y, loc.pos.z);

                CStreaming::LoadScene(&loc.pos);

                PerformDoorScan();
                preWarmIndex++;
            }
            else {
                isPreWarming = false;
                PerformDoorScan();

                if (!doorsToReview.empty()) {
                    isReviewing = true;
                    currentIndex = 0;
                    iw = 0;
                    destMode = 0;
                    TeleportToCurrent();
                    CMessages::AddMessageJumpQ("Pre-warm complete! Starting review mode...", 3000, 0);
                }
                else {
                    CMessages::AddMessageJumpQ("Pre-warm complete! No unmapped doors found.", 3000, 0);
                }
            }
        }
        catch (...) {
            preWarmIndex++;
        }
    }

    CVector GetCurrentDestination() {
        if (doorsToReview.empty() || currentIndex < 0 || currentIndex >= (int)doorsToReview.size()) {
            return CVector(0.0f, 0.0f, 0.0f);
        }

        auto& door = doorsToReview.at(currentIndex);
        if (destMode == 0) {
            return door.doorCenter - (door.forwardVec * 2.0f);
        }
        else {
            return door.doorCenter + (door.forwardVec * 2.0f);
        }
    }

    void TeleportToCurrent() {
        try {
            if (doorsToReview.empty() || currentIndex >= (int)doorsToReview.size()) {
                isReviewing = false;
                return;
            }

            if (currentIndex < 0) currentIndex = 0;

            auto& door = doorsToReview.at(currentIndex);
            CVector dest = GetCurrentDestination();

            // Write attempt file before teleporting to catch potential crashes
            WriteLastAttempt(GetDoorUniqueId(door.doorCenter, door.modelIndex));

            CPlayerPed* player = FindPlayerPed();
            if (player) {
                player->m_nAreaCode = door.areaCode;
                CGame::currArea = door.areaCode;
                player->SetPosn(dest.x, dest.y, dest.z + 1.0f);
                CStreaming::LoadScene(&dest);
            }
        }
        catch (...) {}
    }

    void SaveCurrentLocation() {
        try {
            if (currentIndex < 0 || currentIndex >= (int)doorsToReview.size()) return;

            auto& door = doorsToReview.at(currentIndex);
            CVector dest = GetCurrentDestination();

            std::ofstream outFile(PLUGIN_PATH("locations.txt"), std::ios::app);
            if (outFile.is_open()) {
                char line[1024];
                sprintf_s(line, "%f %f %f 20.0 %d 30000 100.0 2.5 60000 %d -1 -1 1 15.0 0 40.0 0",
                    dest.x, dest.y, dest.z, door.modelIndex, iw);
                outFile << line << "\n";
                outFile.close();

                char msg[256];
                sprintf_s(msg, "Saved Location %d/%d!", currentIndex + 1, (int)doorsToReview.size());
                CMessages::AddMessageJumpQ(msg, 2000, 0);
            }
        }
        catch (...) {}
    }

    void NextLocation() {
        currentIndex++;
        iw = 0;
        destMode = 0;

        if (currentIndex < (int)doorsToReview.size()) {
            TeleportToCurrent();
        }
        else {
            isReviewing = false;
            CMessages::AddMessageJumpQ("Reached end of queue. Review finished.", 3000, 0);
        }
    }

    void PrevLocation() {
        if (!doorsToReview.empty() && currentIndex > 0) {
            currentIndex--;
            iw = 0;
            destMode = 0;
            isReviewing = true;
            TeleportToCurrent();
            CMessages::AddMessageJumpQ("Went back to previous location.", 1500, 0);
        }
        else {
            CMessages::AddMessageJumpQ("Already at the first location.", 1500, 0);
        }
    }

    void OnGameProcess() {
        try {
            CPlayerPed* player = FindPlayerPed();
            if (!player) return;

            if (isPreWarming) {
                ProcessPreWarm();
                return;
            }

            if (!isReviewing || doorsToReview.empty()) return;

            bool isBackspaceDown = KeyPressed(VK_BACK);
            if (isBackspaceDown && !backspacePressed) {
                backspacePressed = true;

                if (destMode == 0) {
                    destMode = 1;
                    TeleportToCurrent();
                    CMessages::AddMessageJumpQ("Skipped (-). Auto-switched to (+) dest.", 1500, 0);
                }
                else {
                    // Permanently block this door if skipped on the (+) side
                    std::string skipId = GetDoorUniqueId(doorsToReview.at(currentIndex).doorCenter, doorsToReview.at(currentIndex).modelIndex);
                    skippedItems.insert(skipId);

                    std::ofstream skipFile(PLUGIN_PATH("skipped_locations.txt"), std::ios::app);
                    skipFile << skipId << "\n";
                    skipFile.close();

                    CMessages::AddMessageJumpQ("Door Blacklisted. Will not load again.", 2000, 0);
                    NextLocation();
                }
            }
            else if (!isBackspaceDown) backspacePressed = false;

            bool isBDown = KeyPressed(0x42);
            if (isBDown && !bPressed) {
                bPressed = true;
                PrevLocation();
            }
            else if (!isBDown) bPressed = false;

            bool isIDown = KeyPressed(0x49);
            if (isIDown && !iPressed) {
                iPressed = true;
                iw = (iw == 0) ? 1 : 0;
            }
            else if (!isIDown) iPressed = false;

            bool isPDown = KeyPressed(0x50);
            if (isPDown && !pPressed) {
                pPressed = true;
                destMode = (destMode == 0) ? 1 : 0;
                TeleportToCurrent();
            }
            else if (!isPDown) pPressed = false;

            bool isGraveDown = KeyPressed(VK_OEM_3);
            if (isGraveDown && !gravePressed) {
                gravePressed = true;
                SaveCurrentLocation();
                NextLocation();
            }
            else if (!isGraveDown) gravePressed = false;
        }
        catch (...) {
            NextLocation();
        }
    }

    void OnDrawHud() {
        try {
            if (isPreWarming) {
                char hudText[256];
                sprintf_s(hudText, "Pre-warming interiors...~n~Progress: %d / %d~n~Doors Found: %d",
                    (int)preWarmIndex, (int)preWarmLocations.size(), (int)doorsToReview.size());

                CFont::SetBackground(false, false);
                CFont::SetScale(0.5f * (static_cast<float>(RsGlobal.maximumWidth) / 640.0f),
                    0.9f * (static_cast<float>(RsGlobal.maximumHeight) / 448.0f));
                CFont::SetProportional(true);
                CFont::SetJustify(false);
                CFont::SetOrientation(ALIGN_LEFT);
                CFont::SetFontStyle(FONT_SUBTITLES);
                CFont::SetEdge(1);
                CFont::SetDropColor(CRGBA(0, 0, 0, 255));
                CFont::SetColor(CRGBA(255, 255, 0, 255));

                CFont::PrintString(20.0f, 150.0f, hudText);
                return;
            }

            if (!isReviewing || doorsToReview.empty() || currentIndex < 0 || currentIndex >= (int)doorsToReview.size()) return;

            auto& door = doorsToReview.at(currentIndex);
            CVector dest = GetCurrentDestination();

            char hudText[512];
            sprintf_s(hudText, "Location: %d / %d~n~Coords: %.2f, %.2f, %.2f~n~iw: %d~n~Dest: %s~n~~n~[P] Toggle Dest  |  [I] Toggle iw~n~[`] Save  |  [BACKSPACE] Skip  |  [B] Go Back",
                currentIndex + 1, (int)doorsToReview.size(),
                dest.x, dest.y, dest.z,
                iw,
                (destMode == 0) ? "MINUS (-)" : "PLUS (+)");

            CFont::SetBackground(false, false);
            CFont::SetScale(0.5f * (static_cast<float>(RsGlobal.maximumWidth) / 640.0f),
                0.9f * (static_cast<float>(RsGlobal.maximumHeight) / 448.0f));
            CFont::SetProportional(true);
            CFont::SetJustify(false);
            CFont::SetOrientation(ALIGN_LEFT);
            CFont::SetFontStyle(FONT_SUBTITLES);
            CFont::SetEdge(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, 255));
            CFont::SetColor(CRGBA(255, 255, 255, 255));

            CFont::PrintString(20.0f, 150.0f, hudText);
        }
        catch (...) {}
    }

} gInstance;
