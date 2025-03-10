# QMM2
Q3 MultiMod 2  
QMM2 - Q3 MultiMod 2  
Copyright 2025  
https://github.com/thecybermind/qmm2/  
3-clause BSD license: https://opensource.org/license/bsd-3-clause  

Created By: Kevin Masterson < cybermind@gmail.com >

---

**QMM** is a system for games based on the Quake 3 engine. It functions similar to [Metamod](http://metamod.org/) for Half-Life and [Metamod:Source](https://www.sourcemm.net/) for the Source engine (Half-Life 2).

It hooks communication between the server engine and the the game logic (the mod). It allows for plugins to be loaded in-between which can add or change functionality without having to change the mod itself.


## Installation

#### Getting Started
The basic concept to operation is for QMM to be loaded as if it were the mod dll, and then it loads the original mod dll file.

> **Quake 3 and Jedi Knight 2 users**:  
> In the event your mod uses a QVM (Quake Virtual Machine) file (the default mods do), you must set the `vm_game` cvar to `0` in order for the QMM dll to be loaded. You can do this by adding the following to `<mod>/autoexec.cfg` (create it if it does not exist):  
> `seta vm_game 0`

> **Listen server users**:  
> You can get rid of the DLL "Security Warning" message by setting the `com_blindlyLoadDLLs` cvar to `1`. You can do this by adding the following to `<mod>/autoexec.cfg` (create it if it does not exist):  
> `seta com_blindlyLoadDLLs 1`

Each game uses a different filename for the mod. Refer to this list when you are asked to rename the QMM DLL in step 2:

| Game | Windows DLL | Linux SO | QVM |
| ----------- | ----------- | ----------- | ----------- |
| Quake 3 Arena | qagamex86.dll | qagamei386.so | vm/qagame.qvm |
| Return to Castle Wolfenstein (Multiplayer) | qagame_mp_x86.dll | qagame.mp.i386.so |   |
| Return to Castle Wolfenstein: Enemy Territory | qagame_mp_x86.dll | qagame.mp.i386.so |   |
| Return to Castle Wolfenstein (Singleplayer | qagamex86.dll | qagamei386.so |   |
| Jedi Knight 2 | jk2mpgamex86.dll | jk2mpgamei386.so | vm/jk2mpgame.qvm |
| Jedi Academy | jampgamex86.dll | jampgamei386.so |  |

#### Insallation Steps:
1. Locate the mod file (listed above) in the mod directory and rename it to `qmm_<name>`
2. Place `qmm2.dll`/`qmm2.so` into the mod directory and rename it to the old mod file name (listed above)
3. Place `qmm2.json` into the mod directory
4. Configure `qmm2.json` (see the file for more details)
