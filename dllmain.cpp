#include "pch.h"

#include "MinHook.h"

#pragma warning(disable: 4996)

typedef DWORD (WINAPI* GETVERSION)();

GETVERSION pOriginalGetVersion = NULL;

DWORD WINAPI DetourGetVersion()
{
    static BOOL alreadyLoaded = FALSE;

    if(alreadyLoaded == FALSE) {
        // We don't really care if sideloading failed. Ignoring return values.
        LoadLibraryW(L"VfPatcher.dll");
        LoadLibraryW(L"SuperWoWhook.dll");

        HMODULE handle = LoadLibraryW(L"nampower.dll");
        if (handle) {
            typedef DWORD(*PLOAD)();
            PLOAD nampowerLoad = (PLOAD)GetProcAddress(handle, "Load");
            if (nampowerLoad) {
                nampowerLoad();
            }
        }
        alreadyLoaded = TRUE;
    }

    MH_DisableHook(&GetVersion);
    
    return pOriginalGetVersion();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (MH_Initialize() != MH_OK) {
            return FALSE;
        }
        if (MH_CreateHook(&GetVersion, &DetourGetVersion, reinterpret_cast<LPVOID*>(&pOriginalGetVersion)) != MH_OK) {
            return FALSE;
        }

        if (MH_EnableHook(&GetVersion) != MH_OK) {
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        MH_Uninitialize();
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void dummy() {
    return;
}
