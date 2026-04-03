/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/


#define _CRT_SECURE_NO_WARNINGS
#include "version.h"
#include <cstdlib>      // atoi
#include <vector>
#include <string>
#include "log.hpp"
#include "format.hpp"
#include "config.hpp"
#include "gameinfo.hpp"
#include "game_api.hpp"
#include "qmmapi.h"
#include "plugin.hpp"
#include "mod.hpp"      // g_mod
#include "qvm.h"        // QVM_MAGIC
#include "util.hpp"

// Currently-loaded game & game engine info.
GameInfo gameinfo;

/* About cgame passthrough hack (not Quake 2 Remaster (Q2R), see comments before GetCGameAPI() for that):
   Some single player games, like Star Trek Voyager: Elite Force (STVOYSP), Jedi Knight 2 (JK2SP) and Jedi
   Academy (JASP), place the game (server side) and cgame (client side) in the same DLL. The game system
   uses GetGameAPI and the cgame system uses dllEntry/vmMain/syscall.

   Since we don't care about the cgame system, QMM will forward the dllEntry call to the mod (with the real
   cgame syscall pointer), and then forward all the incoming cgame vmMain calls directly to the mod's vmMain
   function.

   We do this with a few fields in the "cgameinfo" struct. First, the "cgameinfo.syscall" pointer variable
   is used to store the syscall pointer if dllEntry is called after QMM was already loaded from GetGameAPI.
   Then, dllEntry exits.

   Next, the "cgameinfo.is_from_QMM" bool is used to flag incoming calls to vmMain as coming from a
   game-specific GetGameAPI vmMain wrapper struct (i.e. qmm_export) meaning the call actually came from the
   game system (as opposed to cgame). This flag is set in the GEN_EXPORT macros and all of the custom static
   polyfill functions that route to vmMain. It is set back to false immediately after checking and handling
   passthrough calls.

   Next, when the GAME_INIT event comes through, and we load the actual mod DLL, we also check to see if
   "cgameinfo.syscall" is set. If it is, we look for "dllEntry" in the DLL, and pass "cgameinfo.syscall" to
   it. Next, we look for "vmMain" in the DLL and then store it in the "cgameinfo.vmMain" pointer.

   Whenever control enters vmMain and "cgameinfo.is_from_QMM" is false (meaning it was called directly by
   the engine for the cgame system), it routes the call to the mod's vmMain stored in "cgameinfo.vmMain".

   The final piece is that single player games shutdown and init the DLL a lot, particularly at every new
   level or between-level cutscene. When QMM detects that it is being shutdown and "cgameinfo.syscall" is
   set, it no longer unloads the mod DLL and sets "cgameinfo.is_shutdown" bool to true. Then, after a
   vmMain call is being handled as a passthrough, and "cgameinfo.is_shutdown" is true, it will unload the
   mod DLL. This allows the cgame system to shutdown properly.
*/

// Basic information about the cgame for games where the DLL has both game and cgame
CGameInfo cgameinfo = {
    nullptr,    // syscall
    nullptr,    // vmMain
    false,      // is_from_QMM
    false,      // is_shutdown
};


intptr_t GameInfo::msg_G_PRINT, GameInfo::msg_GAME_INIT, GameInfo::msg_GAME_CONSOLE_COMMAND, GameInfo::msg_GAME_SHUTDOWN;


void* GameInfo::HandleEntry(void* import, void* extra, APIType engine) {
    // return value is generally from the game-specific entry handler

    // in GetGameAPI+GetModuleAPI, the return value should be a pointer to QMM's export struct
    // in dllEntry, the return value is ignored by the engine

    // on returning nullptr:    
    // if GetGameAPI+GetModuleAPI, returning nullptr causes the engine to error out, so InitGame/vmMain will never be called
    // if dllEntry, QMM will check !gameinfo.game in vmMain(GAME_INIT) and call G_ERROR

    this->DetectEnv();

    this->api = engine;

    log_init(fmt::format("{}/qmm2.log", this->qmm_dir));

    QMMLOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ") (" << APIType_Function(engine) << ") loaded!\n";
    QMMLOG(QMM_LOG_INFO, "QMM") << "QMM path: \"" << this->qmm_path << "\"\n";
    QMMLOG(QMM_LOG_INFO, "QMM") << "Engine path: \"" << this->exe_path << "\"\n";
    QMMLOG(QMM_LOG_INFO, "QMM") << "Mod directory (?): \"" << this->mod_dir << "\"\n";

    // syscall, import, or apiversion are null/0
    if (!import) {
        QMMLOG(QMM_LOG_FATAL, "QMM") << APIType_Function(engine) << "(): engine pointer is NULL!\n";
        return nullptr;
    }

    // load config file. check command line arguments for a config filename
    this->LoadConfig(util_get_cmdline_arg("--qmm_config", "qmm2.json"));

    // update log severity from config file
    std::string cfg_loglevel = cfg_get_string(g_cfg, "loglevel", "");
    if (!cfg_loglevel.empty())
        log_set_severity(log_severity_from_name(cfg_loglevel));

    // detect game (possibly take setting from config file, or auto-detect)
    std::string cfg_game = cfg_get_string(g_cfg, "game", "auto");
    // check command line arguments for a game code
    cfg_game = util_get_cmdline_arg("--qmm_game", cfg_game);
    // failed to get engine information
    if (!this->DetectGame(cfg_game, engine) || !this->game) {
        QMMLOG(QMM_LOG_FATAL, "QMM") << APIType_Function(engine) << "(): Unable to determine game engine using \"" << cfg_game << "\"\n";
        return nullptr;
    }

    // now that the game is detected, cache some dynamic message values that get evaluated a lot
    GameInfo::msg_G_PRINT = this->game->QMMEngMsg(QMM_G_PRINT);
    GameInfo::msg_GAME_INIT = this->game->QMMModMsg(QMM_GAME_INIT);
    GameInfo::msg_GAME_CONSOLE_COMMAND = this->game->QMMModMsg(QMM_GAME_CONSOLE_COMMAND);
    GameInfo::msg_GAME_SHUTDOWN = this->game->QMMModMsg(QMM_GAME_SHUTDOWN);

    // call the game-specific entry handler (e.g. Q3A_GameSupport::Entry) which will set up the internals to interact
    // the engine and the mod
    void* ret = this->game->Entry(import, extra, engine);

    return ret;
}


void GameInfo::DetectEnv() {
    // save exe module path
    this->exe_path = path_normalize(util_get_proc_path());
    this->exe_dir = path_dirname(this->exe_path);
    this->exe_file = path_basename(this->exe_path);

    // save qmm module path
    this->qmm_path = path_normalize(util_get_qmm_path());
    this->qmm_dir = path_dirname(this->qmm_path);
    this->qmm_file = path_basename(this->qmm_path);

    // save qmm module pointer
    this->qmm_module_ptr = util_get_qmm_handle();

    // since we don't have the mod directory yet (can only officially get it using engine functions), we can
    // attempt to get the mod directory from the qmm path. if the qmm dir is the same as the exe dir, it's
    // likely that this is a singleplayer game, so just set the temporary moddir to ".".
    // 
    // this doesn't have to be exact, since it will only be used for config loading until the engine is
    // determined and we can actually ask for the mod directory in vmMain(GAME_INIT)
    if (str_striequal(this->qmm_dir, this->exe_dir)) {
        this->mod_dir = ".";
    }
    else {
        this->mod_dir = path_basename(this->qmm_dir);
    }

    // hack for OpenJK if the DLL is loaded from a pak file
    if (str_striequal(this->mod_dir, "temp")) {
        this->mod_dir = "base";
    }
}


void GameInfo::LoadConfig(std::string config_filename) {
    // load config file, try the following locations in order:
    // "<qmmdir>/qmm2.json"
    // "<exedir>/<moddir>/qmm2.json"
    std::string try_paths[] = {
        fmt::format("{}/{}", this->qmm_dir, config_filename),
        fmt::format("{}/{}/{}", this->exe_dir, this->mod_dir, config_filename),
    };
    for (std::string& try_path : try_paths) {
        try_path = path_normalize(try_path);
        if (try_path.empty() || !path_is_allowed(try_path))
            continue;
        g_cfg = cfg_load(try_path);
        if (!g_cfg.empty()) {
            this->cfg_path = try_path;
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Config file found! Path: \"" << this->cfg_path << "\"\n";
            return;
        }
    }

    // a default constructed json object is a blank {}, so in case of load failure, we can still try to read from it and assume defaults
    QMMLOG(QMM_LOG_WARNING, "QMM") << "GameInfo::LoadConfig(): Unable to load config file \"" << config_filename << "\", all settings will use default values\n";
}


bool GameInfo::DetectGame(std::string cfg_game, APIType engine) {
    if (cfg_game.empty())
        cfg_game = "auto";

    bool is_auto = str_striequal(cfg_game, "auto");

    // for (api_supportedgame& game : api_supportedgames) {
    for (GameSupport* gamesupport : api_supportedgames) {
        // if short name matches config option, we found it!
        if (!is_auto && str_striequal(cfg_game, gamesupport->GameCode())) {
            QMMLOG(QMM_LOG_INFO, "QMM") << "Found game match for config option \"" << cfg_game << "\"\n";
            this->game = gamesupport;
            this->is_auto_detected = false;
            // call the game's auto-detect function, since it may do some logic
            (void)gamesupport->AutoDetect(engine);
            return true;
        }
        // otherwise, if auto, call the game's auto-detect function
        else if (is_auto && gamesupport->AutoDetect(engine)) {
            QMMLOG(QMM_LOG_INFO, "QMM") << "Found game match with auto-detection - \"" << gamesupport->GameCode() << "\"\n";
            this->game = gamesupport;
            this->is_auto_detected = true;
            return true;
        }
    }

    return false;
}


bool GameInfo::LoadMod(std::string cfg_mod) {
    if (cfg_mod.empty())
        cfg_mod = "auto";

    QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to find mod using \"" << cfg_mod << "\"\n";
    // if "mod" config setting is an absolute path, just attempt to load it directly
    if (!str_striequal(cfg_mod, "auto") && path_is_absolute(cfg_mod)) {
        QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to load mod \"" << cfg_mod << "\"\n";
        return g_mod.Load(cfg_mod);
    }
    // if "mod" config setting is "auto", try the following locations in order:
    // "<qvmname>" (if the game engine supports it)
    // "<qmmdir>/qmm_<dllname>"
    // "<exedir>/<moddir>/qmm_<dllname>"

    // if "mod" config setting is a relative path, try the following locations in order:
    // "<mod>"
    // "<qmmdir>/<mod>"
    // "<exedir>/<moddir>/<mod>"
    else {
        std::vector<std::string> try_paths;
        // if "mod" config setting was "auto"
        if (str_striequal(cfg_mod, "auto")) {
            // treat as if "mod" config setting was "qmm_" plus the default dll name for this engine
            cfg_mod = fmt::format("qmm_{}", this->game->DefaultDLLName());
            // add QVM filename to search list if this game supports it
            if (this->game->DefaultQVMName())
                try_paths.push_back(this->game->DefaultQVMName());
        }
        // if "mod" config setting was a relative path, do nothing special unless QVM
        if (str_striequal(path_baseext(cfg_mod), EXT_QVM) && this->game->DefaultQVMName())
            try_paths.push_back(cfg_mod);
        try_paths.push_back(fmt::format("{}/{}", this->qmm_dir, cfg_mod));
        try_paths.push_back(fmt::format("{}/{}/{}", this->exe_dir, this->mod_dir, cfg_mod));
        for (std::string& try_path : try_paths) {
            try_path = path_normalize(try_path);
            if (try_path.empty() || !path_is_allowed(try_path))
                continue;
            QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to load mod \"" << try_path << "\"\n";
            if (g_mod.Load(try_path))
                return true;
        }
    }

    return false;
}


bool GameInfo::LoadPlugin(std::string plugin_path) {
    Plugin p;
    // absolute path, just attempt to load it directly
    if (path_is_absolute(plugin_path)) {
        // plugin_load returns 0 if no plugin file was found, 1 if success, and -1 if file was found but failure
        if (plugin_load(p, plugin_path) > 0) {
            g_plugins.push_back(p);
            return true;
        }
        return false;
    }
    // relative path, try the following locations in order:
    // "<qmmdir>/<plugin>"
    // "<exedir>/<moddir>/<plugin>"
    std::string try_paths[] = {
        fmt::format("{}/{}", this->qmm_dir, plugin_path),
        fmt::format("{}/{}/{}", this->exe_dir, this->mod_dir, plugin_path),
    };
    for (std::string& try_path : try_paths) {
        try_path = path_normalize(try_path);
        if (try_path.empty() || !path_is_allowed(try_path))
            continue;
        // plugin_load returns 0 if no plugin file was found, 1 if success, and -1 if file was found but failure
        int ret = plugin_load(p, try_path);
        if (ret > 0) {
            g_plugins.push_back(p);
            return true;
        }
        if (ret < 0)
            return false;
    }

    return false;
}


intptr_t GameInfo::Route(bool is_syscall, intptr_t cmd, intptr_t* args) const {
    const char* msg_name;
    const char* func_name;
    GameSupport* gamesupport = this->game;

    if (is_syscall) {
        msg_name = gamesupport->EngMsgName(cmd);
        func_name = "syscall";
    }
    else {
        msg_name = gamesupport->ModMsgName(cmd);
        func_name = "vmMain";
    }

    // store max result
    plugin_res max_result = QMM_UNUSED;
    // return values from a plugin call
    intptr_t plugin_ret = 0;
    // return value from real call
    intptr_t real_ret = 0;
    // return value to pass back to the caller (either real_ret, or a plugin_ret from QMM_OVERRIDE/QMM_SUPERCEDE result)
    intptr_t final_ret = 0;

    // store previous globals (in case of re-entrancy)
    plugin_globals old_globals = g_plugin_globals;

    // begin passing calls to plugins' pre-hook functions
    for (Plugin& p : g_plugins) {
        g_plugin_globals.plugin_result = QMM_UNUSED;
        // allow plugins to see the current final_ret value
        g_plugin_globals.final_return = final_ret;

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << p.plugininfo->name << "\" QMM_" << func_name << "( " << msg_name << "(" << cmd << ")) called\n";

        // call plugin's pre-hook and store return value
        if (is_syscall)
            plugin_ret = p.QMM_syscall(cmd, args);
        else
            plugin_ret = p.QMM_vmMain(cmd, args);

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << p.plugininfo->name << "\" QMM_" << func_name << "( " << msg_name << "(" << cmd << ")) returning " << plugin_ret << " with result " << plugin_result_to_str(g_plugin_globals.plugin_result) << "\n";

        // set new max result
        max_result = util_max(g_plugin_globals.plugin_result, max_result);
        // store current max result in global for plugins
        g_plugin_globals.high_result = max_result;
        // invalid/error result values
        if (g_plugin_globals.plugin_result == QMM_UNUSED) {
            QMMLOG(QMM_LOG_WARNING, "QMM") << func_name << "(" << msg_name << "): Plugin \"" << p.plugininfo->name << "\" did not set result flag\n";
        }
        else if (g_plugin_globals.plugin_result == QMM_ERROR) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << func_name << "(" << msg_name << "): Plugin \"" << p.plugininfo->name << "\" set result flag QMM_ERROR\n";
        }
        // if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
        else if (g_plugin_globals.plugin_result >= QMM_OVERRIDE) {
            final_ret = plugin_ret;
        }
    }

    // call real function (unless a plugin resulted in QMM_SUPERCEDE)
    if (max_result < QMM_SUPERCEDE) {
        QMMLOG(QMM_LOG_TRACE, "QMM") << "Real " << func_name << "(" << msg_name << "(" << cmd << ")) called\n";

        if (is_syscall)
            real_ret = gamesupport->syscall(cmd, QMM_PUT_SYSCALL_ARGS());
        else
            real_ret = gamesupport->vmMain(cmd, QMM_PUT_VMMAIN_ARGS());

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Real " << func_name << "(" << msg_name << "(" << cmd << ")) returning " << real_ret << "\n";
    }
    else {
        QMMLOG(QMM_LOG_TRACE, "QMM") << "Real " << func_name << "(" << msg_name << "(" << cmd << ")) superceded\n";
    }

    // store real_ret in global for plugins
    g_plugin_globals.orig_return = real_ret;

    // if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the real return value back to the mod
    if (max_result < QMM_OVERRIDE)
        final_ret = real_ret;

    // pass calls to plugins' post-hook functions (QMM_OVERRIDE or QMM_SUPERCEDE can still change final_ret)
    for (Plugin& p : g_plugins) {
        g_plugin_globals.plugin_result = QMM_UNUSED;
        // allow plugins to see the current final_ret value
        g_plugin_globals.final_return = final_ret;

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << p.plugininfo->name << "\" QMM_" << func_name << "_Post( " << msg_name << "(" << cmd << ")) called\n";

        // call plugin's post-hook and store return value
        if (is_syscall)
            plugin_ret = p.QMM_syscall_Post(cmd, args);
        else
            plugin_ret = p.QMM_vmMain_Post(cmd, args);

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << p.plugininfo->name << "\" QMM_" << func_name << "_Post( " << msg_name << "(" << cmd << ")) returning " << plugin_ret << " with result " << plugin_result_to_str(g_plugin_globals.plugin_result) << "\n";

        // ignore QMM_UNUSED so plugins can just use return, but still show a message for QMM_ERROR
        if (g_plugin_globals.plugin_result == QMM_ERROR) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << func_name << "(" << msg_name << "): Plugin \"" << p.plugininfo->name << "\" set result flag QMM_ERROR\n";
        }
        // if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
        else if (g_plugin_globals.plugin_result >= QMM_OVERRIDE) {
            final_ret = plugin_ret;
        }
    }

    // restore previous globals (stored in case of re-entrancy)
    g_plugin_globals = old_globals;

    return final_ret;
}
