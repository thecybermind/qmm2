/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "osdef.h"
#include "game_api.h"

// cache some dynamic message values that get evaluated a lot
intptr_t msg_G_PRINT, msg_GAME_INIT, msg_GAME_CONSOLE_COMMAND, msg_GAME_SHUTDOWN;

// externs for each game's functions/structs
GEN_EXTS(COD11MP);
GEN_EXTS(CODMP);
GEN_EXTS(CODUOMP);
GEN_EXTS(JAMP);
GEN_EXTS(JK2MP);
GEN_EXTS(Q3A);
GEN_EXTS(RTCWMP);
GEN_EXTS(RTCWSP);
GEN_EXTS(SOF2MP);
GEN_EXTS(STVOYHM);
GEN_EXTS(WET);

GEN_EXTS(JASP);
GEN_EXTS(JK2SP);
GEN_EXTS(MOHAA);
GEN_EXTS(MOHSH);
GEN_EXTS(MOHBT);
GEN_EXTS(Q2R);
GEN_EXTS(QUAKE2);
GEN_EXTS(SIN);
GEN_EXTS(SOF2SP);
GEN_EXTS(STEF2);
GEN_EXTS(STVOYSP);

// add your game's info data here
supportedgame g_supportedgames[] = {
    // mod filename		    qvm mod filename	default moddir	full gamename									short name/funcs	#syscall#vmmain

    // vmMain games
	{ "qagame" MOD_DLL,		"vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								GEN_INFO(Q3A),		13,		4 },
	{ "qagame" MOD_DLL,		"vm/qagame.qvm",	"baseef",		"Star Trek Voyager: Elite Force (Holomatch)",	GEN_INFO(STVOYHM),	13,		3 },
	{ "qagame" MOD_DLL,		nullptr,			".",			"Return to Castle Wolfenstein (SP)",			GEN_INFO(RTCWSP),	13,		5 },
	{ "jk2mpgame" MOD_DLL,	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast (MP)",				GEN_INFO(JK2MP),	13,		3 },
	{ "sof2mp_game" MOD_DLL,"vm/sof2mp_game.qvm","base/mp",		"Soldier of Fortune 2: Double Helix (MP)",		GEN_INFO(SOF2MP),	13,		6 },
	{ "jampgame" MOD_DLL,	nullptr,			"base",			"Jedi Knight: Jedi Academy (MP)",				GEN_INFO(JAMP),		13,		6 },
	// COD, RTCWMP & WET filename changes on linux
#if defined(_WIN32)
	{ "qagame_mp_" MOD_DLL,	nullptr,			"etmain",		"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		13,		5 },
	{ "qagame_mp_" MOD_DLL,	nullptr,			"main",			"Return to Castle Wolfenstein (MP)",			GEN_INFO(RTCWMP),	13,		5 },
	{ "game_mp_" MOD_DLL,	nullptr,			"Main",			"Call of Duty (MP)",							GEN_INFO(CODMP),	8,		4 },
	{ "uo_game_mp_" MOD_DLL,nullptr,			"uo",			"Call of Duty: United Offensive (MP)",			GEN_INFO(CODUOMP),	8,		4 },
	// allow a user to choose "COD11MP" manually if they are playing an old version of CoD (no auto-detection)
	{ "game_mp_" MOD_DLL,	nullptr,			"Main",			"Call of Duty v1.1 (MP)",						GEN_INFO(COD11MP),	8,		4 },
#elif defined(__linux__)
	{ "qagame.mp." MOD_DLL,	nullptr,			"etmain",		"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		13,		5 },
	{ "qagame.mp." MOD_DLL,	nullptr,			"main",			"Return to Castle Wolfenstein (MP)",			GEN_INFO(RTCWMP),	13,		5 },
	{ "game.mp." MOD_DLL,	nullptr,			"Main",			"Call of Duty (MP)",							GEN_INFO(CODMP),	8,		4 },
	{ "game.mp.uo." MOD_DLL,nullptr,			"uo",			"Call of Duty: United Offensive (MP)",			GEN_INFO(CODUOMP),	8,		4 },
	// allow a user to choose "COD11MP" manually if they are playing an old version of CoD (no auto-detection)
	{ "game.mp." MOD_DLL,	nullptr,			"Main",			"Call of Duty v1.1 (MP)",						GEN_INFO(COD11MP),	8,		4 },
#endif

	// GetGameAPI games
	{ "game" MOD_DLL,		nullptr,			"baseq2",		"Quake 2",										GEN_INFO(QUAKE2),	7,		3 },
	{ "game" MOD_DLL,		nullptr,			"base",			"Star Trek: Elite Force II",					GEN_INFO(STEF2),	17,		4 },
	{ "game" MOD_DLL,		nullptr,			"base",			"SiN",											GEN_INFO(SIN),		10,		3 },
	{ "game" MOD_DLL,		nullptr,			".",			"Soldier of Fortune 2: Double Helix (SP)",		GEN_INFO(SOF2SP),	-1,		-1 },
// OpenMOHAA adds 64-bit MoH support but the API is very different, so disable it for now
#if !defined(_WIN64) && !defined(__LP64__)
	{ "game" MOD_DLL,		nullptr,			"main",			"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	9,		7 },
	{ "game" MOD_DLL,		nullptr,			"mainta",		"Medal of Honor: Spearhead",					GEN_INFO(MOHSH),	9,		7 },
	{ "game" MOD_DLL,		nullptr,			"maintt",		"Medal of Honor: Breakthrough",					GEN_INFO(MOHBT),	9,		7 },
#endif
	{ "jk2game" MOD_DLL,	nullptr,			".",			"Jedi Knight 2: Jedi Outcast (SP)",				GEN_INFO(JK2SP),	13,		9},
	{ "jagame" MOD_DLL,		nullptr,			".",			"Jedi Knight: Jedi Academy (SP)",				GEN_INFO(JASP),		13,		9},
	{ "efgame" MOD_DLL,		nullptr,			".",			"Star Trek Voyager: Elite Force (SP)",			GEN_INFO(STVOYSP),	13,		9 },
	// Q2R only exists for 64-bit Windows (and no dedicated server)
#if defined(_WIN64)
	{ "game_x64.dll",		nullptr,			"baseq2",		"Quake 2 Remastered",							GEN_INFO(Q2R),		9,		6 },
#endif

	{ nullptr, }
};
