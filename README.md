# QMM2
Q3 MultiMod 2  
QMM2 - Q3 MultiMod 2  
Copyright 2004-2026  
https://github.com/thecybermind/qmm2/  
3-clause BSD license: https://opensource.org/license/bsd-3-clause  

Created By: Kevin Masterson < k.m.masterson@gmail.com >

---

## About QMM

**QMM** is a server-side plugin manager for games based on the Quake 3 (and Quake 2!) engine. It functions similar to [Metamod](http://metamod.org/) for Half-Life and [Metamod:Source](https://www.sourcemm.net/) for the Source engine (Half-Life 2).

Formerly located at `q3mm.org`, `planetquake.com/qmm`, and `sourceforge.net/projects/qmm`.

## Documentation

For the latest installation documentation, please see the [Installation](https://github.com/thecybermind/qmm2/wiki/Installation) wiki page.

## How It Works

QMM hooks communication between the server engine and the game logic (the mod). It allows for plugins to be loaded in-between which can add or change functionality without having to change the mod itself.

QMM supports native DLL/SO mods as well as [QVM](https://github.com/thecybermind/qmm2/wiki/QVM) mods with a complete built-in bytecode virtual machine interpreter.

See the [How QMM Works](https://github.com/thecybermind/qmm2/wiki/How-QMM-works) wiki page for more information.

## Game Support

QMM supports the following games:

  - Call of Duty (MP)
  - Call of Duty: United Offensive (MP)
  - Call of Duty v1.1 (MP)
  - Medal of Honor: Allied Assault
  - Medal of Honor: Breakthrough
  - Medal of Honor: Spearhead
  - Quake 2
  - Quake 2 Remastered
  - Quake 3 Arena
  - Return to Castle Wolfenstein (MP)
  - Return to Castle Wolfenstein (SP)
  - SiN
  - Soldier of Fortune II: Double Helix (MP)
  - Soldier of Fortune II: Double Helix (SP)
  - Star Trek Voyager: Elite Force (SP)
  - Star Trek Voyager: Elite Force (Holomatch)
  - Star Trek: Elite Force II
  - Star Wars Jedi Knight: Jedi Academy (SP)
  - Star Wars Jedi Knight: Jedi Academy (MP)
  - Star Wars Jedi Knight II: Jedi Outcast (SP)
  - Star Wars Jedi Knight II: Jedi Outcast (MP)
  - Wolfenstein: Enemy Territory

See more info about these games on the [Game Support wiki page](https://github.com/thecybermind/qmm2/wiki/Game-support).

## Plugins

Check out some plugins at the [Plugin List](https://github.com/thecybermind/qmm2/wiki/Plugin-List) wiki page.

## Thanks

Designed by:
  - Kevin Masterson

Special thanks to:
  - Brian Stumm
  - loupgarou21
  - nevcairiel
  - BAILOPAN
  - Lumpy
  - para
  - I have forgotten many since 2004; please let me know!
		
QMM uses the following libraries:  
  - [nlohmann/json](https://github.com/nlohmann/json)
  - [aixlog](https://github.com/badaix/aixlog)
  - [fmtlib](https://github.com/fmtlib/fmt)

See the LIBLICENSES file for each of these libraries' licenses.