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
GEN_EXTS(STVOYMP); 
GEN_EXTS(Q3A);
GEN_EXTS(WET);
GEN_EXTS(RTCWMP);
GEN_EXTS(JK2MP);
GEN_EXTS(JAMP);

// extern for each game's qvm syscall handler
GEN_VMEXT(STVOYMP); 
GEN_VMEXT(Q3A);
GEN_VMEXT(JK2MP);

#ifdef QMM_GETGAMEAPI_SUPPORT
// externs for each game's functions/structs
GEN_EXTS(MOHAA);
GEN_EXTS(STVOYSP);
GEN_EXTS(STEF2);

// extern for each game's GetGameAPI call
GEN_APIEXT(MOHAA);
GEN_APIEXT(STVOYSP);
GEN_APIEXT(STEF2);
#endif // QMM_GETGAMEAPI_SUPPORT

// add your game's info data here
supportedgame_t g_supportedgames[] = {
	// mod filename			qvm mod filename	default moddir		full gamename									msgs/short name		qvm handler			api handler			syscall vmmain
#ifdef _WIN32

	{ "qagamex86.dll",		nullptr,			".",				"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,			nullptr,			13,		5 },
	{ "qagamex86.dll",		"vm/qagame.qvm",	"baseEF",			"Star Trek Voyager: Elite Force (Multiplayer)",	GEN_INFO(STVOYMP),	STVOYMP_vmsyscall,	nullptr,			13,		3 },
	{ "qagamex86.dll",		"vm/qagame.qvm",	"baseq3",			"Quake 3 Arena",								GEN_INFO(Q3A),		Q3A_vmsyscall,		nullptr,			13,		4 },
	{ "qagame_mp_x86.dll",	nullptr,			"etmain",			"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		nullptr,			nullptr,			13,		5 },
	{ "qagame_mp_x86.dll",	nullptr,			"Main",				"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,			nullptr,			13,		5 },
	{ "jk2mpgamex86.dll",	"vm/jk2mpgame.qvm",	"base",				"Jedi Knight 2: Jedi Outcast (Multiplayer)",	GEN_INFO(JK2MP),	JK2MP_vmsyscall,	nullptr,			13,		3 },
	{ "jampgamex86.dll",	nullptr,			"base",				"Jedi Knight: Jedi Academy (Multiplayer)",		GEN_INFO(JAMP),		nullptr,			nullptr,			13,		6 },
#ifdef QMM_GETGAMEAPI_SUPPORT
	{ "game.x86.dll",		nullptr,			".",				"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,			MOHAA_GetGameAPI,	9,		7 },
	{ "efgamex86.dll",		nullptr,			"baseEF",			"Star Trek Voyager: Elite Force (Singleplayer)",GEN_INFO(STVOYSP),	nullptr,			STVOYSP_GetGameAPI,	7,		9 },
	{ "gamex86.dll",		nullptr,			"Elite Force II",	"Star Trek: Elite Force II",					GEN_INFO(STEF2),	nullptr,			STEF2_GetGameAPI,	17,		4 },
#endif // QMM_GETGAMEAPI_SUPPORT

#elif defined(__linux__)
	{ "qagamei386.so",		nullptr,			"main",				"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,			nullptr,			13,		5 },
	{ "qagamei386.so",		"vm/qagame.qvm",	"baseEF",			"Star Trek Voyager: Elite Force (Multiplayer)",	GEN_INFO(STVOYMP),	STVOYMP_vmsyscall,	nullptr,			13,		3 },
	{ "qagamei386.so",		"vm/qagame.qvm",	"baseq3",			"Quake 3 Arena",								GEN_INFO(Q3A),		Q3A_vmsyscall,		nullptr,			13,		4 },
	{ "qagame.mp.i386.so",	nullptr,			"etmain",			"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		nullptr,			nullptr,			13,		5 },
	{ "qagame.mp.i386.so",	nullptr,			"main",				"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,			nullptr,			13,		5 },
	{ "jk2mpgamei386.so",	"vm/jk2mpgame.qvm",	"base",				"Jedi Knight 2: Jedi Outcast (Multiplayer)",	GEN_INFO(JK2MP),	JK2MP_vmsyscall,	nullptr,			13,		3 },
	{ "jampgamei386.so",	nullptr,			"base",				"Jedi Knight: Jedi Academy (Multiplayer)",		GEN_INFO(JAMP),		nullptr,			nullptr,			13,		6 },
#ifdef QMM_GETGAMEAPI_SUPPORT
	{ "game.i386.so",		nullptr,			".",				"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,			MOHAA_GetGameAPI,	7,		7 },
	{ "efgamei386.dll",		nullptr,			"baseEF",			"Star Trek Voyager: Elite Force (Singleplayer)",GEN_INFO(STVOYSP),	nullptr,			STVOYSP_GetGameAPI,	7,		9 },
	{ "ef2gamei386.so",		nullptr,			"Elite Force II",	"Star Trek: Elite Force II",					GEN_INFO(STEF2),	nullptr,			STEF2_GetGameAPI,	17,		4 },

#endif // QMM_GETGAMEAPI_SUPPORT

#else // !_WIN32 && !__linux__

#endif
	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0 }
};
