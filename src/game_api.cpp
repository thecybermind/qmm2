/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include "game_api.h"

// externs for each game's functions/structs
GEN_EXTS(Q3A);
GEN_EXTS(RTCWMP);
GEN_EXTS(JK2);
GEN_EXTS(JA);
GEN_EXTS(RTCWSP);
GEN_EXTS(RTCWET);

// extern for each game's qvm syscall handler
GEN_VMEXT(Q3A);
GEN_VMEXT(JK2);

// add your game's info data here
supportedgame_t g_supportedgames[] = {

	//	mod filename			qvm mod filename	default moddir	full gamename									msgs/short name		qvm handler
	#ifdef WIN32
		{ "qagamex86.dll",		"vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								GEN_INFO(Q3A),		&Q3A_vmsyscall },
		{ "qagame_mp_x86.dll",	nullptr,			"Main",			"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr        },
		{ "qagamex86.dll",		nullptr,			".",			"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr        },
		{ "qagame_mp_x86.dll",	nullptr,			"etmain",		"Return to Castle Wolfenstein: Enemy Territory",GEN_INFO(RTCWET),	nullptr        },
		{ "jk2mpgamex86.dll",	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast",					GEN_INFO(JK2),		&JK2_vmsyscall },
		{ "jampgamex86.dll",	nullptr,			"base",			"Jedi Knight: Jedi Academy",					GEN_INFO(JA),		nullptr        },
	#else
		{ "qagamei386.so",		"vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								GEN_INFO(Q3A),		&Q3A_vmsyscall },
		{ "qagame.mp.i386.so",	nullptr,			"main",			"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr        },
		{ "qagamei386.so",		nullptr,			"main",			"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr        },
		{ "qagame.mp.i386.so",	nullptr,			"etmain",		"Return to Castle Wolfenstein: Enemy Territory",GEN_INFO(RTCWET),	nullptr        },
		{ "jk2mpgamei386.so",	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast",					GEN_INFO(JK2),		&JK2_vmsyscall },
		{ "jampgamei386.so",	nullptr,			"base",			"Jedi Knight: Jedi Academy",					GEN_INFO(JA),		nullptr        },
	#endif
		{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
};
