// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

#include "MinHook.h"

#pragma warning(disable: 4996)

using namespace std;
namespace fs = std::filesystem;

std::wstring utf8_to_utf16(const std::string& utf8);

typedef DWORD(WINAPI* GETVERSION)();

GETVERSION p_original_GetVersion = NULL;

static DWORD WINAPI detoured_GetVersion()
{
    static bool firstTime = true;

    if (firstTime) {
        firstTime = false;

        vector<fs::path> load_list;

        for (auto const& dir_entry : fs::directory_iterator{ fs::current_path() }) {
            if (dir_entry.is_directory() == false) {
                auto p = dir_entry.path();
                auto name = p.stem().u8string();
                auto extension = p.extension().u8string();

                if (extension == u8".dll" && name.find(u8"Vanilla1121mod") != name.npos) {
                    load_list.push_back(p);
                }
            }
        }

        if (load_list.size() > 0) {
            vector<fs::path> successful;

            for (auto const& mod : load_list) {
                HMODULE h = LoadLibraryW(utf8_to_utf16(mod.u8string()).data());
                if (h != NULL) {
                    successful.push_back(mod);
                }
            }

            if (successful.size() > 0) {
                wstringstream message;

                message << utf8_to_utf16(u8"These following mods have been loaded into game and executed:") << endl << endl;
                for (auto const& mod : successful) {
                    message << utf8_to_utf16(mod.filename().u8string()) << endl;
                }
                message << endl << utf8_to_utf16(u8"Please beware that mods are powerful tool and use them at your own risk. Applying arbitrary mods onto Azeroth could lead into Burning Legion invasion or crucial information damage/leak.");

                //MessageBoxW(NULL, message.str().data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            }
        }
    }

    return p_original_GetVersion();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        if (MH_Initialize() != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to initialize MinHook library.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        if (MH_CreateHook(&GetVersion, &detoured_GetVersion, reinterpret_cast<LPVOID*>(&p_original_GetVersion)) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to create hook for loading function.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        if (MH_EnableHook(&GetVersion) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed when enabling loading function.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        MH_Uninitialize();
        break;
    }
    return TRUE;
}

// This function is needed for PE import directory entity
extern "C" __declspec(dllexport) void function_doing_nothing(void) {
    return;
}


