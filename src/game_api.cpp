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
GEN_EXTS(Q3A);
GEN_EXTS(RTCWET);
GEN_EXTS(RTCWMP);
GEN_EXTS(JK2);
GEN_EXTS(JA);
#ifdef QMM_MOHAA_SUPPORT
GEN_EXTS(MOHAA);
#endif // QMM_MOHAA_SUPPORT

// extern for each game's qvm syscall handler
GEN_VMEXT(Q3A);
GEN_VMEXT(JK2);

#ifdef QMM_MOHAA_SUPPORT
// extern for each game's GetGameAPI call
GEN_APIEXT(MOHAA);
#endif // QMM_MOHAA_SUPPORT

// add your game's info data here
supportedgame_t g_supportedgames[] = {

		// mod filename			qvm mod filename	default moddir	full gamename									msgs/short name		qvm handler		exe hint	api entry
#ifdef _WIN32
		{ "qagamex86.dll",		nullptr,			".",			"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,		"sp",		nullptr },
		{ "qagamex86.dll",		"vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								GEN_INFO(Q3A),		&Q3A_vmsyscall, nullptr,	nullptr },
		{ "qagame_mp_x86.dll",	nullptr,			"etmain",		"Return to Castle Wolfenstein: Enemy Territory",GEN_INFO(RTCWET),	nullptr,		"et",		nullptr },
		{ "qagame_mp_x86.dll",	nullptr,			"Main",			"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,		nullptr,	nullptr },
		{ "jk2mpgamex86.dll",	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast",					GEN_INFO(JK2),		&JK2_vmsyscall,	nullptr,	nullptr },
		{ "jampgamex86.dll",	nullptr,			"base",			"Jedi Knight: Jedi Academy",					GEN_INFO(JA),		nullptr,		nullptr,	nullptr },
#ifdef QMM_MOHAA_SUPPORT
		{ "game.x86.dll",		nullptr,			".",			"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,		nullptr,	MOHAA_GetGameAPI },
#endif // QMM_MOHAA_SUPPORT
#elif defined(__linux__)
		{ "qagamei386.so",		nullptr,			"main",			"Return to Castle Wolfenstein (Singleplayer)",	GEN_INFO(RTCWSP),	nullptr,		"sp",		nullptr },
		{ "qagamei386.so",		"vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								GEN_INFO(Q3A),		&Q3A_vmsyscall, nullptr,	nullptr },
		{ "qagame.mp.i386.so",	nullptr,			"etmain",		"Return to Castle Wolfenstein: Enemy Territory",GEN_INFO(RTCWET),	nullptr,		"et",		nullptr },
		{ "qagame.mp.i386.so",	nullptr,			"main",			"Return to Castle Wolfenstein (Multiplayer)",	GEN_INFO(RTCWMP),	nullptr,		nullptr,	nullptr },
		{ "jk2mpgamei386.so",	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast",					GEN_INFO(JK2),		&JK2_vmsyscall, nullptr,	nullptr },
		{ "jampgamei386.so",	nullptr,			"base",			"Jedi Knight: Jedi Academy",					GEN_INFO(JA),		nullptr,		nullptr,	nullptr },
#ifdef QMM_MOHAA_SUPPORT
		{ "game.x86.so",		nullptr,			".",			"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,		nullptr,	MOHAA_GetGameAPI },
#endif // QMM_MOHAA_SUPPORT
#else // !_WIN32 && !__linux__

#endif
		{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
};
