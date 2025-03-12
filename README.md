# QMM2
Q3 MultiMod 2  
QMM2 - Q3 MultiMod 2  
Copyright 2025  
https://github.com/thecybermind/qmm2/  
3-clause BSD license: https://opensource.org/license/bsd-3-clause  

Created By: Kevin Masterson < cybermind@gmail.com >

---

**QMM** is a plugin manager for games based on the Quake 3 engine. It functions similar to [Metamod](http://metamod.org/) for Half-Life and [Metamod:Source](https://www.sourcemm.net/) for the Source engine (Half-Life 2).

It hooks communication between the server engine and the game logic (the mod). It allows for plugins to be loaded in-between which can add or change functionality without having to change the mod itself.

- [Installation](#installation)
    - [Getting Started](#getting-started)
    - [Installation Steps](#installation-steps)
- [Notes](#notes)
    - [File Locations](#file-locations)
	    - [Config file](#config-file)
	    - [Mod file](#mod-file)
	    - [Plugins](#plugins)
    - [Quake 3 and Jedi Knight 2](#quake-3-and-jedi-knight-2)
    - [Listen Servers](#listen-servers)


## Installation

### Getting Started
The basic concept to operation is for QMM to be loaded as if it were the mod dll, and then it loads the original mod dll file.

Each game uses a different filename for the mod. Refer to this list when you are asked to rename the QMM DLL in step 2:

| Game | Windows DLL | Linux SO | QVM |
| ----------- | ----------- | ----------- | ----------- |
| Quake 3 Arena | qagamex86.dll | qagamei386.so | vm/qagame.qvm |
| Return to Castle Wolfenstein (Multiplayer) | qagame_mp_x86.dll | qagame.mp.i386.so |   |
| Return to Castle Wolfenstein: Enemy Territory | qagame_mp_x86.dll | qagame.mp.i386.so |   |
| Return to Castle Wolfenstein (Singleplayer | qagamex86.dll | qagamei386.so |   |
| Jedi Knight 2 | jk2mpgamex86.dll | jk2mpgamei386.so | vm/jk2mpgame.qvm |
| Jedi Academy | jampgamex86.dll | jampgamei386.so |  |

### Installation Steps:
1. Locate the mod file (listed above) in the mod directory and rename it to `qmm_<name>`
2. Place `qmm2.dll`/`qmm2.so` into the mod directory and rename it to the old mod file name (listed above)
3. Place `qmm2.json` into the mod directory
4. Configure `qmm2.json` (see the file for more details)

## Notes

### File Locations

Quake 3 engine games will generally attempt to load game files from 2 locations:
 - home directory: from `<home>/.q3a/` (i.e. `C:\Users\user\.q3a\` or `/home/user/.q3a/`)
 - game directory: where the game/server .exe file is (i.e. `C:\Program Files\Quake 3 Arena\` or `/opt/q3server/`)

QMM will attempt to load files from various paths in order to best work with these multiple locations.

In the following lists, you will see these placeholders used:
- `<qmmdir>` - the directory where the QMM DLL is located
- `<exedir>` - the directory where the game executable is located
- `<moddir>` - the mod the game is running (i.e. "baseq3", "main", etc)
- `<mod>` - the "mod" setting in the config file
- `<qvmname>` - the default name of a QVM mod for the game engine
- `<dllname>` - the default name of a DLL mod for the game engine
- `<plugin>` - a plugin file string given in the "plugins" list in the config file

#### Config file
The config file will be loaded from the following locations in order:
1. `<qmmdir>/qmm2.json`
2. `<exedir>/<moddir>/qmm2.json`
2. `./<moddir>/qmm2.json`

#### Mod file
The mod file can be specified in the config either as an absolute or relative path, or as "auto" (the default).

If an absolute path is given, only that path is loaded.

If a relative path is given, the mod file will be loaded from the following locations in order:
1. `<mod>`
2. `<qmmdir>/<mod>`
3. `<exedir>/<moddir>/<mod>`
4. `./<moddir>/<mod>`

If "auto" is used, the mod file will loaded from the following locations in order:
1. `<qvmname>` (if the game engine supports QVM mods)
2. `<qmmdir>/qmm_<dllname>`
3. `<exedir>/<moddir>/qmm_<dllname>`
4. `<exedir>/<moddir>/<dllname>`
5. `./<moddir>/qmm_<dllname>`

If QMM is unable to load a mod after checking all the above locations, it will exit with an error.

QMM will not attempt to load a mod or plugin file if it thinks the path is the same as QMM itself.

#### Plugins
A plugin file can be specified in the config either as an absolute or relative path.

If an absolute path is given, only that path is loaded.

If a relative path is given, the plugin file will be loaded from the following locations in order:
1. `<qmmdir>/<plugin>`
2. `<exedir>/<moddir>/<plugin>`
3. `./<moddir>/<plugin>`

### Quake 3 and Jedi Knight 2 
In the event your mod uses a QVM (Quake Virtual Machine) file (the default mods do), you must set the `vm_game` cvar to `0` in order for the QMM dll to be loaded. You can do this by adding the following to `<mod>/autoexec.cfg` (create it if it does not exist):  
> `seta vm_game 0`

### Listen Servers
You can get rid of the DLL "Security Warning" message by setting the `com_blindlyLoadDLLs` cvar to `1`. You can do this by adding the following to `<mod>/autoexec.cfg` (create it if it does not exist):  
> `seta com_blindlyLoadDLLs 1`
