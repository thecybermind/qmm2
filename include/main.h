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
#include "CPluginMgr.h"

// store all currently-loaded game info
typedef struct game_info_s {
	eng_syscall_t pfnsyscall = nullptr;
	supported_game_t* game = nullptr;
	std::string qmm_path;
	std::string qmm_dir;
	std::string qmm_file;
	std::string moddir;
	bool isautodetected = false;
} game_info_t;

extern game_info_t g_GameInfo;

extern CModMgr* g_ModMgr;
extern CPluginMgr* g_PluginMgr;

#endif //__QMM2_MAIN_H__
