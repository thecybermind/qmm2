/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAMEINFO_H
#define QMM2_GAMEINFO_H

#include <string>
#include "qmmapi.h"
#include "game_api.hpp"

// store all currently-loaded game & game engine info
struct GameInfo {
    std::string exe_path;					// full path of running server binary
    std::string exe_dir;					// directory of running server binary
    std::string exe_file;					// filename of running server binary
    std::string qmm_path;					// full path of qmm dll
    std::string qmm_dir;					// directory of qmm dll
    std::string qmm_file;					// filename of qmm dll
    std::string mod_dir;					// active mod dir
    std::string cfg_path;					// qmm config file path
    GameSupport* game = nullptr;			// loaded engine from supported games table from game_api.cpp
    eng_syscall syscall = nullptr;			// syscall from dllEntry (if applicable) to call G_ERROR if needed
    void* qmm_module_ptr = nullptr;			// qmm module pointer
    bool is_auto_detected = false;			// was this engine auto-detected?
    bool is_shutdown = false;				// is game shutting down due to G_ERROR? avoids calling G_ERROR again from GAME_SHUTDOWN
    APIType api = QMM_API_ERROR;			// engine api that QMM was loaded with

    // shared code for all API entry points
    void* HandleEntry(void* import, void* extra, APIType engine);
    // get path/module/binary/etc information
    void DetectEnv();
    // load config file
    void LoadConfig(std::string config_filename);
    // find what game engine loaded us
    bool DetectGame(std::string cfg_game, APIType engine);
    // find a mod file to load
    bool LoadMod(std::string cfg_mod);
    // find a plugin file to load
    bool LoadPlugin(std::string plugin_path);
    // route syscall or vmMain call to plugins and mod
    intptr_t Route(bool is_syscall, intptr_t cmd, intptr_t* args) const;

    // cache some dynamic message values that get evaluated a lot
    static intptr_t msg_G_PRINT, msg_GAME_INIT, msg_GAME_CONSOLE_COMMAND, msg_GAME_SHUTDOWN;
};
extern GameInfo gameinfo;

#define QMM_ENG_MSG					gameinfo.game->QMMEngMsg
#define QMM_MOD_MSG					gameinfo.game->QMMModMsg

#define ENG_SYSCALL					gameinfo.game->syscall
#define CONSOLE_PRINT(str)			ENG_SYSCALL(GameInfo::msg_G_PRINT, str)
#define CONSOLE_PRINTF(str, ...)	ENG_SYSCALL(GameInfo::msg_G_PRINT, fmt::format(str, ## __VA_ARGS__).c_str())

// this is used if we couldn't determine a game engine and we have to fail.
// G_ERROR appears to be 1 in all supported dllEntry games. they are different in some GetGameAPI games,
// but for those we just return nullptr from GetGameAPI
constexpr int QMM_FAIL_G_ERROR = 1;

// store cgame passthrough stuff
struct CGameInfo {
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
    bool is_shutdown;
};
extern CGameInfo cgameinfo;

#endif // QMM2_GAMEINFO_H
