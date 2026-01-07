/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_MAIN_H__
#define __QMM2_MAIN_H__

#include <string>
#include "qmmapi.h"
#include "game_api.h"

// store pointers for GetGameAPI-based systems
struct api_info_t {
    void* orig_import = nullptr;			// original import struct from engine
    void* orig_export = nullptr;			// original export struct from mod
    void* qmm_import = nullptr;				// import struct that is hooked by QMM and given to the mod
    void* qmm_export = nullptr;				// export struct that is hooked by QMM and given to the engine
};

// store all currently-loaded game & game engine info
struct game_info_t {
    eng_syscall_t pfnsyscall = nullptr;		// game-specific wrapper for syscall. given to plugins and called by QMM
    mod_vmMain_t pfnvmMain = nullptr;		// game-specific wrapper for vmMain. given to plugins and called by QMM
    supportedgame_t* game = nullptr;		// loaded engine from supported games table from game_api.cpp
    bool isautodetected = false;			// was this engine auto-detected?
    std::string exe_path;					// full path of running server binary
    std::string exe_dir;					// directory of running server binary
    std::string exe_file;					// filename of running server binary
    std::string qmm_path;					// full path of qmm dll
    std::string qmm_dir;					// directory of qmm dll
    std::string qmm_file;					// filename of qmm dll
    void* qmm_module_ptr = nullptr;			// qmm module pointer
    std::string moddir;						// active mod dir
    std::string cfg_path;					// qmm config file path
    api_info_t api_info;					// some pointers utilized by GetGameAPI games
};

extern game_info_t g_gameinfo;

#define QMM_ENG_MSG	(g_gameinfo.game->qmm_eng_msgs)
#define QMM_MOD_MSG	(g_gameinfo.game->qmm_mod_msgs)

#define ENG_SYSCALL	g_gameinfo.pfnsyscall

// this is used if we couldn't determine a game engine and we have to fail.
// G_ERROR appears to be 1 in all supported dllEntry games. they are different in some GetGameAPI games,
// but for those we just return nullptr from GetGameAPI
constexpr int QMM_FAIL_G_ERROR = 1;
// set to true if a G_ERROR has been triggered, to avoid calling it again from GAME_SHUTDOWN or its ilk (SOF2MP's GAME_GHOUL2_SHUTDOWN, etc)
extern bool g_shutdown;

// flag that is set by GEN_EXPORT macro before calling into vmMain. used to tell if this is a call that
// should be directly routed to the mod or not in some single player games that have game & cgame in the
// same DLL
extern bool cgame_is_QMM_vmMain_call;
// engine->mod entry point
C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...);
// renamed syscall to qmm_syscall to avoid conflict with POSIX "long syscall(long number, ...)" which has pretty much the same interface
intptr_t qmm_syscall(intptr_t cmd, ...);

// get a given argument with G_ARGV, based on game engine type
void qmm_argv(intptr_t argn, char* buf, intptr_t buflen);

#endif // __QMM2_MAIN_H__
