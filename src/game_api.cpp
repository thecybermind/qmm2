/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include <stdlib.h>
#include "game_api.h"

//add your game file's msg arrays here
GEN_EXTS(Q3A);
GEN_VMEXT(Q3A);
GEN_EXTS(RTCWMP);
GEN_EXTS(JK2);
GEN_VMEXT(JK2);
GEN_EXTS(JA);
GEN_EXTS(RTCWSP);
GEN_EXTS(RTCWET);

//add your game's info data here
supported_game_t g_SupportedGameList[] = {

//	mod filename			qvm mod filename	default moddir	full gamename									msgs/short name		qvm handler
#ifdef WIN32
	{ "qagamex86.dll",		"vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								GEN_INFO(Q3A),		&Q3A_vmsyscall },
	{ "jk2mpgamex86.dll",	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast",					GEN_INFO(JK2),		&JK2_vmsyscall },
	{ "jampgamex86.dll",	NULL,				"base",			"Jedi Knight: Jedi Academy",					GEN_INFO(JA),		NULL           },
	{ "qagame_mp_x86.dll",	NULL,				"Main",			"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	NULL           },
	{ "qagamex86.dll",		NULL,				".",			"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	NULL           },
	{ "qagame_mp_x86.dll",	NULL,				"etmain",		"Return to Castle Wolfenstein: Enemy Territory",GEN_INFO(RTCWET),	NULL           }
#else
	{ "qagamei386.so",		"vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								GEN_INFO(Q3A),		&Q3A_vmsyscall },
	{ "jk2mpgamei386.so",	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast",					GEN_INFO(JK2),		&JK2_vmsyscall },
	{ "jampgamei386.so",	NULL,				"base",			"Jedi Knight: Jedi Academy",					GEN_INFO(JA),		NULL           },
	{ "qagame.mp.i386.so",	NULL,				"main",			"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	NULL           },
	{ "qagamei386.so",		NULL,				"main",			"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	NULL           },
	{ "qagame.mp.i386.so",	NULL,				"etmain",		"Return to Castle Wolfenstein: Enemy Territory",GEN_INFO(RTCWET),	NULL           }
#endif
};
