#include "pch.h"

#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

#include "MinHook.h"

#pragma warning(disable: 4996)

using namespace std;
namespace fs = std::filesystem;

typedef DWORD(WINAPI* GETVERSION)();
typedef DWORD(*LOAD_FUNC)();

GETVERSION p_original_GetVersion = NULL;

// From https://stackoverflow.com/a/7154226
std::wstring utf8_to_utf16(const std::string& utf8)
{
    std::vector<unsigned long> unicode;
    size_t i = 0;
    while (i < utf8.size())
    {
        unsigned long uni;
        size_t todo;
        bool error = false;
        unsigned char ch = utf8[i++];
        if (ch <= 0x7F)
        {
            uni = ch;
            todo = 0;
        }
        else if (ch <= 0xBF)
        {
            throw std::logic_error("not a UTF-8 string");
        }
        else if (ch <= 0xDF)
        {
            uni = ch & 0x1F;
            todo = 1;
        }
        else if (ch <= 0xEF)
        {
            uni = ch & 0x0F;
            todo = 2;
        }
        else if (ch <= 0xF7)
        {
            uni = ch & 0x07;
            todo = 3;
        }
        else
        {
            throw std::logic_error("not a UTF-8 string");
        }
        for (size_t j = 0; j < todo; ++j)
        {
            if (i == utf8.size())
                throw std::logic_error("not a UTF-8 string");
            unsigned char ch = utf8[i++];
            if (ch < 0x80 || ch > 0xBF)
                throw std::logic_error("not a UTF-8 string");
            uni <<= 6;
            uni += ch & 0x3F;
        }
        if (uni >= 0xD800 && uni <= 0xDFFF)
            throw std::logic_error("not a UTF-8 string");
        if (uni > 0x10FFFF)
            throw std::logic_error("not a UTF-8 string");
        unicode.push_back(uni);
    }
    std::wstring utf16;
    for (size_t i = 0; i < unicode.size(); ++i)
    {
        unsigned long uni = unicode[i];
        if (uni <= 0xFFFF)
        {
            utf16 += (wchar_t)uni;
        }
        else
        {
            uni -= 0x10000;
            utf16 += (wchar_t)((uni >> 10) + 0xD800);
            utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
        }
    }
    return utf16;
}

DWORD WINAPI detoured_GetVersion()
{
	static bool already_loaded = false;

	if (!already_loaded) {
		already_loaded = true;

		vector<string> known_names = {
			u8"VfPatcher.dll",
			u8"SuperWoWhook.dll",
			u8"VanillaMultiMonitorFix.dll",
			u8"nampower.dll"
		};

		vector<fs::path> load_list;

		for (auto const& i : known_names) {
			auto p = fs::current_path() / fs::u8path(i);
			load_list.push_back(p);
		}

		for (auto const& dir_entry : fs::directory_iterator{ fs::current_path() }) {
			if (dir_entry.is_directory() == false) {
				auto p = dir_entry.path();
				auto name = p.stem().u8string();
				auto extension = p.extension().u8string();

				if (extension == u8".dll" && name.find(u8"WoWmod") != name.npos) {
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

					LOAD_FUNC load = (LOAD_FUNC)GetProcAddress(h, "Load");
					if (load != NULL) {
						load();
					}
				}
			}

			if (successful.size() > 0) {
				wstringstream message;

                message << utf8_to_utf16(u8"These following mods have been loaded into game and executed:") << endl << endl;
                for (auto const& mod : successful) {
                    message << utf8_to_utf16(mod.filename().u8string()) << endl;
                }
                message << endl << utf8_to_utf16(u8"Please beware that mods are powerful tool and use them at your own risk. Applying arbitrary mods onto Azeroth could lead into Burning Legion invasion or crucial information damage/leak.");

                MessageBoxW(NULL, message.str().data(), utf8_to_utf16(u8"Vanilla DLL mod sideloader").data(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
			}
		}
	}

	MH_DisableHook(&GetVersion);

	return p_original_GetVersion();
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
		if (MH_CreateHook(&GetVersion, &detoured_GetVersion, reinterpret_cast<LPVOID*>(&p_original_GetVersion)) != MH_OK) {
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
extern "C" __declspec(dllexport) DWORD function_doing_nothing() {
	return 0;
}

