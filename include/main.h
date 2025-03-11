/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_MAIN_H__
#define __QMM2_MAIN_H__

#include <string>
#include "CModMgr.h"

// store all currently-loaded game info
typedef struct game_info_s {
	eng_syscall_t pfnsyscall = nullptr;
	supportedgame_t* game = nullptr;
	std::string qmm_path;
	std::string qmm_dir;
	std::string qmm_file;
	std::string moddir;
	bool isautodetected = false;
} game_info_t;

extern game_info_t g_gameinfo;
extern CModMgr* g_ModMgr;

#define QMM_ENG_MSG	(g_gameinfo.game->qmm_eng_msgs)
#define QMM_MOD_MSG	(g_gameinfo.game->qmm_mod_msgs)

#define ENG_MSGNAME	(g_gameinfo.game->eng_msg_names)
#define MOD_MSGNAME	(g_gameinfo.game->mod_msg_names)

#define ENG_SYSCALL	g_gameinfo.pfnsyscall
#define MOD_VMMAIN	g_ModMgr->Mod()->vmMain

// these are used if we couldn't determine a game engine and we have to fail
// G_ERROR and GAME_SHUTDOWN appear to be 1 in all supported games
// we hope it is true for all
constexpr int QMM_FAIL_G_ERROR = 1;
constexpr int QMM_FAIL_GAME_SHUTDOWN = 1;

C_DLLEXPORT void dllEntry(eng_syscall_t);
C_DLLEXPORT int vmMain(int, int, int, int, int, int, int, int, int, int, int, int, int);
int syscall(int, ...);

#endif // __QMM2_MAIN_H__
