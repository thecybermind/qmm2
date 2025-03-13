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
#include "qmmapi.h"
#include "game_api.h"

// store all currently-loaded game & game engine info
typedef struct game_info_s {
	eng_syscall_t pfnsyscall = nullptr;
	supportedgame_t* game = nullptr;
	bool isautodetected = false;
	std::string exe_path;
	std::string exe_dir;
	std::string exe_file;
	std::string qmm_path;
	std::string qmm_dir;
	std::string qmm_file;
	std::string moddir;
	std::string cfg_path;
} game_info_t;

extern game_info_t g_gameinfo;

#define QMM_ENG_MSG	(g_gameinfo.game->qmm_eng_msgs)
#define QMM_MOD_MSG	(g_gameinfo.game->qmm_mod_msgs)

#define ENG_SYSCALL	g_gameinfo.pfnsyscall

// these are used if we couldn't determine a game engine and we have to fail
// G_ERROR and GAME_SHUTDOWN appear to be 1 in all supported games
// we hope it is true for all
constexpr int QMM_FAIL_G_ERROR = 1;
constexpr int QMM_FAIL_GAME_SHUTDOWN = 1;

C_DLLEXPORT void dllEntry(eng_syscall_t syscall);
C_DLLEXPORT int vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11);
int syscall(int cmd, ...);

#endif // __QMM2_MAIN_H__
