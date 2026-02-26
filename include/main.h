/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_MAIN_H
#define QMM2_MAIN_H

#include <string>
#include "qmmapi.h"
#include "game_api.h"

// store all currently-loaded game & game engine info
struct gameinfo {
    std::string exe_path;					// full path of running server binary
    std::string exe_dir;					// directory of running server binary
    std::string exe_file;					// filename of running server binary
    std::string qmm_path;					// full path of qmm dll
    std::string qmm_dir;					// directory of qmm dll
    std::string qmm_file;					// filename of qmm dll
    std::string mod_dir;					// active mod dir
    std::string cfg_path;					// qmm config file path
    eng_syscall pfnsyscall = nullptr;		// game-specific wrapper for syscall. given to plugins and called by QMM
    mod_vmMain pfnvmMain = nullptr;			// game-specific wrapper for vmMain. given to plugins and called by QMM
    supportedgame* game = nullptr;			// loaded engine from supported games table from game_api.cpp
    void* qmm_module_ptr = nullptr;			// qmm module pointer
    bool isautodetected = false;			// was this engine auto-detected?
    bool isshutdown = false;				// is game shutting down due to G_ERROR? avoids calling G_ERROR again from GAME_SHUTDOWN
};
extern gameinfo g_gameinfo;

#define QMM_ENG_MSG	(g_gameinfo.game->qmm_eng_msgs)
#define QMM_MOD_MSG	(g_gameinfo.game->qmm_mod_msgs)

#define ENG_SYSCALL	(g_gameinfo.pfnsyscall)

// this is used if we couldn't determine a game engine and we have to fail.
// G_ERROR appears to be 1 in all supported dllEntry games. they are different in some GetGameAPI games,
// but for those we just return nullptr from GetGameAPI
constexpr int QMM_FAIL_G_ERROR = 1;

// store cgame passthrough stuff
struct cgameinfo {
    // store syscall pointer if we need to pass it through to the mod's dllEntry function for games with
    // combined game+cgame (singleplayer)
    eng_syscall syscall;
    // store mod's vmMain function for cgame passthrough
    mod_vmMain vmMain;
    // flag that is set by GEN_EXPORT macro before calling into vmMain. used to tell if this is a call that
    // should be directly routed to the mod or not in some single player games that have game & cgame in the
    // same DLL
    bool is_from_QMM;
    // GAME_SHUTDOWN has been called, but mod DLL was kept loaded so cgame shutdown can run
    bool shutdown;
};
extern cgameinfo cgame;

// engine->mod entry point
C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...);
// renamed syscall to qmm_syscall to avoid conflict with POSIX "long syscall(long number, ...)" which has pretty much the same interface
intptr_t qmm_syscall(intptr_t cmd, ...);

// get a given argument with G_ARGV, based on game engine type
void qmm_argv(intptr_t argn, char* buf, intptr_t buflen);

#endif // QMM2_MAIN_H
