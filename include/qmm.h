/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_QMM_H__
#define __QMM2_QMM_H__

#include "nlohmann/json.hpp"
#include "CModMgr.h"
#include "CPluginMgr.h"
#include "qmmapi.h"

pluginfuncs_t* get_pluginfuncs();

extern CModMgr* g_ModMgr;
extern CPluginMgr* g_PluginMgr;
extern nlohmann::json g_cfg;

#define ENG_MSG		(g_GameInfo.game->eng_msgs)
#define ENG_MSGNAME	(g_GameInfo.game->eng_msg_names)

#define MOD_MSG		(g_GameInfo.game->mod_msgs)
#define MOD_MSGNAME	(g_GameInfo.game->mod_msg_names)

#define ENG_SYSCALL	(g_GameInfo.pfnsyscall)
#define MOD_VMMAIN	g_ModMgr->Mod()->vmMain
#define	QMM_SYSCALL	(g_ModMgr->QMM_SysCall())

C_DLLEXPORT void dllEntry(eng_syscall_t);
C_DLLEXPORT int vmMain(int, int, int, int, int, int, int, int, int, int, int, int, int);
int QMM_syscall(int, ...);

#endif //__QMM2_QMM_H__
