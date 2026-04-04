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
#include "gameapi.hpp"

// Currently-loaded game & game engine info.
struct GameInfo {
    std::string exe_path;					// Full path of running server binary
    std::string exe_dir;					// Directory of running server binary
    std::string exe_file;					// Filename of running server binary
    std::string qmm_path;					// Full path of QMM dll
    std::string qmm_dir;					// Directory of QMM dll
    std::string qmm_file;					// Filename of QMM dll
    std::string mod_dir;					// Active mod dir
    std::string cfg_path;					// QMM config file path
    GameSupport* game = nullptr;			// :oaded engine from supported games table from game_api.cpp
    eng_syscall syscall = nullptr;			// syscall from dllEntry (if applicable) to call G_ERROR if needed
    void* qmm_module_ptr = nullptr;			// QMM module pointer
    bool is_auto_detected = false;			// Was this engine auto-detected?
    bool is_shutdown = false;				// Is the game shutting down due to G_ERROR? Used to avoid calling G_ERROR again from GAME_SHUTDOWN
    APIType api = QMM_API_ERROR;			// Engine api that QMM was loaded with

    /**
    * @brief Shared code for QMM initialization from all API entry points.
    *
    * @param import First argument to API entry point
    * @param extra Second argument to API entry point
    * @param engine APIType of engine API that was called
    * @return value to return back to the engine
    */
    void* HandleEntry(void* import, void* extra, APIType engine);

    /**
    * @brief Populate path/module/binary/environment/etc information.
    */
    void DetectEnv();

    /**
    * @brief Load config file into g_cfg.
    *
    * @param config_filename Filename of config file to load
    */
    void LoadConfig(std::string config_filename);

    /**
    * @brief Detect game engine that loaded QMM.
    *
    * @param cfg_game Value of "game" config option
    * @param engine APIType of engine API that loaded QMM
    * @return true if game was detected, false otherwise
    */
    bool DetectGame(std::string cfg_game, APIType engine);

    /**
    * @brief Load mod file.
    *
    * @param cfg_mod Value of "mod" config option
    * @return true if mod was loaded, false otherwise
    */
    bool LoadMod(std::string cfg_mod);

    /**
    * @brief Load plugin file.
    *
    * @param plugin_path Plugin filename from config file
    * @return true if plugin was loaded, false otherwise
    */
    bool LoadPlugin(std::string plugin_path);

    /**
    * @brief Route syscall or vmMain calls to plugins and destination.
    *
    * @param is_syscall true if the call is for syscall, false for vmMain
    * @param cmd Function value to send
    * @param args Function arguments to send
    * @return return value of call
    */
    intptr_t Route(bool is_syscall, intptr_t cmd, intptr_t* args) const;

    static intptr_t msg_G_PRINT;                // Value of G_PRINT for the detected game
    static intptr_t msg_GAME_INIT;              // Value of GAME_INIT for the detected game
    static intptr_t msg_GAME_CONSOLE_COMMAND;   // Value of GAME_CONSOLE_COMMAND for the detected game
    static intptr_t msg_GAME_SHUTDOWN;          // Value of GAME_SHUTDOWN for the detected game
};

extern GameInfo gameinfo;


// Convert from QMM_G_ message to actual G_ message
#define QMM_ENG_MSG					gameinfo.game->QMMEngMsg
// Convert from QMM_GAME_ message to actual GAME_ message
#define QMM_MOD_MSG					gameinfo.game->QMMModMsg

// Call game-specific syscall handler
#define ENG_SYSCALL					gameinfo.game->syscall
// Print string to game console
#define CONSOLE_PRINT(str)			ENG_SYSCALL(GameInfo::msg_G_PRINT, str)
// Print formatted string to game console
#define CONSOLE_PRINTF(str, ...)	ENG_SYSCALL(GameInfo::msg_G_PRINT, fmt::format(str, ## __VA_ARGS__).c_str())

// This is used if we couldn't determine a game engine and we have to fail.
// G_ERROR appears to be 1 in all supported dllEntry games.
// They are different in some GetGameAPI games, but for those we just return nullptr from GetGameAPI.
constexpr int QMM_FAIL_G_ERROR = 1;

// Store cgame passthrough stuff
struct CGameInfo {
    // Store syscall pointer to pass through to the mod's dllEntry function
    eng_syscall syscall;
    // Store mod's vmMain function to pass vmMain calls if is_from_QMM is false
    mod_vmMain vmMain;
    // This flag is set by the GEN_EXPORT macro(s) before calling into vmMain.
    // If true, the vmMain call is assumed to be from a lambda in QMM's game_export_t.
    // If false, the vmMain call is assumed to be directly from the engine to call into the cgame.
    bool is_from_QMM;
    // If true, GAME_SHUTDOWN has been called, but the mod DLL was kept loaded so cgame shutdown can run.
    bool is_shutdown;
};

extern CGameInfo cgameinfo;

#endif // QMM2_GAMEINFO_H
