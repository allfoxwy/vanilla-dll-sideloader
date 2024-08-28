# vanilla-dll-sideloader

This DLL would attempt loading [Vanilla Fixes](https://github.com/hannesmann/vanillafixes), [SuperWoW](https://github.com/balakethelock/SuperWoW), [nampower](https://github.com/namreeb/nampower) and [VanillaMultiMonitorFix](https://github.com/Mates1500/VanillaMultiMonitorFix) when WoW Vanilla game start.

This loader is distinguished because it is NOT based on memory injection.

It is intented to be paired use with [add-dll-to-exe](https://github.com/allfoxwy/add-dll-to-exe). So that when WoW.exe start, this DLL would detour GetVersion() Win32 API.

The game would call GetVersion() very early in its bootstrap. In my observation, GetVersion() is even earlier than graphic or audio initialization. And game is calling it from main thread.

I think it's a perfect timing to sideload additional memory mods because game's memory is constructed but not yet execute much instruction.

GetVersion() itself is a trivial function so detour it should NOT causing harm.


