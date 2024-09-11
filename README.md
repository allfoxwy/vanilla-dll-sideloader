# vanilla-dll-sideloader

This is a DLL loader program for WoW Vanilla. 

To load a customized mod, you could name the mod with "Vanilla1121mod" in its name:
- For example `my-mod.Vanilla1121mod.dll`.

Loader would search ***current working directory*** for mods.


This loader is distinguished because it is NOT based on memory injection.

It is intented to be paired use with [add-dll-to-exe](https://github.com/allfoxwy/add-dll-to-exe). Then when WoW.exe start, this DLL would detour GetVersion() Win32 API.

The game would call GetVersion() very early in its bootstrap. In my observation, GetVersion() is even earlier than graphic or audio initialization. And game is calling it from main thread.

Also loader would detour game's EnterWorld event. When the first time it occurs, loader would try calling "FirstEnterWorld" procedure from each loaded mods. It's OK for mod doesn't provide a "FirstEnterWorld". Game's LUA environment is initialized and ready when EnterWorld.


