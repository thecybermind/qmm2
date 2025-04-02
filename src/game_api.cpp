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
GEN_EXTS(RTCWSP);
GEN_EXTS(STVOYHM); 
GEN_EXTS(Q3A);
GEN_EXTS(WET);
GEN_EXTS(RTCWMP);
GEN_EXTS(JK2MP);
GEN_EXTS(JAMP);

// extern for each game's qvm syscall handler
GEN_VMEXT(STVOYHM); 
GEN_VMEXT(Q3A);
GEN_VMEXT(JK2MP);

// externs for each game's functions/structs
GEN_EXTS(MOHAA);
GEN_EXTS(MOHSH);
GEN_EXTS(MOHBT);
GEN_EXTS(STEF2);

// extern for each game's GetGameAPI call
GEN_APIEXT(MOHAA);
GEN_APIEXT(MOHSH);
GEN_APIEXT(MOHBT);
GEN_APIEXT(STEF2);

// add your game's info data here
supportedgame_t g_supportedgames[] = {
	// mod filename			qvm mod filename	default moddir		full gamename									msgs/short name		qvm handler			GetGameAPI handler	syscall vmmain	exe hints
#ifdef _WIN32

	{ "qagamex86.dll",		nullptr,			".",				"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,			nullptr,			13,		5,		{ "sp" } },
	{ "qagamex86.dll",		"vm/qagame.qvm",	"baseef",			"Star Trek Voyager: Elite Force (Holomatch)",	GEN_INFO(STVOYHM),	STVOYHM_vmsyscall,	nullptr,			13,		3,		{ "stvoy" } },
	{ "qagamex86.dll",		"vm/qagame.qvm",	"baseq3",			"Quake 3 Arena",								GEN_INFO(Q3A),		Q3A_vmsyscall,		nullptr,			13,		4,		{ "q3ded", "quake3" } },
	{ "qagame_mp_x86.dll",	nullptr,			"etmain",			"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		nullptr,			nullptr,			13,		5,		{ "et" } },
	{ "qagame_mp_x86.dll",	nullptr,			"main",				"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,			nullptr,			13,		5,		{ "mp" } },
	{ "jk2mpgamex86.dll",	"vm/jk2mpgame.qvm",	"base",				"Jedi Knight 2: Jedi Outcast (Multiplayer)",	GEN_INFO(JK2MP),	JK2MP_vmsyscall,	nullptr,			13,		3,		{ "jk2" } },
	{ "jampgamex86.dll",	nullptr,			"base",				"Jedi Knight: Jedi Academy (Multiplayer)",		GEN_INFO(JAMP),		nullptr,			nullptr,			13,		6,		{ "ja" } },
	{ "gamex86.dll",		nullptr,			"main",				"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,			MOHAA_GetGameAPI,	9,		7,		{ "mohaa" }},
	{ "gamex86.dll",		nullptr,			"mainta",			"Medal of Honor: Spearhead",					GEN_INFO(MOHSH),	nullptr,			MOHSH_GetGameAPI,	9,		7,		{ "spear" }},
	//{ "gamex86.dll",		nullptr,			"maintt",			"Medal of Honor: Breakthrough",					GEN_INFO(MOHBT),	nullptr,			MOHBT_GetGameAPI,	9,		7,		{ "break" }},
	{ "gamex86.dll",		nullptr,			"base",				"Star Trek: Elite Force II",					GEN_INFO(STEF2),	nullptr,			STEF2_GetGameAPI,	17,		4,		{ "ef" } },

#elif defined(__linux__)

	{ "qagamei386.so",		nullptr,			".",				"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,			nullptr,			13,		5,		{ "sp" } },
	{ "qagamei386.so",		"vm/qagame.qvm",	"baseef",			"Star Trek Voyager: Elite Force (Holomatch)",	GEN_INFO(STVOYHM),	STVOYHM_vmsyscall,	nullptr,			13,		3,		{ "stvoy" } },
	{ "qagamei386.so",		"vm/qagame.qvm",	"baseq3",			"Quake 3 Arena",								GEN_INFO(Q3A),		Q3A_vmsyscall,		nullptr,			13,		4,		{ "q3ded", "quake3" } },
	{ "qagame.mp.i386.so",	nullptr,			"etmain",			"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		nullptr,			nullptr,			13,		5,		{ "et" } },
	{ "qagame.mp.i386.so",	nullptr,			"main",				"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,			nullptr,			13,		5,		{ "mp" } },
	{ "jk2mpgamei386.so",	"vm/jk2mpgame.qvm",	"base",				"Jedi Knight 2: Jedi Outcast (Multiplayer)",	GEN_INFO(JK2MP),	JK2MP_vmsyscall,	nullptr,			13,		3,		{ "jk2" } },
	{ "jampgamei386.so",	nullptr,			"base",				"Jedi Knight: Jedi Academy (Multiplayer)",		GEN_INFO(JAMP),		nullptr,			nullptr,			13,		6,		{ "ja" } },
	{ "gamei386.so",		nullptr,			"main",				"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,			MOHAA_GetGameAPI,	9,		7,		{ "mohaa" } },
	{ "gamei386.so",		nullptr,			"mainta",			"Medal of Honor: Spearhead",					GEN_INFO(MOHSH),	nullptr,			MOHSH_GetGameAPI,	9,		7,		{ "spear" }},
	//{ "gamei386.so",		nullptr,			"maintt",			"Medal of Honor: Breakthrough",					GEN_INFO(MOHBT),	nullptr,			MOHBT_GetGameAPI,	9,		7,		{ "break" }},
	{ "gamei386.so",		nullptr,			"base",				"Star Trek: Elite Force II",					GEN_INFO(STEF2),	nullptr,			STEF2_GetGameAPI,	17,		4,		{ "ef" } },

#else // !_WIN32 && !__linux__

#endif
	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, {} }
};
