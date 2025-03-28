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

***For the latest installation documentation, please see the [GitHub project wiki](https://github.com/thecybermind/qmm2/wiki).***

- [Installation](#installation)
    - [Getting Started](#getting-started)
    - [Installation Steps](#installation-steps)
- [Configuration](#configuration)
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
| Star Trek Voyager: Elite Force (Holomatch) | qagamex86.dll | qagamei386.so | vm/qagame.qvm |
| Star Trek: Elite Force II | gamex86.dll | gamei386.so |  |

### Installation Steps:
1. Locate the mod file (listed above) in the mod directory and rename it to `qmm_<name>`
2. Place `qmm2.dll`/`qmm2.so` into the mod directory and rename it to the old mod file name (listed above)
3. Place `qmm2.json` into the mod directory
4. Configure `qmm2.json` (see [Configuration](#configuration) section for details)

> **Note for Jedi Academy on Windows:** You will have to run a server without QMM first to extract the original `jampgamex86.dll` file from the .pk3 files, so that you can rename it to `qmm_jampgamex86.dll` in step 1. JA stores the .dll file inside .pk3s, but it must be outside a .pk3 in order for Windows to load it, so JA will read it from the .pk3 and write it to disk.  
> Also, for step 2, place the `zzz_qmm_jamp.pk3` file in the mod directory instead of the qmm2.dll. This is necessary because Quake 3-based games will load files from .pk3 files BEFORE real files, and the .pk3s are loaded in alphabetical order. This means that the `zzz_qmm_jamp.pk3` file will be loaded last, ensuring it has the jampgamex86.dll that gets loaded by the server.

## Configuration

The configuration for QMM primarily comes from the qmm2.json file. This should be placed in the mod directory, the same place the qmm DLL is located.

The QMM config contains several options:

- `game` - valid options:
    "auto" (default) = attempt to automatically determine engine
    "Q3A" = Quake 3 Arena
    "RTCWSP" = Return to Castle Wolfenstein (Singleplayer)
    "RTCWMP" = Return to Castle Wolfenstein (Multiplayer)
    "WET" = Wolfenstein: Enemy Territory
    "JK2MP" = Jedi Knight II: Jedi Outcast
    "JAMP" = Jedi Knight: Jedi Academy
    "STVOYHM" = Star Trek Voyager: Elite Force (Holomatch)
    "STEF2" = Star Trek: Elite Force II
 
- `mod` - path to mod file. specify "auto" to attempt to automatically determine mod filename based on the game engine. default = "auto"

- `qvmstacksize` - size (in MiB) of the QVM stack for QVM mods. default = 1

- `execcfg` - name of Quake config file to execute after QMM and plugins are loaded. specify an empty string to explicitly disable this function. default = "qmmexec.cfg"

- `plugins` - list of plugin filenames to load

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

QMM will not attempt to load a mod file if it thinks it is QMM itself.

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
