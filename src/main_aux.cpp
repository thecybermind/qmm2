/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS
#include "osdef.h"
#include <cstdlib>
#include <string>
#include "log.h"
#include "format.h"
#include "config.h"
#include "main.h"
#include "game_api.h"
#include "qmmapi.h"
#include "plugin.h"
#include "mod.h"
#include "util.h"
#include "version.h"


// general code to get path/module/binary/etc information
void main_detect_env() {
    // save exe module path
    g_gameinfo.exe_path = util_get_proc_path();
    g_gameinfo.exe_dir = path_dirname(g_gameinfo.exe_path);
    g_gameinfo.exe_file = path_basename(g_gameinfo.exe_path);

    // save qmm module path
    g_gameinfo.qmm_path = util_get_qmm_path();
    g_gameinfo.qmm_dir = path_dirname(g_gameinfo.qmm_path);
    g_gameinfo.qmm_file = path_basename(g_gameinfo.qmm_path);

    // save qmm module pointer
    g_gameinfo.qmm_module_ptr = util_get_qmm_handle();

    // since we don't have the mod directory yet (can only officially get it using engine functions), we can
    // attempt to get the mod directory from the qmm path. if the qmm dir is the same as the exe dir, it's
    // likely that this is a singleplayer game, so just set the temporary moddir to ".".
    // this doesn't have to be exact, since it will only be used only for config loading.
    if (str_striequal(g_gameinfo.qmm_dir, g_gameinfo.exe_dir)) {
        g_gameinfo.mod_dir = ".";
    }
    else {
        g_gameinfo.mod_dir = path_basename(g_gameinfo.qmm_dir);
    }
}


// general code to load config file. called from dllEntry() and GetGameAPI()
void main_load_config() {
    // load config file, try the following locations in order:
    // "<qmmdir>/qmm2.json"
    // "<exedir>/<moddir>/qmm2.json"
    // "./<moddir>/qmm2.json"
    std::string try_paths[] = {
        fmt::format("{}/qmm2.json", g_gameinfo.qmm_dir),
        fmt::format("{}/{}/qmm2.json", g_gameinfo.exe_dir, g_gameinfo.mod_dir),
        fmt::format("./{}/qmm2.json", g_gameinfo.mod_dir)
    };
    for (std::string& try_path : try_paths) {
        g_cfg = cfg_load(try_path);
        if (!g_cfg.empty()) {
            g_gameinfo.cfg_path = try_path;
            LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Config file found! Path: \"{}\"\n", g_gameinfo.cfg_path);
            return;
        }
    }

    // a default constructed json object is a blank {}, so in case of load failure, we can still try to read from it and assume defaults
    LOG(QMM_LOG_WARNING, "QMM") << "Unable to load config file, all settings will use default values\n";
}


// general code to auto-detect what game engine loaded us
void main_detect_game(std::string cfg_game, bool is_GetGameAPI_mode) {
    // find what game we are loaded in
    for (int i = 0; g_supportedgames[i].dllname; i++) {
        supportedgame& game = g_supportedgames[i];
        // only check games with apientry based on GetGameAPI_mode
        if (is_GetGameAPI_mode != !!game.pfnGetGameAPI)
            continue;

        // if short name matches config option, we found it!
        if (str_striequal(cfg_game, game.gamename_short)) {
            LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Found game match for config option \"{}\"\n", cfg_game);
            g_gameinfo.game = &game;
            g_gameinfo.isautodetected = false;
            return;
        }
        // otherwise, if auto, we need to check matching dll names, with optional exe hint
        if (str_striequal(cfg_game, "auto")) {
            // dll name matches
            if (str_striequal(g_gameinfo.qmm_file, game.dllname)) {
                LOG(QMM_LOG_INFO, "QMM") << fmt::format("Found game match for dll name \"{}\" - {}\n", game.dllname, game.gamename_short);
                // if no hint array exists, assume we match
                if (!game.exe_hints.size()) {
                    LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("No exe hint for game, assuming match\n");
                    g_gameinfo.game = &game;
                    g_gameinfo.isautodetected = true;
                    return;
                }
                // if a hint array exists, check each for an exe file match
                for (std::string& hint : game.exe_hints) {
                    if (str_stristr(g_gameinfo.exe_file, hint)) {
                        LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Found game match for exe hint \"{}\" - {}\n", hint, game.gamename_short);
                        g_gameinfo.game = &game;
                        g_gameinfo.isautodetected = true;
                        return;
                    }
                }
            }
        }
    }
}


// general code to find a mod file to load
bool main_load_mod(std::string cfg_mod) {
    LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to find mod using \"{}\"\n", cfg_mod);
    // if config setting is an absolute path, just attempt to load it directly
    if (!path_is_relative(cfg_mod)) {
        LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to load mod \"{}\"\n", cfg_mod);
        if (!mod_load(g_mod, cfg_mod))
            return false;
    }
    // if config setting is "auto", try the following locations in order:
    // "<qvmname>" (if the game engine supports it)
    // "<qmmdir>/qmm_<dllname>"
    // "<exedir>/<moddir>/qmm_<dllname>"
    // "<exedir>/<moddir>/<dllname>"
    // "./<moddir>/qmm_<dllname>"
    else if (str_striequal(cfg_mod, "auto")) {
        std::string try_paths[] = {
            g_gameinfo.game->qvmname ? g_gameinfo.game->qvmname : "",	// (only if game engine supports it)
            fmt::format("{}/qmm_{}", g_gameinfo.qmm_dir, g_gameinfo.game->dllname),
            fmt::format("{}/{}/qmm_{}", g_gameinfo.exe_dir, g_gameinfo.mod_dir, g_gameinfo.game->dllname),
            fmt::format("{}/{}/{}", g_gameinfo.exe_dir, g_gameinfo.mod_dir, g_gameinfo.game->dllname),
            fmt::format("./{}/qmm_{}", g_gameinfo.mod_dir, g_gameinfo.game->dllname)
        };
        // try paths
        for (std::string& try_path : try_paths) {
            if (try_path.empty())
                continue;
            LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to auto-load mod \"{}\"\n", try_path);
            if (mod_load(g_mod, try_path))
                return true;
        }
    }
    // if config setting is a relative path, try the following locations in order:
    // "<mod>"
    // "<qmmdir>/<mod>"
    // "<exedir>/<moddir>/<mod>"
    // "./<moddir>/<mod>"
    else {
        std::string try_paths[] = {
            cfg_mod,
            fmt::format("{}/{}", g_gameinfo.qmm_dir, cfg_mod),
            fmt::format("{}/{}/{}", g_gameinfo.exe_dir, g_gameinfo.mod_dir, cfg_mod),
            fmt::format("./{}/{}", g_gameinfo.mod_dir, cfg_mod)
        };
        // try paths
        for (std::string& try_path : try_paths) {
            if (try_path.empty())
                continue;
            LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to load mod \"{}\"\n", try_path);
            if (mod_load(g_mod, try_path))
                return true;
        }
    }

    return false;
}


// general code to find a plugin file to load
bool main_load_plugin(std::string plugin_path) {
    plugin p;
    // absolute path, just attempt to load it directly
    if (!path_is_relative(plugin_path)) {
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
    // "./<moddir>/<plugin>"
    std::string try_paths[] = {
        fmt::format("{}/{}", g_gameinfo.qmm_dir, plugin_path),
        fmt::format("{}/{}/{}", g_gameinfo.exe_dir, g_gameinfo.mod_dir, plugin_path),
        fmt::format("./{}/{}", g_gameinfo.mod_dir, plugin_path)
    };
    for (std::string& try_path : try_paths) {
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


// handle "qmm" console command
void main_handle_command_qmm(intptr_t arg_start) {
    char arg1[10] = "", arg2[10] = "";

    int argc = (int)ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGC]);
    qmm_argv(arg_start + 1, arg1, sizeof(arg1));
    if (argc > arg_start + 2)
        qmm_argv(arg_start + 2, arg2, sizeof(arg2));

    if (str_striequal("status", arg1) || str_striequal("info", arg1)) {
        CONSOLE_PRINTF("(QMM) QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ") loaded\n");
        CONSOLE_PRINTF("(QMM) Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file");
        CONSOLE_PRINTF("(QMM) ModDir: {}\n", g_gameinfo.mod_dir);
        CONSOLE_PRINTF("(QMM) Config file: \"{}\" {}\n", g_gameinfo.cfg_path, g_cfg.is_discarded() ? " (error)" : "");
        CONSOLE_PRINTF("(QMM) Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
        CONSOLE_PRINTF("(QMM) URL: " QMM_URL "\n");
        CONSOLE_PRINTF("(QMM) Plugin interface: {}:{}\n", QMM_PIFV_MAJOR, QMM_PIFV_MINOR);
        CONSOLE_PRINTF("(QMM) Plugins loaded: {}\n", g_plugins.size());
        CONSOLE_PRINTF("(QMM) Loaded mod file: {}\n", g_mod.path);
        if (g_mod.vmbase) {
            CONSOLE_PRINTF("(QMM) QVM file size      : {}\n", g_mod.vm.filesize);
            CONSOLE_PRINTF("(QMM) QVM memory base    : {}\n", (void*)g_mod.vm.memory);
            CONSOLE_PRINTF("(QMM) QVM memory size    : {}\n", g_mod.vm.memorysize);
            CONSOLE_PRINTF("(QMM) QVM instr count    : {}\n", g_mod.vm.instructioncount);
            CONSOLE_PRINTF("(QMM) QVM codeseg size   : {}\n", g_mod.vm.codeseglen);
            CONSOLE_PRINTF("(QMM) QVM dataseg size   : {}\n", g_mod.vm.dataseglen);
            CONSOLE_PRINTF("(QMM) QVM stack size     : {}\n", g_mod.vm.stacksize);
            CONSOLE_PRINTF("(QMM) QVM data validation: {}\n", g_mod.vm.verify_data ? "on" : "off");
        }
    }
    else if (str_striequal("list", arg1)) {
        CONSOLE_PRINTF("(QMM) id - plugin [version]\n");
        CONSOLE_PRINTF("(QMM) ---------------------\n");
        int num = 1;
        for (plugin& p : g_plugins) {
            CONSOLE_PRINTF("(QMM) {:>2} - {} [{}]\n", num, p.plugininfo->name, p.plugininfo->version);
            num++;
        }
    }
    else if (str_striequal("plugin", arg1) || str_striequal("plugininfo", arg1)) {
        if (argc == arg_start + 2) {
            CONSOLE_PRINTF("(QMM) qmm info <id> - outputs info on plugin with id\n");
            return;
        }
        size_t pid = (size_t)atoi(arg2);
        if (pid > 0 && pid <= g_plugins.size()) {
            plugin& p = g_plugins[pid - 1];
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
    else if (str_striequal("loglevel", arg1)) {
        if (argc == arg_start + 2) {
            CONSOLE_PRINTF("(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
            return;
        }
        AixLog::Severity severity = log_severity_from_name(arg2);
        log_set_severity(severity);
        CONSOLE_PRINTF("(QMM) Log level set to {}\n", log_name_from_severity(severity));
    }
    else {
        CONSOLE_PRINTF("(QMM) Usage: qmm <command> [params]\n");
        CONSOLE_PRINTF("(QMM) Available commands:\n");
        CONSOLE_PRINTF("(QMM) qmm info - displays information about QMM\n");
        CONSOLE_PRINTF("(QMM) qmm list - displays information about loaded QMM plugins\n");
        CONSOLE_PRINTF("(QMM) qmm plugin <id> - outputs info on plugin with id\n");
        CONSOLE_PRINTF("(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
    }
}


// route vmMain call to plugins and mod
intptr_t main_route_vmmain(intptr_t cmd, intptr_t* args) {
#ifdef _DEBUG
    const char* msgname = g_gameinfo.game->mod_msg_names(cmd);
#endif

    // store max result
    plugin_res max_result = QMM_UNUSED;
    // return values from a plugin call
    intptr_t plugin_ret = 0;
    // return value from mod call
    intptr_t mod_ret = 0;
    // return value to pass back to the engine (either mod_ret, or a plugin_ret from QMM_OVERRIDE/QMM_SUPERCEDE result)
    intptr_t final_ret = 0;

    // begin passing calls to plugins' QMM_vmMain functions
    for (plugin& p : g_plugins) {
        g_plugin_globals.plugin_result = QMM_UNUSED;
        // allow plugins to see the current final_ret value
        g_plugin_globals.final_return = final_ret;

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Plugin {} QMM_vmMain({} {}) called\n", p.plugininfo->name, msgname, cmd);
#endif

        // call plugin's vmMain and store return value
        plugin_ret = p.QMM_vmMain(cmd, args);

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Plugin {} QMM_vmMain({} {}) returning {} with result {}\n", p.plugininfo->name, msgname, cmd, plugin_ret, plugin_result_to_str(g_plugin_globals.plugin_result));
#endif

        // set new max result
        max_result = util_max(g_plugin_globals.plugin_result, max_result);
        // store current max result in global for plugins
        g_plugin_globals.high_result = max_result;
        // invalid/error result values
        if (g_plugin_globals.plugin_result == QMM_UNUSED)
            LOG(QMM_LOG_WARNING, "QMM") << fmt::format("vmMain({}): Plugin \"{}\" did not set result flag\n", g_gameinfo.game->mod_msg_names(cmd), p.plugininfo->name);
        else if (g_plugin_globals.plugin_result == QMM_ERROR)
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("vmMain({}): Plugin \"{}\" set result flag QMM_ERROR\n", g_gameinfo.game->mod_msg_names(cmd), p.plugininfo->name);

        // if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
        else if (g_plugin_globals.plugin_result >= QMM_OVERRIDE)
            final_ret = plugin_ret;
    }

    // call real vmMain function (unless a plugin resulted in QMM_SUPERCEDE)
    if (max_result < QMM_SUPERCEDE) {
#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Game vmMain({} {}) called\n", msgname, cmd);
#endif

        mod_ret = g_gameinfo.pfnvmMain(cmd, QMM_PUT_VMMAIN_ARGS());

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Game vmMain({} {}) returning {}\n", msgname, cmd, mod_ret);
#endif
    }
    else {
#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Game vmMain({} {}) superceded\n", msgname, cmd);
#endif
    }

    // store mod_ret in global for plugins
    g_plugin_globals.orig_return = mod_ret;

    // if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual mod's return value back to the engine
    if (max_result < QMM_OVERRIDE)
        final_ret = mod_ret;

    // pass calls to plugins' QMM_vmMain_Post functions (QMM_OVERRIDE or QMM_SUPERCEDE can still change final_ret)
    for (plugin& p : g_plugins) {
        g_plugin_globals.plugin_result = QMM_UNUSED;
        // allow plugins to see the current final_ret value
        g_plugin_globals.final_return = final_ret;

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Plugin {} QMM_vmMain_Post({} {}) called\n", p.plugininfo->name, msgname, cmd);
#endif

        // call plugin's vmMain_Post and store return value
        plugin_ret = p.QMM_vmMain_Post(cmd, args);

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Plugin {} QMM_vmMain_Post({} {}) returning {} with result {}\n", p.plugininfo->name, msgname, cmd, plugin_ret, plugin_result_to_str(g_plugin_globals.plugin_result));
#endif

        // ignore QMM_UNUSED so plugins can just use return, but still show a message for QMM_ERROR
        if (g_plugin_globals.plugin_result == QMM_ERROR)
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("vmMain({}): Plugin \"{}\" set result flag QMM_ERROR\n", g_gameinfo.game->mod_msg_names(cmd), p.plugininfo->name);

        // if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
        else if (g_plugin_globals.plugin_result >= QMM_OVERRIDE)
            final_ret = plugin_ret;
    }

    return final_ret;
}


// route syscall call to plugins and mod
intptr_t main_route_syscall(intptr_t cmd, intptr_t* args) {
#ifdef _DEBUG
    const char* msgname = g_gameinfo.game->eng_msg_names(cmd);
#endif

    // store max result
    plugin_res max_result = QMM_UNUSED;
    // return values from a plugin call
    intptr_t plugin_ret = 0;
    // return value from engine call
    intptr_t eng_ret = 0;
    // return value to pass back to the engine (either eng_ret, or a plugin_ret from QMM_OVERRIDE/QMM_SUPERCEDE result)
    intptr_t final_ret = 0;

    // begin passing calls to plugins' QMM_syscall functions
    for (plugin& p : g_plugins) {
        g_plugin_globals.plugin_result = QMM_UNUSED;
        // allow plugins to see the current final_ret value
        g_plugin_globals.final_return = final_ret;

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Plugin {} QMM_syscall({} {}) called\n", p.plugininfo->name, msgname, cmd);
#endif

        // call plugin's syscall and store return value
        plugin_ret = p.QMM_syscall(cmd, args);

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Plugin {} QMM_syscall({} {}) returning {} with result {}\n", p.plugininfo->name, msgname, cmd, plugin_ret, plugin_result_to_str(g_plugin_globals.plugin_result));
#endif

        // set new max result
        max_result = util_max(g_plugin_globals.plugin_result, max_result);
        // store current max result in global for plugins
        g_plugin_globals.high_result = max_result;
        // invalid/error result values
        if (g_plugin_globals.plugin_result == QMM_UNUSED)
            LOG(QMM_LOG_WARNING, "QMM") << fmt::format("syscall({}): Plugin \"{}\" did not set result flag\n", g_gameinfo.game->eng_msg_names(cmd), p.plugininfo->name);
        else if (g_plugin_globals.plugin_result == QMM_ERROR)
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("syscall({}): Plugin \"{}\" set result flag QMM_ERROR\n", g_gameinfo.game->eng_msg_names(cmd), p.plugininfo->name);

        // if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
        else if (g_plugin_globals.plugin_result >= QMM_OVERRIDE)
            final_ret = plugin_ret;
    }

    // call real syscall function (unless a plugin resulted in QMM_SUPERCEDE)
    if (max_result < QMM_SUPERCEDE) {
#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Engine syscall({} {}) called\n", msgname, cmd);
#endif

        eng_ret = g_gameinfo.pfnsyscall(cmd, QMM_PUT_SYSCALL_ARGS());

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Engine syscall({} {}) returning {}\n", msgname, cmd, eng_ret);
#endif
    }
    else {
#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Engine syscall({} {}) superceded\n", msgname, cmd);
#endif
    }

    // store eng_ret in global for plugins
    g_plugin_globals.orig_return = eng_ret;

    // if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual engine's return value back to the mod
    if (max_result < QMM_OVERRIDE)
        final_ret = eng_ret;

    // pass calls to plugins' QMM_syscall_Post functions (ignore return values and results)
    for (plugin& p : g_plugins) {
        g_plugin_globals.plugin_result = QMM_UNUSED;
        // allow plugins to see the current final_ret value
        g_plugin_globals.final_return = final_ret;

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Plugin {} QMM_syscall_Post({} {}) called\n", p.plugininfo->name, msgname, cmd);
#endif

        // call plugin's syscall_Post and store return value
        plugin_ret = p.QMM_syscall_Post(cmd, args);

#ifdef _DEBUG
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Plugin {} QMM_syscall_Post({} {}) returning {} with result {}\n", p.plugininfo->name, msgname, cmd, plugin_ret, plugin_result_to_str(g_plugin_globals.plugin_result));
#endif

        // ignore QMM_UNUSED so plugins can just use return, but still show a message for QMM_ERROR
        if (g_plugin_globals.plugin_result == QMM_ERROR)
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("syscall({}): Plugin \"{}\" set result flag QMM_ERROR\n", g_gameinfo.game->eng_msg_names(cmd), p.plugininfo->name);

        // if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
        else if (g_plugin_globals.plugin_result >= QMM_OVERRIDE)
            final_ret = plugin_ret;
    }
    return final_ret;
}
