#include "pch.h"

#include <stdio.h>

#include "MinHook.h"

#pragma warning(disable: 4996)

typedef DWORD(WINAPI* GETVERSION)();

GETVERSION pOriginalGetVersion = NULL;

DWORD WINAPI DetouredGetVersion()
{
	static BOOL alreadyLoaded = FALSE;

	if (!alreadyLoaded) {
		LPCWSTR dllNames[] = { L"VfPatcher.dll" , L"SuperWoWhook.dll" , L"VanillaMultiMonitorFix.dll" , L"nampower.dll" };
		BOOL showMsgBox = FALSE;

		// We don't really care if sideloading failed.

		for (int i = 0; i < sizeof(dllNames) / sizeof(LPCWSTR); ++i) {
			HMODULE handle = LoadLibraryW(dllNames[i]);

			if (!handle) {
				dllNames[i] = NULL;
				continue;
			}

			showMsgBox = TRUE;

			typedef DWORD(*PLOAD)();
			PLOAD loadFunc = (PLOAD)GetProcAddress(handle, "Load");
			if (loadFunc) {
				loadFunc();
			}
		}

		alreadyLoaded = TRUE;

		if (showMsgBox) {
			WCHAR nameBuffer[512];
			int writingAt = 0;
			for (int i = 0; i < sizeof(dllNames) / sizeof(LPCWSTR); ++i) {
				if (dllNames[i]) {
					int wrote = swprintf_s(&nameBuffer[writingAt], sizeof(nameBuffer) / sizeof(WCHAR) - writingAt, L"%ls\n", dllNames[i]);

					if (wrote == -1) {
						continue;
					}

					writingAt += wrote;
				}
			}

			WCHAR msgBuffer[1024];
			int wrote = swprintf_s(msgBuffer, sizeof(msgBuffer) / sizeof(WCHAR), L"These following mods have been loaded into game and executed:\n\n%ls\nPlease beware that mods are powerful tool and use them at your own risk. Applying arbitrary mods onto Azeroth could lead into Burning Legion invasion or crucial information damage/leak.", nameBuffer);

			if (wrote != -1) {
				MessageBoxW(NULL, msgBuffer, L"Vanilla DLL sideloader", MB_SYSTEMMODAL | MB_OK | MB_ICONINFORMATION);
			}
		}

	}

	MH_DisableHook(&GetVersion);

	return pOriginalGetVersion();
}

BOOL APIENTRY DllMain(HMODULE hModule,
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
		if (MH_CreateHook(&GetVersion, &DetouredGetVersion, reinterpret_cast<LPVOID*>(&pOriginalGetVersion)) != MH_OK) {
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

// This function is needed for PE import directory entity
extern "C" __declspec(dllexport) void functionDoingNothing() {
	return;
}

