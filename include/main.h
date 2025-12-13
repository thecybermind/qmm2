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

// store pointers for GetGameAPI-based systems
typedef struct api_info_s {
	void* orig_import = nullptr;			// original import struct from engine
	void* orig_export = nullptr;			// original export struct from mod
	void* qmm_import = nullptr;				// import struct that is hooked by QMM and given to the mod
	void* qmm_export = nullptr;				// export struct that is hooked by QMM and given to the engine
	mod_vmMain_t orig_vmmain = nullptr;		// pointer to wrapper vmMain function that calls actual mod func from orig_export
} api_info_t;

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
	void* qmm_module_ptr = nullptr;
	std::string moddir;
	std::string cfg_path;
	api_info_t api_info;
} game_info_t;

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
extern bool is_QMM_vmMain_call;
// engine->mod entry point
C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...);
// renamed syscall to qmm_syscall to avoid conflict with POSIX "long syscall(long number, ...)" which has pretty much the same interface
intptr_t qmm_syscall(intptr_t cmd, ...);

// get a given argument with G_ARGV, based on game engine type
void qmm_argv(intptr_t argn, char* buf, intptr_t buflen);

constexpr bool QMM_DETECT_GETGAMEAPI = true;
constexpr bool QMM_DETECT_DLLENTRY = false;

#endif // __QMM2_MAIN_H__
