/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

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
GEN_EXTS(SOF2MP);
GEN_EXTS(COD11MP);
GEN_EXTS(CODMP);
GEN_EXTS(CODUOMP);

GEN_EXTS(MOHAA);
GEN_EXTS(MOHSH);
GEN_EXTS(MOHBT);
GEN_EXTS(STEF2);
GEN_EXTS(QUAKE2);
GEN_EXTS(Q2R);
GEN_EXTS(SIN);
GEN_EXTS(JK2SP);
GEN_EXTS(JASP);
GEN_EXTS(STVOYSP);

// add your game's info data here
supportedgame_t g_supportedgames[] = {
	// mod filename	suffix		qvm mod filename	default moddir	full gamename									msgs/short name		qvm handler			dllEntry proc		GetGameAPI proc		#syscall#vmmain	exe hints

	// vmMain games
	{ "qagame",		SUF_DLL,	"vm/qagame.qvm",	"baseq3",		"Quake 3 Arena",								GEN_INFO(Q3A),		Q3A_vmsyscall,		Q3A_dllEntry,		nullptr,			13,		4,		{ "q3", "quake3"} },
	{ "qagame",		SUF_DLL,	"vm/qagame.qvm",	"baseef",		"Star Trek Voyager: Elite Force (Holomatch)",	GEN_INFO(STVOYHM),	STVOYHM_vmsyscall,	STVOYHM_dllEntry,	nullptr,			13,		3,		{ "stvoy" } },
	{ "qagame",		SUF_DLL,	nullptr,			".",			"Return to Castle Wolfenstein (SP)",			GEN_INFO(RTCWSP),	nullptr,			RTCWSP_dllEntry,	nullptr,			13,		5,		{ "sp" } },
	{ "jampgame",	SUF_DLL,	nullptr,			"base",			"Jedi Knight: Jedi Academy (MP)",				GEN_INFO(JAMP),		nullptr,			JAMP_dllEntry,		nullptr,			13,		6,		{ "ja" } },
	{ "jk2mpgame",	SUF_DLL,	"vm/jk2mpgame.qvm",	"base",			"Jedi Knight 2: Jedi Outcast (MP)",				GEN_INFO(JK2MP),	JK2MP_vmsyscall,	JK2MP_dllEntry,		nullptr,			13,		3,		{ "jk2" } },
	{ "sof2mp_game",SUF_DLL,	"vm/sof2mp_game.qvm","base/mp",		"Soldier of Fortune 2: Double Helix (MP)",		GEN_INFO(SOF2MP),	SOF2MP_vmsyscall,	SOF2MP_dllEntry,	nullptr,			13,		6,		{ "mp" } },
// COD, RTCWMP & WET filename changes on linux
#if defined(_WIN32)
	{ "qagame_mp_",	SUF_DLL,	nullptr,			"etmain",		"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		nullptr,			WET_dllEntry,		nullptr,			13,		5,		{ "et" } },
	{ "qagame_mp_",	SUF_DLL,	nullptr,			"main",			"Return to Castle Wolfenstein (MP)",			GEN_INFO(RTCWMP),	nullptr,			RTCWMP_dllEntry,	nullptr,			13,		5,		{ "mp" } },
	{ "game_mp_",	SUF_DLL,	nullptr,			"Main",			"Call of Duty (MP)",							GEN_INFO(CODMP),	nullptr,			CODMP_dllEntry,		nullptr,			8,		4,		{ "codmp", "cod_"} },
	{ "uo_game_mp_",SUF_DLL,	nullptr,			"uo",			"Call of Duty: United Offensive (MP)",			GEN_INFO(CODUOMP),	nullptr,			CODUOMP_dllEntry,	nullptr,			8,		4,		{ "coduo" } },
	// allow a user to choose "COD11MP" manually if they are playing an old version of CoD (fake exe hints so it won't auto-detect)
	{ "game_mp_",	SUF_DLL,	nullptr,			"Main",			"Call of Duty v1.1 (MP)",						GEN_INFO(COD11MP),	nullptr,			COD11MP_dllEntry,	nullptr,			8,		4,		{ "zzz_no_auto" } },
#elif defined(__linux__)
	{ "qagame.mp.",	SUF_DLL,	nullptr,			"etmain",		"Wolfenstein: Enemy Territory",					GEN_INFO(WET),		nullptr,	 		WET_dllEntry,		nullptr,			13,		5,		{ "et" } },
	{ "qagame.mp.",	SUF_DLL,	nullptr,			"main",			"Return to Castle Wolfenstein (MP)",			GEN_INFO(RTCWMP),	nullptr,			RTCWMP_dllEntry,	nullptr,			13,		5,		{ "mp" } },
	{ "game.mp.",	SUF_DLL,	nullptr,			"Main",			"Call of Duty (MP)",							GEN_INFO(CODMP),	nullptr,			CODMP_dllEntry,		nullptr,			8,		4,		{ "codmp", "cod_" } },
	{ "game.mp.uo.",SUF_DLL,	nullptr,			"uo",			"Call of Duty: United Offensive (MP)",			GEN_INFO(CODUOMP),	nullptr,			CODUOMP_dllEntry,	nullptr,			8,		4,		{ "coduo" } },
	// allow a user to choose "COD11MP" manually if they are playing an old version of CoD (fake exe hints so it won't auto-detect)
	{ "game.mp.",	SUF_DLL,	nullptr,			"Main",			"Call of Duty v1.1 (MP)",						GEN_INFO(COD11MP),	nullptr,			COD11MP_dllEntry,	nullptr,			8,		4,		{ "zzz_no_auto" } },
#endif
// don't include CoD single players in linux
#if 0 // defined(_WIN32)
	{ "game",		SUF_DLL,	nullptr,			".",			"Call of Duty (SP)",							GEN_INFO(CODSP),	nullptr,			CODSP_dllEntry,		nullptr,			-1,		-1,		{ "cod" } },
	{ "uo_game",	SUF_DLL,	nullptr,			".",			"Call of Duty: United Offensive (SP)",			GEN_INFO(CODUOSP),	nullptr,			CODUOSP_dllEntry,	nullptr,			-1,		-1,		{ "coduo" } },
#endif

	// GetGameAPI games
	{ "game",		SUF_DLL,	nullptr,			"baseq2",		"Quake 2",										GEN_INFO(QUAKE2),	nullptr,			nullptr,			QUAKE2_GetGameAPI,	7,		3,		{ "q2", "quake2" } },
	{ "game",		SUF_DLL,	nullptr,			"base",			"Star Trek: Elite Force II",					GEN_INFO(STEF2),	nullptr,			nullptr,			STEF2_GetGameAPI,	17,		4,		{ "ef" } },
	{ "game",		SUF_DLL,	nullptr,			"base",			"SiN",											GEN_INFO(SIN),		nullptr,			nullptr,			SIN_GetGameAPI,		10,		3,		{ "sin" } },
	{ "jk2game",	SUF_DLL,	nullptr,			".",			"Jedi Knight 2: Jedi Outcast (SP)",				GEN_INFO(JK2SP),	nullptr,			nullptr,			JK2SP_GetGameAPI,	13,		9,		{ "jk2" } },
	{ "jagame",		SUF_DLL,	nullptr,			".",			"Jedi Knight: Jedi Academy (SP)",				GEN_INFO(JASP),		nullptr,			nullptr,			JASP_GetGameAPI,	13,		9,		{ "ja" } },
	{ "efgame",		SUF_DLL,	nullptr,			".",			"Star Trek Voyager: Elite Force (SP)",			GEN_INFO(STVOYSP),	nullptr,			nullptr,			STVOYSP_GetGameAPI,	13,		9,		{ "stvoy" } },
// OpenMOHAA adds 64-bit MoH support but the API is very different, so disable it for now
#if !defined(_WIN64) && !defined(__LP64__)
	{ "game",		SUF_DLL,	nullptr,			"main",			"Medal of Honor: Allied Assault",				GEN_INFO(MOHAA),	nullptr,			nullptr,			MOHAA_GetGameAPI,	9,		7,		{ "mohaa" } },
	{ "game",		SUF_DLL,	nullptr,			"mainta",		"Medal of Honor: Spearhead",					GEN_INFO(MOHSH),	nullptr,			nullptr,			MOHSH_GetGameAPI,	9,		7,		{ "spear" } },
	{ "game",		SUF_DLL,	nullptr,			"maintt",		"Medal of Honor: Breakthrough",					GEN_INFO(MOHBT),	nullptr,			nullptr,			MOHBT_GetGameAPI,	9,		7,		{ "break" } },
#endif
// Q2R only exists for 64-bit Windows (and no dedicated server)
#if defined(_WIN64)
	{ "game_",		"x64",		nullptr,			"baseq2",		"Quake 2 Remastered",							GEN_INFO(Q2R),		nullptr,			nullptr,			Q2R_GetGameAPI,		9,		6,		{ "quake2ex" } },
#endif

	{ nullptr, }
};
