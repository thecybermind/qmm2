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
supportedgame_t g_supportedgames[] = {
    // mod filename		    qvm mod filename	default moddir	full gamename									exe hints               msgs/short name		entry procs		    #syscall#vmmain

    // vmMain games
    { "qagame" SUF_DLL,	    "vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								{ "q3", "quake3" },     GEN_INFO(Q3A),		GEN_DLLQVM(Q3A),    13,		4 },
    { "qagame" SUF_DLL,	    "vm/qagame.qvm",	"baseef",		"Star Trek Voyager: Elite Force (Holomatch)",	{ "stvoy" },            GEN_INFO(STVOYHM),	GEN_DLLQVM(STVOYHM),13,		3 },
    { "qagame" SUF_DLL,	    nullptr,			".",			"Return to Castle Wolfenstein (SP)",			{ "sp" },               GEN_INFO(RTCWSP),	GEN_DLL(RTCWSP),    13,		5 },
    { "jampgame" SUF_DLL,	nullptr,			"base",			"Jedi Knight: Jedi Academy (MP)",				{ "ja" },               GEN_INFO(JAMP),		GEN_DLL(JAMP),      13,		6 },
    { "jk2mpgame" SUF_DLL,	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast (MP)",				{ "jk2" },              GEN_INFO(JK2MP),	GEN_DLLQVM(JK2MP),	13,		3 },
    { "sof2mp_game" SUF_DLL,"vm/sof2mp_game.qvm","base/mp",		"Soldier of Fortune 2: Double Helix (MP)",		{ "mp" },               GEN_INFO(SOF2MP),	GEN_DLLQVM(SOF2MP),	13,		6 },
// COD, RTCWMP & WET filename changes on linux
#if defined(_WIN32)
    { "qagame_mp_" SUF_DLL, nullptr,			"etmain",		"Wolfenstein: Enemy Territory",					{ "et" },               GEN_INFO(WET),		GEN_DLL(WET),       13,		5 },
    { "qagame_mp_" SUF_DLL, nullptr,			"main",			"Return to Castle Wolfenstein (MP)",			{ "mp" },               GEN_INFO(RTCWMP),	GEN_DLL(RTCWMP),	13,		5 },
    { "game_mp_" SUF_DLL,   nullptr,			"Main",			"Call of Duty (MP)",							{ "codmp", "cod_" },    GEN_INFO(CODMP),	GEN_DLL(CODMP),		8,		4 },
    { "uo_game_mp_" SUF_DLL,nullptr,			"uo",			"Call of Duty: United Offensive (MP)",			{ "coduo" },            GEN_INFO(CODUOMP),	GEN_DLL(CODUOMP),	8,		4 },
    // allow a user to choose "COD11MP" manually if they are playing an old version of CoD (fake exe hints so it won't auto-detect)
    { "game_mp_" SUF_DLL,   nullptr,			"Main",			"Call of Duty v1.1 (MP)",						{ "zzz_no_auto" },      GEN_INFO(COD11MP),	GEN_DLL(COD11MP),	8,		4 },
#elif defined(__linux__)
    { "qagame.mp." SUF_DLL, nullptr,			"etmain",		"Wolfenstein: Enemy Territory",					{ "et" },               GEN_INFO(WET),		GEN_DLL(WET),		13,		5 },
    { "qagame.mp." SUF_DLL,	nullptr,			"main",			"Return to Castle Wolfenstein (MP)",			{ "mp" },               GEN_INFO(RTCWMP),	GEN_DLL(RTCWMP),	13,		5 },
    { "game.mp." SUF_DLL,	nullptr,			"Main",			"Call of Duty (MP)",							{ "codmp", "cod_" },    GEN_INFO(CODMP),	GEN_DLL(CODMP),		8,		4 },
    { "game.mp.uo." SUF_DLL,nullptr,			"uo",			"Call of Duty: United Offensive (MP)",			{ "coduo" },            GEN_INFO(CODUOMP),	GEN_DLL(CODUOMP),	8,		4 },
    // allow a user to choose "COD11MP" manually if they are playing an old version of CoD (fake exe hints so it won't auto-detect)
    { "game.mp." SUF_DLL,   nullptr,			"Main",			"Call of Duty v1.1 (MP)",						{ "zzz_no_auto" },      GEN_INFO(COD11MP),	GEN_DLL(COD11MP),	8,		4 },
#endif
// don't include CoD single players in linux
/*
#if defined(_WIN32)
	{ "game" SUF_DLL,	    nullptr,			".",			"Call of Duty (SP)",							{ "cod" },              GEN_INFO(CODSP),	GEN_DLL(CODSP),		-1,		-1 },
	{ "uo_game" SUF_DLL,    nullptr,			".",			"Call of Duty: United Offensive (SP)",			{ "coduo" },            GEN_INFO(CODUOSP),	GEN_DLL(CODUOSP),	-1,		-1 },
#endif
*/

    // GetGameAPI games
    { "game" SUF_DLL,	    nullptr,			"baseq2",		"Quake 2",										{ "q2", "quake2" },     GEN_INFO(QUAKE2),	GEN_GGA(QUAKE2),	7,		3 },
    { "game" SUF_DLL,	    nullptr,			"base",			"Star Trek: Elite Force II",					{ "ef" },               GEN_INFO(STEF2),	GEN_GGA(STEF2),	    17,		4 },
    { "game" SUF_DLL,	    nullptr,			"base",			"SiN",											{ "sin" },              GEN_INFO(SIN),		GEN_GGA(SIN),	    10,		3 },
    { "game" SUF_DLL,	    nullptr,			".",			"Soldier of Fortune 2: Double Helix (SP)",		{ "sof2" },             GEN_INFO(SOF2SP),	GEN_GGA(SOF2SP),    -1,		-1 },
    { "jk2game" SUF_DLL,	nullptr,			".",			"Jedi Knight 2: Jedi Outcast (SP)",				{ "jk2" },              GEN_INFO(JK2SP),	GEN_GGA(JK2SP),	    13,		9 },
    { "jagame" SUF_DLL,	    nullptr,			".",			"Jedi Knight: Jedi Academy (SP)",				{ "ja" },               GEN_INFO(JASP),		GEN_GGA(JASP),	    13,		9 },
    { "efgame" SUF_DLL,	    nullptr,			".",			"Star Trek Voyager: Elite Force (SP)",			{ "stvoy" },            GEN_INFO(STVOYSP),	GEN_GGA(STVOYSP),	13,		9 },
// OpenMOHAA adds 64-bit MoH support but the API is very different, so disable it for now
#if !defined(_WIN64) && !defined(__LP64__)
    { "game" SUF_DLL,	    nullptr,			"main",			"Medal of Honor: Allied Assault",				{ "mohaa" },            GEN_INFO(MOHAA),	GEN_GGA(MOHAA), 	9,		7 },
    { "game" SUF_DLL,	    nullptr,			"mainta",		"Medal of Honor: Spearhead",					{ "spear" },            GEN_INFO(MOHSH),	GEN_GGA(MOHSH),	    9,		7 },
    { "game" SUF_DLL,	    nullptr,			"maintt",		"Medal of Honor: Breakthrough",					{ "break" },            GEN_INFO(MOHBT),	GEN_GGA(MOHBT),	    9,		7 },
#endif
// Q2R only exists for 64-bit Windows (and no dedicated server)
#if defined(_WIN64)
    { "game_x64",		    nullptr,			"baseq2",		"Quake 2 Remastered",							{ "quake2ex" },         GEN_INFO(Q2R),		GEN_GGA(Q2R),	    9,		6 },
#endif

    { nullptr, }
};
