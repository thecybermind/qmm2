/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include "osdef.h"
#include "game_api.h"

// externs for each game's functions/structs
GEN_EXTS(Q3A);
GEN_EXTS(STVOYHM);
GEN_EXTS(RTCWSP);
GEN_EXTS(WET);
GEN_EXTS(RTCWMP);
GEN_EXTS(JAMP);
GEN_EXTS(JK2MP);

GEN_EXTS(MOHAA);
GEN_EXTS(MOHSH);
GEN_EXTS(MOHBT);
GEN_EXTS(STEF2);
GEN_EXTS(QUAKE2);
GEN_EXTS(Q2R);

// add your game's info data here
supportedgame_t g_supportedgames[] = {
	// mod filename	suffix		qvm mod filename	default moddir		full gamename									msgs/short name		qvm handler			GetGameAPI handler	syscall vmmain	exe hints
	{ "qagame",		SUF_DLL,	"vm/qagame.qvm",	"baseq3",			"Quake 3 Arena",								GEN_INFO(Q3A),		Q3A_vmsyscall,		nullptr,			13,		4,		{ "q3", "quake3" } },
	{ "qagame",		SUF_DLL,	"vm/qagame.qvm",	"baseef",			"Star Trek Voyager: Elite Force (Holomatch)",	GEN_INFO(STVOYHM),	STVOYHM_vmsyscall,	nullptr,			13,		3,		{ "stvoy" } },
	{ "qagame",		SUF_DLL,	nullptr,			".",				"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,			nullptr,			13,		5,		{ "sp" } },
#if defined(_WIN32)
	{ "qagame_mp_",	SUF_DLL,	nullptr,			"etmain",			"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		nullptr,			nullptr,			13,		5,		{ "et" } },
	{ "qagame_mp_",	SUF_DLL,	nullptr,			"main",				"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,			nullptr,			13,		5,		{ "mp" } },
#elif defined(__linux__)
	{ "qagame.mp.",	SUF_DLL,	nullptr,			"etmain",			"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		nullptr,			nullptr,			13,		5,		{ "et" } },
	{ "qagame.mp.",	SUF_DLL,	nullptr,			"main",				"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,			nullptr,			13,		5,		{ "mp" } },
#endif
	{ "jampgame",	SUF_DLL,	nullptr,			"base",				"Jedi Knight: Jedi Academy (Multiplayer)",		GEN_INFO(JAMP),		nullptr,			nullptr,			13,		6,		{ "ja" } },
	{ "jk2mpgame",	SUF_DLL,	"vm/jk2mpgame.qvm",	"base",				"Jedi Knight 2: Jedi Outcast (Multiplayer)",	GEN_INFO(JK2MP),	JK2MP_vmsyscall,	nullptr,			13,		3,		{ "jk2" } },

	{ "game",		SUF_DLL,	nullptr,			"main",				"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,			MOHAA_GetGameAPI,	9,		7,		{ "mohaa" }},
	{ "game",		SUF_DLL,	nullptr,			"mainta",			"Medal of Honor: Spearhead",					GEN_INFO(MOHSH),	nullptr,			MOHSH_GetGameAPI,	9,		7,		{ "spear" }},
	{ "game",		SUF_DLL,	nullptr,			"maintt",			"Medal of Honor: Breakthrough",					GEN_INFO(MOHBT),	nullptr,			MOHBT_GetGameAPI,	9,		7,		{ "break" }},
	{ "game",		SUF_DLL,	nullptr,			"base",				"Star Trek: Elite Force II",					GEN_INFO(STEF2),	nullptr,			STEF2_GetGameAPI,	17,		4,		{ "ef" } },

#if defined(_WIN64)
	{ "game",		"_x64",		nullptr,			"baseq2",			"Quake 2 Remastered",							GEN_INFO(Q2R),		nullptr,			Q2R_GetGameAPI,		7,		3,		{ "quake2ex" } },
#else
	{ "game",		SUF_DLL,	nullptr,			"baseq2",			"Quake 2 Remastered",							GEN_INFO(Q2R),		nullptr,			Q2R_GetGameAPI,		7,		3,		{ "quake2ex" } },
#endif

	{ "game",		SUF_DLL,	nullptr,			"baseq2",			"Quake 2",										GEN_INFO(QUAKE2),	nullptr,			QUAKE2_GetGameAPI,	7,		3,		{ "q2", "quake2" } },

	{ nullptr, }
};
