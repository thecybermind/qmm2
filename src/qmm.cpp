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
#include "qmm.hpp"
#include "gameapi.hpp"
#include "qmmapi.h"
#include "plugin.hpp"   // g_plugins
#include "mod.hpp"      // g_mod
#include "util.hpp"


namespace QMM {

    namespace CGame {
        /* About cgame passthrough hack (not Quake 2 Remaster (Q2R), see comments before GetCGameAPI() for that):
           Some single player games, like Star Trek Voyager: Elite Force (STVOYSP), Jedi Knight 2 (JK2SP) and Jedi
           Academy (JASP), place the game (server side) and cgame (client side) in the same DLL. The game system
           uses GetGameAPI and the cgame system uses dllEntry/vmMain/syscall.

           Since we don't care about the cgame system, QMM will forward the dllEntry call to the mod (with the real
           cgame syscall pointer), and then forward all the incoming cgame vmMain calls directly to the mod's vmMain
           function.

           We do this with a few fields in the "CGame" namespace. First, the "QMM::CGame::syscall" pointer
           variable is used to store the syscall pointer if dllEntry is called after QMM was already loaded from
           GetGameAPI. Then, dllEntry exits.

           Next, since all real game calls come through QMM::vmMain_args, we know that any call to vmMain while
           "QMM::CGame::syscall" is set means it should be passed to the cgame.

           Next, when the GAME_INIT event comes through, and we load the actual mod DLL, we also check to see if
           "QMM::CGame::syscall" is set. If it is, we look for "dllEntry" in the DLL, and pass "QMM::CGame::syscall"
           to it. Next, we look for "vmMain" in the DLL and then store it in the "QMM::CGame::vmMain" pointer.

           Whenever control enters vmMain and "QMM::CGame::syscall" is set (meaning it was called directly by
           the engine for the cgame system), it routes the call to the mod's vmMain stored in "QMM::CGame::vmMain".

           The final piece is that single player games shutdown and init the DLL a lot, particularly at every new
           level or between-level cutscene. When QMM detects that it is being shutdown and "QMM::CGame::syscall" is
           set, it no longer unloads the mod DLL and sets "QMM::CGame::is_shutdown" bool to true. Then, after a
           vmMain call is being handled as a passthrough, and "QMM::CGame::is_shutdown" is true, it will unload the
           mod DLL. This allows the cgame system to shutdown properly.
        */
        //

        eng_syscall syscall = nullptr;
        mod_vmMain vmMain = nullptr;
        bool is_shutdown = false;
    }

    std::string exe_path;
    std::string exe_dir;
    std::string exe_file;
    std::string qmm_path;
    std::string qmm_dir;
    std::string qmm_file;
    std::string mod_dir;
    std::string cfg_path;
    GameSupport* game;
    eng_syscall syscall;
    void* qmm_module_ptr;
    bool is_auto_detected;
    bool is_shutdown;
    APIType api;

    intptr_t msg_G_PRINT;
    intptr_t msg_GAME_INIT;
    intptr_t msg_GAME_CONSOLE_COMMAND;
    intptr_t msg_GAME_SHUTDOWN;


    void* HandleEntry(void* import, void* extra, APIType engine) {
        // return value is generally from the game-specific entry handler

        // in GetGameAPI+GetModuleAPI, the return value should be a pointer to QMM's export struct
        // in dllEntry, the return value is ignored by the engine

        // on returning nullptr:    
        // if GetGameAPI+GetModuleAPI, returning nullptr causes the engine to error out, so InitGame/vmMain will never be called
        // if dllEntry, QMM will check !QMM::game in vmMain(GAME_INIT) and call G_ERROR

        DetectEnv();

        api = engine;

        Log::log_init(fmt::format("{}/qmm2.log", qmm_dir));

        QMMLOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " [" QMM_OS " " QMM_ARCH " (" QMM_BUILD ")] (" << APIType_Function(engine) << ") loaded!\n";
        QMMLOG(QMM_LOG_INFO, "QMM") << "QMM path: \"" << qmm_path << "\"\n";
        QMMLOG(QMM_LOG_INFO, "QMM") << "Engine path: \"" << exe_path << "\"\n";
        QMMLOG(QMM_LOG_INFO, "QMM") << "Mod directory (?): \"" << mod_dir << "\"\n";

        // syscall, import, or apiversion are null/0
        if (!import) {
            QMMLOG(QMM_LOG_FATAL, "QMM") << APIType_Function(engine) << "(): engine pointer is NULL!\n";
            return nullptr;
        }

        // load config file. check command line arguments for a config filename
        LoadConfig(Util::util_get_cmdline_arg("--qmm_config", "qmm2.json"));

        // update log severity from config file
        std::string cfg_loglevel = Config::cfg_get_string(g_cfg, "loglevel", "");
        if (!cfg_loglevel.empty())
            Log::log_set_severity(Log::log_severity_from_name(cfg_loglevel));

        // detect game (possibly take setting from config file, or auto-detect)
        std::string cfg_game = Config::cfg_get_string(g_cfg, "game", "auto");
        // check command line arguments for a game code
        cfg_game = Util::util_get_cmdline_arg("--qmm_game", cfg_game);
        // failed to get engine information
        if (!DetectGame(cfg_game, engine) || !game) {
            QMMLOG(QMM_LOG_FATAL, "QMM") << APIType_Function(engine) << "(): Unable to determine game engine using \"" << cfg_game << "\"\n";
            return nullptr;
        }

        // now that the game is detected, cache some dynamic message values that get evaluated a lot
        msg_G_PRINT = game->QMMEngMsg(QMM_G_PRINT);
        msg_GAME_INIT = game->QMMModMsg(QMM_GAME_INIT);
        msg_GAME_CONSOLE_COMMAND = game->QMMModMsg(QMM_GAME_CONSOLE_COMMAND);
        msg_GAME_SHUTDOWN = game->QMMModMsg(QMM_GAME_SHUTDOWN);

        // call the game-specific entry handler (e.g. Q3A_GameSupport::Entry) which will set up the internals to interact
        // the engine and the mod
        void* ret = game->Entry(import, extra, engine);

        return ret;
    }


    void DetectEnv() {
        // save exe module path
        exe_path = Util::path_normalize(Util::util_get_proc_path());
        exe_dir = Util::path_dirname(exe_path);
        exe_file = Util::path_basename(exe_path);

        // save qmm module path
        qmm_path = Util::path_normalize(Util::util_get_qmm_path());
        qmm_dir = Util::path_dirname(qmm_path);
        qmm_file = Util::path_basename(qmm_path);

        // save qmm module pointer
        qmm_module_ptr = Util::util_get_qmm_handle();

        // since we don't have the mod directory yet (can only officially get it using engine functions), we can
        // attempt to get the mod directory from the qmm path. if the qmm dir is the same as the exe dir, it's
        // likely that this is a singleplayer game, so just set the temporary moddir to ".".
        // 
        // this doesn't have to be exact, since it will only be used for config loading until the engine is
        // determined and we can actually ask for the mod directory in vmMain(GAME_INIT)
        if (Util::str_striequal(qmm_dir, exe_dir)) {
            mod_dir = ".";
        }
        else {
            mod_dir = Util::path_basename(qmm_dir);
        }

        // hack for OpenJK if the DLL is loaded from a pak file
        if (Util::str_striequal(mod_dir, "temp")) {
            mod_dir = "base";
        }
    }


    void LoadConfig(std::string config_filename) {
        // load config file, try the following locations in order:
        // "<qmmdir>/qmm2.json"
        // "<exedir>/<moddir>/qmm2.json"
        std::string try_paths[] = {
            fmt::format("{}/{}", qmm_dir, config_filename),
            fmt::format("{}/{}/{}", exe_dir, mod_dir, config_filename),
        };
        for (std::string& try_path : try_paths) {
            try_path = Util::path_normalize(try_path);
            if (try_path.empty() || !Util::path_is_allowed(try_path))
                continue;
            g_cfg = Config::cfg_load(try_path);
            if (!g_cfg.empty()) {
                cfg_path = try_path;
                QMMLOG(QMM_LOG_NOTICE, "QMM") << "Config file found! Path: \"" << cfg_path << "\"\n";
                return;
            }
        }

        // a default constructed json object is a blank {}, so in case of load failure, we can still try to read from it and assume defaults
        QMMLOG(QMM_LOG_WARNING, "QMM") << "QMM::LoadConfig(): Unable to load config file \"" << config_filename << "\", all settings will use default values\n";
    }


    bool DetectGame(std::string cfg_game, APIType engine) {
        if (cfg_game.empty())
            cfg_game = "auto";

        bool is_auto = Util::str_striequal(cfg_game, "auto");

        for (GameSupport* gamesupport : api_supportedgames) {
            // if short name matches config option, we found it!
            if (!is_auto && Util::str_striequal(cfg_game, gamesupport->GameCode())) {
                QMMLOG(QMM_LOG_INFO, "QMM") << "Found game match for config option \"" << cfg_game << "\"\n";
                game = gamesupport;
                is_auto_detected = false;
                // call the game's auto-detect function, since it may do some logic
                (void)gamesupport->AutoDetect(engine);
                return true;
            }
            // otherwise, if auto, call the game's auto-detect function
            else if (is_auto && gamesupport->AutoDetect(engine)) {
                QMMLOG(QMM_LOG_INFO, "QMM") << "Found game match with auto-detection - \"" << gamesupport->GameCode() << "\"\n";
                game = gamesupport;
                is_auto_detected = true;
                return true;
            }
        }

        return false;
    }


    bool LoadMod(std::string cfg_mod) {
        if (cfg_mod.empty())
            cfg_mod = "auto";

        QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to find mod using \"" << cfg_mod << "\"\n";
        // if "mod" config setting is an absolute path, just attempt to load it directly
        if (!Util::str_striequal(cfg_mod, "auto") && Util::path_is_absolute(cfg_mod)) {
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
        std::vector<std::string> try_paths;
        // if "mod" config setting was "auto"
        if (Util::str_striequal(cfg_mod, "auto")) {
            // treat as if "mod" config setting was "qmm_" plus the default dll name for this engine
            cfg_mod = fmt::format("qmm_{}", game->DefaultDLLName());
            // add QVM filename to search list if this game supports it
            if (game->DefaultQVMName())
                try_paths.push_back(game->DefaultQVMName());
        }
        // if "mod" config setting was a relative path, do nothing special unless QVM
        if (Util::str_striequal(Util::path_baseext(cfg_mod), EXT_QVM) && game->DefaultQVMName())
            try_paths.push_back(cfg_mod);
        try_paths.push_back(fmt::format("{}/{}", qmm_dir, cfg_mod));
        try_paths.push_back(fmt::format("{}/{}/{}", exe_dir, mod_dir, cfg_mod));
        for (std::string& try_path : try_paths) {
            try_path = Util::path_normalize(try_path);
            if (try_path.empty() || !Util::path_is_allowed(try_path))
                continue;
            QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to load mod \"" << try_path << "\"\n";
            if (g_mod.Load(try_path))
                return true;
        }

        return false;
    }


    bool LoadPlugin(std::string plugin_path) {
        Plugin p;
        // absolute path, just attempt to load it directly
        if (Util::path_is_absolute(plugin_path)) {
            // plugin_load returns 0 if no plugin file was found, 1 if success, and -1 if file was found but failure
            if (p.Load(plugin_path) > 0) {
                g_plugins.push_back(std::move(p));
                return true;
            }
            return false;
        }
        // relative path, try the following locations in order:
        // "<qmmdir>/<plugin>"
        // "<exedir>/<moddir>/<plugin>"
        std::string try_paths[] = {
            fmt::format("{}/{}", qmm_dir, plugin_path),
            fmt::format("{}/{}/{}", exe_dir, mod_dir, plugin_path),
        };
        for (std::string& try_path : try_paths) {
            try_path = Util::path_normalize(try_path);
            if (try_path.empty() || !Util::path_is_allowed(try_path))
                continue;
            // plugin_load returns 0 if no plugin file was found, 1 if success, and -1 if file was found but failure
            int ret = p.Load(try_path);
            if (ret > 0) {
                g_plugins.push_back(std::move(p));
                return true;
            }
            // file not found, bad DLL, or not a valid plugin DLL
            else if (ret == 0)
                continue;
            // path was to a valid plugin DLL, but shouldn't be loaded
            else if (ret < 0)
                return false;
        }

        return false;
    }


    intptr_t Route(bool is_syscall, intptr_t cmd, intptr_t* args) {
        const char* msg_name;
        const char* func_name;
        GameSupport* gamesupport = game;

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

            QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << p.plugininfo->name << "\" QMM_" << func_name << "( " << msg_name << "(" << cmd << ")) returning " << plugin_ret << " with result " << Plugin::plugin_result_to_str(g_plugin_globals.plugin_result) << "\n";

            // set new max result
            max_result = Util::util_max(g_plugin_globals.plugin_result, max_result);
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
                real_ret = gamesupport->syscall_args(cmd, args);
            else
                real_ret = gamesupport->vmMain_args(cmd, args);

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

            QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << p.plugininfo->name << "\" QMM_" << func_name << "_Post( " << msg_name << "(" << cmd << ")) returning " << plugin_ret << " with result " << Plugin::plugin_result_to_str(g_plugin_globals.plugin_result) << "\n";

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


    void ArgV(intptr_t argn, char* buf, intptr_t buflen) {
        if (!buf || !buflen)
            return;

        // char* (*argv)(int argn);
        // void trap_Argv(int argn, char* buffer, int bufferSize);
        // some games don't return pointers because of QVM interaction, so if this returns anything but null
        // (or true?), we probably are in an api game, and need to get the arg from the return value instead
        intptr_t args[] = { argn, (intptr_t)buf, buflen };
        intptr_t ret = QMM::game->syscall_args(QMM::game->QMMEngMsg(QMM_G_ARGV), args);
        if (ret > 1)
            Util::strncpyz(buf, (const char*)ret, (size_t)buflen);
    }


    intptr_t vmMain_args(intptr_t cmd, intptr_t* args) {
        QMMLOG(QMM_LOG_DEBUG, "QMM") << "vmMain(" << QMM::game->ModMsgName(cmd) << "(" << cmd << ")) called\n";

        if (cmd == QMM::msg_GAME_INIT) {
            // initialize our polyfill milliseconds tracker so that now is 0
            (void)Util::util_get_milliseconds();

            // add engine G_PRINT logger (info level and above)
            Log::log_add_sink([](const AixLog::Metadata& metadata, const std::string& message) {
                ENG_SYSCALL(QMM::msg_G_PRINT, Log::log_format(metadata, message, false).c_str());
                },
                Log::CONSOLE_SEVERITY);

            QMMLOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " [" QMM_OS " " QMM_ARCH " (" QMM_BUILD ")] initializing\n";

            // get mod dir from engine
            char moddir[256];
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_CVAR_VARIABLE_STRING_BUFFER), QMM::game->ModCvar(), moddir, sizeof(moddir));
            moddir[sizeof(moddir) - 1] = '\0';
            QMM::mod_dir = moddir;
            // the default mod (including all singleplayer games) returns "" for the fs_game, so grab the default mod dir from game info instead
            if (QMM::mod_dir.empty())
                QMM::mod_dir = QMM::game->DefaultModDir();

            QMMLOG(QMM_LOG_INFO, "QMM") << "Game: " << QMM::game->GameCode() << "/\"" << QMM::game->GameName() << "\" (Source: " << (QMM::is_auto_detected ? "Auto-detected" : "Config file") << ")\n";
            QMMLOG(QMM_LOG_INFO, "QMM") << "ModDir: " << QMM::mod_dir << "\n";
            QMMLOG(QMM_LOG_INFO, "QMM") << "Config file: \"" << QMM::cfg_path << "\" " << (g_cfg.is_discarded() ? "(error)" : "") << "\n";

            QMMLOG(QMM_LOG_INFO, "QMM") << "Built: " QMM_COMPILE " by " QMM_BUILDER "\n";
            QMMLOG(QMM_LOG_INFO, "QMM") << "URL: " QMM_URL "\n";

            // create qmm_version cvar
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_CVAR_REGISTER), nullptr, "qmm_version", "v" QMM_VERSION, QMM_ENG_MSG(QMM_CVAR_ROM) | QMM_ENG_MSG(QMM_CVAR_SERVERINFO));

            // load mod
            std::string cfg_mod = Config::cfg_get_string(g_cfg, "mod", "auto");
            // check command line arguments for a mod filename
            cfg_mod = Util::util_get_cmdline_arg("--qmm_mod", cfg_mod);
            if (!QMM::LoadMod(cfg_mod)) {
                if (!QMM::is_shutdown) {
                    QMM::is_shutdown = true;
                    QMMLOG(QMM_LOG_FATAL, "QMM") << "QMM was unable to load the mod file using \"" << cfg_mod << "\". Please set the \"mod\" option in qmm2.json. Refer to the documentation for more information.\n";
                    ENG_SYSCALL(QMM_ENG_MSG(QMM_G_ERROR), "\nFatal QMM Error:\nQMM was unable to load the mod file.\nPlease set the \"mod\" option in qmm2.json.\nRefer to the documentation for more information.\n");
                }
                return 0;
            }
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Successfully loaded " << APIType_Function(g_mod.api) << " mod \"" << g_mod.path << "\"\n";

            // cgame passthrough hack:
            // mod DLL is loaded, so find the vmMain and dllEntry functions and call dllEntry.
            // JASP+JK2SP's cgame dllEntry functions actually call into the syscall almost immediately,
            // so make sure we store vmMain first in case there's some re-entrancy
            if (QMM::CGame::syscall) {
                QMM::CGame::vmMain = (mod_vmMain)Util::dll_symbol(g_mod.dll, "vmMain");
                QMMLOG(QMM_LOG_DEBUG, "QMM") << "Storing cgame vmMain = " << QMM::CGame::vmMain << "\n";

                // pass original cgame syscall to dllEntry in mod
                mod_dllEntry pfndllEntry = (mod_dllEntry)Util::dll_symbol(g_mod.dll, "dllEntry");
                QMMLOG(QMM_LOG_DEBUG, "QMM") << "Passing cgame syscall to dllEntry = " << pfndllEntry << "\n";
                pfndllEntry(QMM::CGame::syscall);
            }

            // load plugins
            QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to load plugins\n";
            for (std::string& plugin_path : Config::cfg_get_array_str(g_cfg, "plugins")) {
                QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to load plugin \"" << plugin_path << "\"...\n";
                if (QMM::LoadPlugin(plugin_path)) {
                    QMMLOG(QMM_LOG_INFO, "QMM") << "Plugin \"" << plugin_path << "\" loaded\n";
                }
                else {
                    QMMLOG(QMM_LOG_INFO, "QMM") << "Plugin \"" << plugin_path << "\" not loaded\n";
                }
            }
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Successfully loaded " << g_plugins.size() << " plugin(s)\n";

            // exec the qmmexec cfg
            std::string cfg_execcfg = Config::cfg_get_string(g_cfg, "execcfg", "qmmexec.cfg");
            if (!cfg_execcfg.empty()) {
                QMMLOG(QMM_LOG_NOTICE, "QMM") << "Executing config file \"" << cfg_execcfg << "\"\n";
                ENG_SYSCALL(QMM_ENG_MSG(QMM_G_SEND_CONSOLE_COMMAND), QMM_ENG_MSG(QMM_EXEC_APPEND), fmt::format("exec {}\n", cfg_execcfg).c_str());
            }

            // we're done!
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Startup successful!\n";
        }

        else if (cmd == QMM::msg_GAME_CONSOLE_COMMAND) {
            char arg_cmd[10];
            int argn = 0;
            // get command
            QMM::ArgV(argn, arg_cmd, sizeof(arg_cmd));

            // if command is "sv", then get the next arg
            // idTech2 games use "sv" to run a gamedll command
            if (Util::str_striequal("sv", arg_cmd)) {
                argn++;
                QMM::ArgV(argn, arg_cmd, sizeof(arg_cmd));
            }
            // check for "qmm" command
            if (Util::str_striequal("qmm", arg_cmd) || Util::str_striequal("/qmm", arg_cmd)) {
                // because of "sv", pass 0 or 1 which gets added to argn in the handler function
                HandleQMMCommand(argn);
                return 1;
            }
        }

        // route call to plugins and mod
        intptr_t ret = QMM::Route(false, cmd, args); // true = is_syscall

        // handle shut down (this is after the plugins and mod get called with GAME_SHUTDOWN)
        if (cmd == QMM::msg_GAME_SHUTDOWN) {
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Shutdown initiated!\n";

            // cgame passthrough hack:
            // hack to keep single player games shutting down correctly between levels/cutscenes/etc
            if (QMM::CGame::syscall) {
                QMM::CGame::is_shutdown = true;
                QMMLOG(QMM_LOG_NOTICE, "QMM") << "Delaying shutting down mod so cgame shutdown can run\n";
            }
            else {
                // unload mod
                QMMLOG(QMM_LOG_NOTICE, "QMM") << "Shutting down mod\n";
                g_mod.Unload();
            }

            // unload each plugin (call QMM_Detach, and then dlclose)
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Shutting down plugins\n";
            for (Plugin& p : g_plugins) {
                p.Unload();
            }
            g_plugins.clear();

            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Finished shutting down\n";
        }

        QMMLOG(QMM_LOG_TRACE, "QMM") << "vmMain(" << QMM::game->ModMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";

        return ret;
    }


    intptr_t syscall_args(intptr_t cmd, intptr_t* args) {
        QMMLOG(QMM_LOG_DEBUG, "QMM") << "syscall(" << QMM::game->EngMsgName(cmd) << "(" << cmd << ")) called\n";

        // route call to plugins and mod
        intptr_t ret = QMM::Route(true, cmd, args); // true = is_syscall

        QMMLOG(QMM_LOG_DEBUG, "QMM") << "syscall(" << QMM::game->EngMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";

        return ret;
    }


    // Print string to game console
#define CONSOLE_PRINT(str)			ENG_SYSCALL(msg_G_PRINT, str)
// Print formatted string to game console
#define CONSOLE_PRINTF(str, ...)	ENG_SYSCALL(msg_G_PRINT, fmt::format(str, ## __VA_ARGS__).c_str())

    void HandleQMMCommand(intptr_t arg_start) {
        char arg1[10] = "", arg2[10] = "";

        int argc = (int)ENG_SYSCALL(QMM_ENG_MSG(QMM_G_ARGC));
        QMM::ArgV(arg_start + 1, arg1, sizeof(arg1));
        if (argc > arg_start + 2)
            QMM::ArgV(arg_start + 2, arg2, sizeof(arg2));

        if (Util::str_striequal("status", arg1) || Util::str_striequal("info", arg1)) {
            CONSOLE_PRINT("(QMM) QMM v" QMM_VERSION " [" QMM_OS " " QMM_ARCH " (" QMM_BUILD ")]\n");
            CONSOLE_PRINTF("(QMM) Game       : {}/\"{}\" ({}) (Source: {})\n", QMM::game->GameCode(), QMM::game->GameName(), APIType_Function(QMM::api), QMM::is_auto_detected ? "Auto-detected" : "Config file");
            CONSOLE_PRINTF("(QMM) ModDir     : {}\n", QMM::mod_dir);
            CONSOLE_PRINTF("(QMM) Config file: \"{}\" {}\n", QMM::cfg_path, g_cfg.empty() ? "(error)" : "");
            CONSOLE_PRINT("(QMM) Built      : " QMM_COMPILE " by " QMM_BUILDER "\n");
            CONSOLE_PRINT("(QMM) URL        : " QMM_URL "\n");
            CONSOLE_PRINT("(QMM) PIFV       : " STRINGIFY(QMM_PIFV_MAJOR) ":" STRINGIFY(QMM_PIFV_MINOR) "\n");
            CONSOLE_PRINTF("(QMM) Plugins    : {}\n", g_plugins.size());
            CONSOLE_PRINTF("(QMM) Loaded mod : {} ({})\n", g_mod.path, APIType_Function(g_mod.api));
            if (g_mod.vm.memory) {
                CONSOLE_PRINT("(QMM)\n");
                CONSOLE_PRINT("(QMM) QVM mod information\n");
                CONSOLE_PRINT("(QMM) -------------------\n");
                CONSOLE_PRINTF("(QMM) QVM magic number   : {:x} ({})\n", g_mod.vm.magic, g_mod.vm.magic == QVM_MAGIC ? "QVM_MAGIC" : "QVM_MAGIC_VER2");
                CONSOLE_PRINTF("(QMM) QVM file size      : {}\n", g_mod.vm.filesize);
                CONSOLE_PRINTF("(QMM) QVM memory base    : {}\n", fmt::ptr(g_mod.vm.memory));
                CONSOLE_PRINTF("(QMM) QVM memory size    : {}\n", g_mod.vm.memorysize);
                CONSOLE_PRINTF("(QMM) QVM instr count    : {}\n", g_mod.vm.instructioncount);
                CONSOLE_PRINTF("(QMM) QVM codeseg size   : {}\n", g_mod.vm.codeseglen);
                CONSOLE_PRINTF("(QMM) QVM dataseg size   : {}\n", g_mod.vm.dataseglen);
                CONSOLE_PRINTF("(QMM) QVM stack size     : {}\n", g_mod.vm.stacksize);
                CONSOLE_PRINTF("(QMM) QVM hunk size      : {}\n", g_mod.vm.hunksize);
                CONSOLE_PRINTF("(QMM) QVM hunk usage     : {}\n", g_mod.vm.hunkhigh - g_mod.vm.hunkptr);
                CONSOLE_PRINTF("(QMM) QVM data validation: {}\n", g_mod.vm.verify_data ? "on" : "off");
            }
        }
        else if (Util::str_striequal("list", arg1) || Util::str_striequal("pluginlist", arg1)) {
            CONSOLE_PRINT("(QMM) id - plugin [version]\n");
            CONSOLE_PRINT("(QMM) ---------------------\n");
            int num = 1;
            for (Plugin& p : g_plugins) {
                CONSOLE_PRINTF("(QMM) {:>2} - {} [{}]\n", num, p.plugininfo->name, p.plugininfo->version);
                num++;
            }
        }
        else if (Util::str_striequal("plugin", arg1) || Util::str_striequal("plugininfo", arg1)) {
            if (argc == arg_start + 2) {
                CONSOLE_PRINTF("(QMM) qmm {} <id> - outputs info on plugin with id\n", arg1);
                return;
            }
            size_t pid = (size_t)atoi(arg2);
            if (pid > 0 && pid <= g_plugins.size()) {
                Plugin& p = g_plugins[pid - 1];
                CONSOLE_PRINTF("(QMM) Plugin info for #{}:\n", arg2);
                CONSOLE_PRINTF("(QMM) Name: {}\n", p.plugininfo->name);
                CONSOLE_PRINTF("(QMM) Version: {}\n", p.plugininfo->version);
                CONSOLE_PRINTF("(QMM) URL: {}\n", p.plugininfo->url);
                CONSOLE_PRINTF("(QMM) Author: {}\n", p.plugininfo->author);
                CONSOLE_PRINTF("(QMM) Desc: {}\n", p.plugininfo->desc);
                CONSOLE_PRINTF("(QMM) Logtag: {}\n", p.plugininfo->logtag);
                CONSOLE_PRINTF("(QMM) Interface version: {}:{}\n", p.plugininfo->pifv_major, p.plugininfo->pifv_minor);
                CONSOLE_PRINTF("(QMM) Path: {}\n", p.path);
            }
            else {
                CONSOLE_PRINTF("(QMM) Unable to find plugin #{}\n", arg2);
            }
        }
        else if (Util::str_striequal("loglevel", arg1)) {
            if (argc == arg_start + 2) {
                CONSOLE_PRINTF("(QMM) qmm {} <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n", arg1);
                return;
            }
            int severity = Log::log_severity_from_name(arg2);
            Log::log_set_severity(severity);
            CONSOLE_PRINTF("(QMM) Log level set to {}\n", Log::log_name_from_severity(severity));
        }
        else if (Util::str_striequal("reload", arg1)) {
            g_cfg = Config::cfg_load(QMM::cfg_path);
            CONSOLE_PRINT("(QMM) Configuration file reloaded!\n");
        }
        else if (Util::str_striequal("credits", arg1) || Util::str_striequal("thanks", arg1)) {
            CONSOLE_PRINT("(QMM) QMM credits:\n");
            CONSOLE_PRINT("(QMM) Designed by:\n");
            CONSOLE_PRINT("(QMM)  - Kevin Masterson\n");
            CONSOLE_PRINT("(QMM)\n");
            CONSOLE_PRINT("(QMM) Special thanks to:\n");
            CONSOLE_PRINT("(QMM)  - BAStumm\n");
            CONSOLE_PRINT("(QMM)  - loupgarou21\n");
            CONSOLE_PRINT("(QMM)  - nevcairiel\n");
            CONSOLE_PRINT("(QMM)  - BAILOPAN\n");
            CONSOLE_PRINT("(QMM)  - Lumpy\n");
            CONSOLE_PRINT("(QMM)  - para\n");
            CONSOLE_PRINT("(QMM)  - I have forgotten many since 2004; please let me know!\n");
            CONSOLE_PRINT("(QMM)\n");
            CONSOLE_PRINT("(QMM) QMM uses the following libraries:\n");
            CONSOLE_PRINT("(QMM)  - nlohmann/json - https://github.com/nlohmann/json\n");
            CONSOLE_PRINT("(QMM)  - aixlog - https://github.com/badaix/aixlog\n");
            CONSOLE_PRINT("(QMM)  - fmtlib - https://github.com/fmtlib/fmt\n");
        }
        else {
            if (!Util::str_striequal("help", arg1)) {
                CONSOLE_PRINTF("(QMM) Unknown command: {}\n", arg1);
                CONSOLE_PRINT("(QMM)\n");
            }
            CONSOLE_PRINT("(QMM) Usage: qmm <command> [params]\n");
            CONSOLE_PRINT("(QMM) Available commands:\n");
            CONSOLE_PRINT("(QMM) qmm info - displays information about QMM\n");
            CONSOLE_PRINT("(QMM) qmm list - displays list of loaded QMM plugins\n");
            CONSOLE_PRINT("(QMM) qmm plugin <id> - outputs info on plugin with id\n");
            CONSOLE_PRINT("(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
            CONSOLE_PRINT("(QMM) qmm reload - reloads the QMM configuration file\n");
            CONSOLE_PRINT("(QMM) qmm credits - QMM credits\n");
            CONSOLE_PRINT("(QMM) qmm help - displays this help\n");
        }
    }


    EngineFileRead::EngineFileRead() : handle(0) {
    }


    uint8_t* EngineFileRead::Open(std::string path) {
        intptr_t filelen = ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FOPEN_FILE), path.c_str(), &this->handle, QMM_ENG_MSG(QMM_FS_READ));
        if (filelen <= 0 || !this->handle) {
            this->Close();
            return nullptr;
        }
        this->file.resize((size_t)filelen);
        ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_READ), this->file.data(), this->file.size(), this->handle);
        return this->file.data();
    }


    int EngineFileRead::Size() {
        return (int)this->file.size();
    }


    void EngineFileRead::Close() {
        this->file.clear();
        if (this->handle)
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FCLOSE_FILE), this->handle);
        this->handle = 0;
    }


    EngineFileRead::~EngineFileRead() {
        this->Close();
    }

}   // namespace QMM
