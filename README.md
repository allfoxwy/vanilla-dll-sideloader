# vanilla-dll-sideloader

This is a DLL loader program for WoW Vanilla. 

To load a customized mod, you could add its name in `dlls.txt`. (Same as VanillaFixes)


This loader is distinguished because it is NOT based on memory injection.

It is intented to be paired use with [add-dll-to-exe](https://github.com/allfoxwy/add-dll-to-exe). When WoW.exe start, this DLL would detour GetVersion() Win32 API.

Game would call GetVersion() very early in its bootstrap. In my observation, GetVersion() is even earlier than graphic or audio initialization. And game is calling from its main thread.

Also loader would detour game's EnterWorld event. When the first time it occurs, loader would try calling "FirstEnterWorld" procedure from each loaded mods. It's OK for mod doesn't provide a "FirstEnterWorld". Game's LUA environment is initialized and ready when EnterWorld.


