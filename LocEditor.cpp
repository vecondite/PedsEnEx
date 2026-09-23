#include "plugin.h"
#include "CMessages.h"
#include "CGame.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace plugin;

struct Main
{
    std::vector<std::string> locations;
    int currentIndex = -1;

    // Key debouncing toggles
    bool semiColonPressed = false;
    bool f12Pressed = false;

    Main()
    {
        Events::initGameEvent += [this] { LoadLocations(); };
        Events::gameProcessEvent += [this] { OnGameProcess(); };
    }

    void LoadLocations()
    {
        locations.clear();
        std::ifstream inFile(PLUGIN_PATH("locations.txt"));

        if (!inFile.is_open()) return;

        std::string line;
        while (std::getline(inFile, line)) {
            if (!line.empty()) {
                locations.push_back(line);
            }
        }
        currentIndex = -1;
    }

    void OnGameProcess()
    {
        CPlayerPed* player = FindPlayerPed();
        if (!player) return;

        // VK_OEM_1 is the virtual key code for the ';' (Semicolon) key on standard keyboards
        bool isSemiColonDown = KeyPressed(VK_OEM_1);
        if (isSemiColonDown && !semiColonPressed) {
            semiColonPressed = true;
            TeleportToNextLocation(player);
        }
        else if (!isSemiColonDown) {
            semiColonPressed = false;
        }

        // VK_F12 for deletion
        bool isF12Down = KeyPressed(VK_F12);
        if (isF12Down && !f12Pressed) {
            f12Pressed = true;
            DeleteCurrentLocation();
        }
        else if (!isF12Down) {
            f12Pressed = false;
        }
    }

    void TeleportToNextLocation(CPlayerPed* player)
    {
        if (locations.empty()) {
            CMessages::AddMessageJumpQ("Locations list is empty!", 3000, 0);
            return;
        }

        // Advance to the next location, loop back to zero if at the end
        currentIndex++;
        if (currentIndex >= locations.size()) {
            currentIndex = 0;
        }

        std::stringstream ss(locations[currentIndex]);
        float x, y, z;

        // Extract the first three fields directly into floats
        if (ss >> x >> y >> z) {
            // Force exterior area code to prevent interior clipping bugs
            player->m_nAreaCode = 0;
            CGame::currArea = 0;

            // Set position (adding 1.0f to Z to prevent spawning inside the floor collision)
            player->SetPosn(x, y, z + 1.0f);

            static char msg[256];
            sprintf_s(msg, "Teleported to %d / %d", currentIndex + 1, (int)locations.size());
            CMessages::AddMessageJumpQ(msg, 3000, 0);
        }
    }

    void DeleteCurrentLocation()
    {
        if (locations.empty() || currentIndex < 0 || currentIndex >= locations.size()) {
            CMessages::AddMessageJumpQ("No valid location to delete!", 3000, 0);
            return;
        }

        // Remove the current line from the vector
        locations.erase(locations.begin() + currentIndex);

        // Overwrite the locations.txt file completely with the new vector contents
        std::ofstream outFile(PLUGIN_PATH("locations.txt"), std::ios::trunc);
        if (outFile.is_open()) {
            for (const std::string& line : locations) {
                outFile << line << "\n";
            }
            outFile.close();
        }

        static char msg[256];
        sprintf_s(msg, "Location deleted. %d remaining.", (int)locations.size());
        CMessages::AddMessageJumpQ(msg, 4000, 0);

        // Step the index backward so the next time ';' is pressed, it goes to the correct new line
        currentIndex--;
    }

} gInstance;
