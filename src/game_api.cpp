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
GEN_EXTS(RTCWET);
GEN_EXTS(RTCWMP);
GEN_EXTS(JK2);
GEN_EXTS(JA);
#ifdef QMM_GETGAMEAPI_SUPPORT
GEN_EXTS(MOHAA);
GEN_EXTS(STVOYSP);
GEN_EXTS(STEF2);
#endif // QMM_GETGAMEAPI_SUPPORT

// extern for each game's qvm syscall handler
GEN_VMEXT(STVOYMP); 
GEN_VMEXT(Q3A);
GEN_VMEXT(JK2);

#ifdef QMM_GETGAMEAPI_SUPPORT
// extern for each game's GetGameAPI call
GEN_APIEXT(MOHAA);
GEN_APIEXT(STVOYSP);
GEN_APIEXT(STEF2);
#endif // QMM_GETGAMEAPI_SUPPORT

// add your game's info data here
supportedgame_t g_supportedgames[] = {

		// mod filename			qvm mod filename	default moddir	full gamename									msgs/short name		qvm handler			exe hint	api entry
#ifdef _WIN32
		{ "qagamex86.dll",		nullptr,			".",				"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,			"sp",		nullptr },
		{ "qagamex86.dll",		"vm/qagame.qvm",	"baseEF",			"Star Trek Voyager: Elite Force (Multiplayer)",	GEN_INFO(STVOYMP),	STVOYMP_vmsyscall,	"stvoy",	nullptr },
		{ "qagamex86.dll",		"vm/qagame.qvm",	"baseq3",			"Quake 3 Arena",								GEN_INFO(Q3A),		Q3A_vmsyscall,		nullptr,	nullptr },
		{ "qagame_mp_x86.dll",	nullptr,			"etmain",			"Return to Castle Wolfenstein: Enemy Territory",GEN_INFO(RTCWET),	nullptr,			"et",		nullptr },
		{ "qagame_mp_x86.dll",	nullptr,			"Main",				"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,			nullptr,	nullptr },
		{ "jk2mpgamex86.dll",	"vm/jk2mpgame.qvm",	"base",				"Jedi Knight 2: Jedi Outcast",					GEN_INFO(JK2),		JK2_vmsyscall,		nullptr,	nullptr },
		{ "jampgamex86.dll",	nullptr,			"base",				"Jedi Knight: Jedi Academy",					GEN_INFO(JA),		nullptr,			nullptr,	nullptr },
#ifdef QMM_GETGAMEAPI_SUPPORT
		{ "game.x86.dll",		nullptr,			".",				"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,			nullptr,	MOHAA_GetGameAPI },
		{ "efgamex86.dll",		nullptr,			"baseEF",			"Star Trek Voyager: Elite Force (Singleplayer)",GEN_INFO(STVOYSP),	nullptr,			nullptr,	STVOYSP_GetGameAPI },
		{ "gamex86.dll",		nullptr,			"Elite Force II",	"Star Trek: Elite Force II",					GEN_INFO(STEF2),	nullptr,			nullptr,	STEF2_GetGameAPI },
#endif // QMM_GETGAMEAPI_SUPPORT
#elif defined(__linux__)
		{ "qagamei386.so",		nullptr,			"main",				"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,			"sp",		nullptr },
		{ "qagamei386.so",		"vm/qagame.qvm",	"baseEF",			"Star Trek Voyager: Elite Force (Multiplayer)",	GEN_INFO(STVOYMP),	STVOYMP_vmsyscall,	"stvoy",	nullptr },
		{ "qagamei386.so",		"vm/qagame.qvm",	"baseq3",			"Quake 3 Arena",								GEN_INFO(Q3A),		Q3A_vmsyscall,		nullptr,	nullptr },
		{ "qagame.mp.i386.so",	nullptr,			"etmain",			"Return to Castle Wolfenstein: Enemy Territory",GEN_INFO(RTCWET),	nullptr,			"et",		nullptr },
		{ "qagame.mp.i386.so",	nullptr,			"main",				"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,			nullptr,	nullptr },
		{ "jk2mpgamei386.so",	"vm/jk2mpgame.qvm",	"base",				"Jedi Knight 2: Jedi Outcast",					GEN_INFO(JK2),		JK2_vmsyscall,		nullptr,	nullptr },
		{ "jampgamei386.so",	nullptr,			"base",				"Jedi Knight: Jedi Academy",					GEN_INFO(JA),		nullptr,			nullptr,	nullptr },
#ifdef QMM_GETGAMEAPI_SUPPORT
		{ "game.i386.so",		nullptr,			".",				"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,			nullptr,	MOHAA_GetGameAPI },
		{ "efgamei386.dll",		nullptr,			"baseEF",			"Star Trek Voyager: Elite Force (Singleplayer)",GEN_INFO(STVOYSP),	nullptr,			nullptr,	STVOYSP_GetGameAPI },
		{ "ef2gamei386.so",		nullptr,			"Elite Force II",	"Star Trek: Elite Force II",					GEN_INFO(STEF2),	nullptr,			nullptr,	STEF2_GetGameAPI },
#endif // QMM_GETGAMEAPI_SUPPORT

#else // !_WIN32 && !__linux__

#endif
		{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
};
