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

// these are used if we couldn't determine a game engine and we have to fail.
// G_ERROR and GAME_SHUTDOWN appear to be 1 in all supported dllEntry games.
// they are different in GetGameAPI games, but returning nullptr from GetGameAPI causes the engine to exit anyway
constexpr int QMM_FAIL_G_ERROR = 1;
constexpr int QMM_FAIL_GAME_SHUTDOWN = 1;

C_DLLEXPORT void dllEntry(eng_syscall_t syscall);	// initial entry point for non-GetGameAPI games
C_DLLEXPORT void* GetGameAPI(void* import);			// initial entry point for GetGameAPI games
C_DLLEXPORT int vmMain(int cmd, ...);
int syscall(int cmd, ...);

void main_detect_env();
void main_load_config();
constexpr bool QMM_DETECT_GETGAMEAPI = true;
constexpr bool QMM_DETECT_DLLENTRY = false;
void main_detect_game(std::string cfg_game, bool is_GetGameAPI_mode = QMM_DETECT_DLLENTRY);
bool main_load_mod(std::string cfg_mod);
void main_load_plugin(std::string plugin_path);

#endif // __QMM2_MAIN_H__
