// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include "processthreadsapi.h"
#include "synchapi.h"
#include "libloaderapi.h"

#include <vector>
#include <string>
#include <sstream>
#include <filesystem>
#include <cstdlib>

#include "MinHook.h"
#include "textfile.h"

#pragma warning(disable: 4996)

using namespace std;
namespace fs = std::filesystem;

std::wstring utf8_to_utf16(const std::string& utf8);

typedef DWORD(WINAPI* GETVERSION)();
typedef void(__fastcall* CGGameUI__EnterWorld)(void);
typedef void(*MODULE_ENTERWORLD)(void);

GETVERSION p_original_GetVersion = NULL;
CGGameUI__EnterWorld p_EnterWorld = reinterpret_cast<CGGameUI__EnterWorld>(0x4908C0); // Called after Every Loading screen.
CGGameUI__EnterWorld p_original_EnterWorld = NULL;

vector<HMODULE> loaded_modules;
static HMODULE hSelf = 0;

DWORD WINAPI detoured_GetVersion()
{
    static bool firstTime = true;

    if (firstTime) {
        firstTime = false;

        wchar_t tempWoW_exe[MAX_PATH];
        if (GetModuleFileName(NULL, tempWoW_exe, MAX_PATH) == 0) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to get WoW.exe path. Nothing is loaded.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return p_original_GetVersion();
        }

        fs::path WoW_exe{ tempWoW_exe };

        vector<fs::path> load_list;

        fs::path configFilename{ u8"dlls.txt" };

        int linesRead = 0;
        LPWSTR* vanillaFixesLoadList = FromTextFile(utf8_to_utf16((WoW_exe.parent_path() / configFilename).u8string()).data(), &linesRead, TRUE);
        for (int i = 0; i < linesRead; ++i) {
            fs::path p{ vanillaFixesLoadList[i] };
            load_list.push_back(p);
        }
        free(vanillaFixesLoadList);

        if (load_list.size() > 0) {
            vector<fs::path> successful;
            vector<fs::path> failed;

            for (auto const& mod : load_list) {
                HMODULE test = GetModuleHandleW(utf8_to_utf16(mod.filename().u8string()).data());
                if (test != NULL) {
                    // Skip already loaded modules
                    continue;
                }

                HMODULE h = LoadLibraryW(utf8_to_utf16((WoW_exe.parent_path() / mod.filename()).u8string()).data());
                if (h != NULL) {
                    successful.push_back(mod);
                    loaded_modules.push_back(h);

                    // nampower require additional loading step
                    if (mod.filename().u8string().find(u8"nampower") != string::npos) {
                        typedef DWORD(*PLOAD)(void);
                        PLOAD pLoad = (PLOAD)GetProcAddress(h, "Load");
                        if (pLoad) {
                            pLoad();
                        }
                    }
                }
                else {
                    failed.push_back(mod);
                }
            }

            if (failed.size() > 0) {
                stringstream ss;
                ss << u8"Some mods have failed to load:" << endl << endl;
                for (auto const& mod : failed) {
                    ss << mod.u8string() << endl;
                }
                ss << endl << u8"You could edit \"dlls.txt\" file in game's folder to disable them. Game would start without them. Enjoy.";
                
                MessageBoxW(NULL, utf8_to_utf16(ss.str()).data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            }
        }
    }

    return p_original_GetVersion();
}

DWORD WINAPI threaded_selfEject(LPVOID lpParameter) {
    HMODULE* p_hSelf = static_cast<HMODULE*>(lpParameter);

    Sleep(3000); // Wait a few seconds for detoured_EnterWorld() to return

    FreeLibraryAndExitThread(*p_hSelf, 0);
}

void __fastcall detoured_EnterWorld(void) {
    p_original_EnterWorld();

    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        
        for (auto m : loaded_modules) {
            MODULE_ENTERWORLD p_module_EnterWorld = reinterpret_cast<MODULE_ENTERWORLD>(GetProcAddress(m, "FirstEnterWorld"));
            if (p_module_EnterWorld) {
                p_module_EnterWorld();
            }
        }

        // We done all our job, starting self eject
        CreateThread(NULL, 0, &threaded_selfEject, &hSelf, 0, NULL);
    }
    return;
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

        hSelf = hModule; // Save handle for later self eject

        if (MH_Initialize() != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to initialize MinHook.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        if (MH_CreateHook(&GetVersion, &detoured_GetVersion, reinterpret_cast<LPVOID*>(&p_original_GetVersion)) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to create hook for loading function.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        if (MH_CreateHook(p_EnterWorld, &detoured_EnterWorld, reinterpret_cast<LPVOID*>(&p_original_EnterWorld)) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to create hook for EnterWorld function.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed when enabling MinHooks.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to disable MinHook. Game might crash later.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        if (MH_RemoveHook(p_EnterWorld) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to remove hook for EnterWorld function. Game might crash later.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        if (MH_RemoveHook(&GetVersion) != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to remove hook for loading function. Game might crash later.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        if (MH_Uninitialize() != MH_OK) {
            MessageBoxW(NULL, utf8_to_utf16(u8"Failed to unintialize MinHook. Game might crash later.").data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return FALSE;
        }
        break;
    }
    return TRUE;
}

// This function is needed for PE import directory entity
extern "C" __declspec(dllexport) void function_doing_nothing(void) {
    return;
}


