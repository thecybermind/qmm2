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
#include "log.hpp"
#include "format.hpp"
#include "config.hpp"
#include "gameinfo.hpp"
#include "plugin.hpp"   // g_plugins
#include "main.hpp"     // ArgV
#include "mod.hpp"      // g_mod
#include "util.hpp"


/* This file contains all the entry points for QMM.
 * This is how the engine loads QMM, and how the mod will call into QMM (thinking it's the engine).
 * 
 * void dllEntry(eng_syscall syscall):
 * Called by some engines to give the mod the engine's syscall pointer. the engine will then call vmMain for all mod
 * entries
 * 
 * void* GetGameAPI(void* import, void* extra):
 * Called by some engines to give the mod the engine's game_import_t struct pointer. sometimes there is an apiversion
 * argument as well depending on the game engine. this function should return a game_export_t struct pointer back to
 * the engine. the engine will then call one of the game_export_t functions for all further mod entries.
 * 
 * void* GetModuleAPI(void* import, void* extra):
 * This is the same as GetGameAPI, and was added for the OpenJK engine.
 * 
 * 64-bit Windows only:
 * void* GetCGameAPI(void* import):
 * This is similar to GetGameAPI, but for the client side of the game. This is used only in Quake 2: Remastered
 * which puts the server- and client-side mod components in the same DLL. QMM does not do any hooking of this, and
 * attempts to simply pass the import pointer through to the actual mod DLL and return the mod's export pointer.
 *
 * intptr_t vmMain(intptr_t cmd, ...):
 * Primary entry point for actual game-related functions from engine->mod. Whenever the engine wants the mod to do
 * something or know something, this is called with a particular "cmd" value and associated arguments. For
 * GetGameAPI games, QMM makes a game_export_t struct with lambdas that call this function with a different "cmd"
 * for each function.
 *
 * intptr_t qmm_syscall(intptr_t cmd, ...):
 * Primary entry point for actual game-related functions from mod->engine. Whenever the mod wants the engine to do
 * something or know something, this is called with a particular "cmd" value and associated arguments. For
 * GetGameAPI games, QMM makes a game_import_t struct with lambdas that call this function with a different "cmd"
 * for each function.
 */

 /**
 * @brief Handle "qmm" console command
 *
 * @param arg_start Which argv to start reading command arguments from
 */
static void HandleQMMCommand(intptr_t arg_start);

/* =====================================================
   About overall control flow for dllEntry/vmMain games:
   dllEntry (engine->mod) call flow:
   1. engine calls QMM's dllEntry and passes syscall pointer
   2. get environment info
   3. load config file
   4. open logfile
   5. detect game engine
   6. call game-specific dllEntry function to store syscall
   7. return to engine

   vmMain (engine->mod) call flow:
   1. call is handled by vmMain()
   2. call is passed to plugins' QMM_vmMain functions
   3. if at least one plugin sets the result to QMM_SUPERCEDE, skip to step 6
   4. call is passed to game-specific GAME_vmMain function
   5. call is passed to actual mod vmMain function (or the QVM system is executed)
   6. call is passed to plugins' QMM_vmMain_Post functions
   7. mod vmMain return value (or a value given by last plugin which uses result QMM_SUPERCEDE or QMM_OVERRIDE)
      is returned to engine

   syscall (mod->engine) call flow for QVM mods only:
   1. QVM system calls <GAME>_QVMSyscall function
   2. pointer arguments are converted: if not NULL, the QVM data segment base address is added
   3. call qmm_syscall with converted arguments (continue with next section as if it were a DLL mod)

   syscall (mod->engine) call flow:
   1. call is handled by qmm_syscall()
   2. call is passed to plugins' QMM_syscall functions
   3. if at least one plugin sets the result to QMM_SUPERCEDE, skip to step 6
   4. call is passed to game-specific GAME_syscall function
   5. call is passed to actual engine syscall function
   6. call is passed to plugins' QMM_syscall_Post functions
   7. engine syscall return value (or a value given by last plugin which uses result QMM_SUPERCEDE or QMM_OVERRIDE)
      is returned to mod
   =====================================================
*/

C_DLLEXPORT void dllEntry(eng_syscall syscall) {
    // cgame passthrough hack:
    // QMM is already loaded, so this is a cgame passthrough situation. since the mod DLL isn't loaded yet, we can
    // just store the syscall pointer and pass it to the mod once it's loaded in vmMain(GAME_INIT)
    if (gameinfo.game && gameinfo.api == QMM_API_GETGAMEAPI) {
        cgameinfo.syscall = syscall;
        QMMLOG(QMM_LOG_DEBUG, "QMM") << "Passthrough syscall = " << syscall << "\n";
        return;
    }

    // store the given syscall pointer as a backup.
    // this is used in case we couldn't detect a game and have to call syscall(G_ERROR) to shutdown in vmMain
    gameinfo.syscall = syscall;

    gameinfo.HandleEntry((void*)syscall, nullptr, QMM_API_DLLENTRY);
    return;
}


/* =====================================================
   About overall control flow for GetGameAPI games:
   GetGameAPI (engine->mod) call flow:
   1. engine calls QMM's GetGameAPI and passes game_import_t pointer
   2. get environment info
   3. load config file
   4. open logfile
   5. detect game engine
   6. call game-specific GetGameAPI function to store game_import_t pointer and generate game_export_t pointer
   7. create a game-specific game_export_t struct ("qmm_export") with hooks, and return a pointer to engine
   8. engine stores qmm_export pointer, checks qmm_export->apiversion to match GAME_API_VERSION

   export (engine->mod) call flow
   1. call is handled by lambda inside qmm_export struct that was returned to engine
   2. call is passed to vmMain with function-specific enum
   3. call is passed to plugins' QMM_vmMain functions
   4. if at least one plugin sets the result to QMM_SUPERCEDE, skip to step 7
   5. call is passed to game-specific GAME_vmMain function
   6. call is passed to actual mod game_export_t function
   7. call is passed to plugins' QMM_vmMain_Post functions
   8. mod game_export_t function return value (or a value given by last plugin which uses result QMM_SUPERCEDE or
      QMM_OVERRIDE) is returned to engine

   import (mod->engine) call flow
   1. call is handled by lambda inside game_import_t struct that was given to QMM
   2. call is passed to qmm_syscall with function-specific enum
   3. call is passed to plugins' QMM_syscall functions
   4. if at least one plugin sets the result to QMM_SUPERCEDE, skip to step 7
   5. call is passed to game-specific GAME_syscall function
   6. call is passed to actual engine game_import_t function
   7. call is passed to plugins' QMM_syscall_Post functions
   8. engine game_import_t function return value (or a value given by last plugin which uses result QMM_SUPERCEDE or
      QMM_OVERRIDE) is returned to mod
   =====================================================
*/

C_DLLEXPORT void* GetGameAPI(void* import, void* extra) {
    return gameinfo.HandleEntry(import, extra, QMM_API_GETGAMEAPI);
}


C_DLLEXPORT void* GetModuleAPI(int apiversion, void* import) {
    return gameinfo.HandleEntry((void*)(intptr_t)apiversion, import, QMM_API_GETMODULEAPI);
}


C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

    // if this is a call from cgame and we need to pass this call onto the mod
    if (cgameinfo.syscall && !cgameinfo.is_from_QMM) {
        // cancel if cgame portion of mod isn't actually loaded yet
        if (!cgameinfo.vmMain)
            return 0;

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Passthrough vmMain(" << cmd << ") called\n";

        intptr_t ret = cgameinfo.vmMain(cmd, QMM_PUT_VMMAIN_ARGS());

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Passthrough vmMain(" << cmd << ") returning " << ret << "\n";

        // next call into combined mod DLL after GAME_SHUTDOWN should be CGAME_SHUTDOWN so unload mod now
        if (cgameinfo.is_shutdown) {
            // unload mod (dlclose)
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Shutting down mod\n";
            g_mod.Unload();
        }

        return ret;
    }

    // clear passthrough flag
    cgameinfo.is_from_QMM = false;

    // couldn't load engine info, so we will just call syscall(G_ERROR) to exit
    if (!gameinfo.game) {
        if (!gameinfo.is_shutdown) {
            gameinfo.is_shutdown = true;
            QMMLOG(QMM_LOG_FATAL, "QMM") << "QMM was unable to determine the game engine. Please set the \"game\" option in qmm2.json. Refer to the documentation for more information.\n";
            // if syscall passed to dllEntry was null, revert to std::exit because *shrug*
            if (!gameinfo.syscall) {
                printf("\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n");
                std::exit(-1);
            }
            gameinfo.syscall(QMM_FAIL_G_ERROR, "\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n");
        }
        return 0;
    }

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "vmMain(" << gameinfo.game->ModMsgName(cmd) << "(" << cmd << ")) called\n";

    if (cmd == GameInfo::msg_GAME_INIT) {
        // initialize our polyfill milliseconds tracker so that now is 0
        (void)util_get_milliseconds();

        // add engine G_PRINT logger (info level and above)
        log_add_sink([](const AixLog::Metadata& metadata, const std::string& message) {
                ENG_SYSCALL(GameInfo::msg_G_PRINT, log_format(metadata, message, false).c_str());
            },
            QMM2_LOG_CONSOLE_SEVERITY);

        QMMLOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ") initializing\n";

        // get mod dir from engine
        char moddir[256];
        ENG_SYSCALL(QMM_ENG_MSG(QMM_G_CVAR_VARIABLE_STRING_BUFFER), gameinfo.game->ModCvar(), moddir, sizeof(moddir));
        moddir[sizeof(moddir) - 1] = '\0';
        gameinfo.mod_dir = moddir;
        // the default mod (including all singleplayer games) returns "" for the fs_game, so grab the default mod dir from game info instead
        if (gameinfo.mod_dir.empty())
            gameinfo.mod_dir = gameinfo.game->DefaultModDir();

        QMMLOG(QMM_LOG_INFO, "QMM") << "Game: " << gameinfo.game->GameCode() << "/\"" << gameinfo.game->GameName() << "\" (Source: " << (gameinfo.is_auto_detected ? "Auto-detected" : "Config file") << ")\n";
        QMMLOG(QMM_LOG_INFO, "QMM") << "ModDir: " << gameinfo.mod_dir << "\n";
        QMMLOG(QMM_LOG_INFO, "QMM") << "Config file: \"" << gameinfo.cfg_path << "\" " << (g_cfg.is_discarded() ? "(error)" : "") << "\n";

        QMMLOG(QMM_LOG_INFO, "QMM") << "Built: " QMM_COMPILE " by " QMM_BUILDER "\n";
        QMMLOG(QMM_LOG_INFO, "QMM") << "URL: " QMM_URL "\n";

        // create qmm_version cvar
        ENG_SYSCALL(QMM_ENG_MSG(QMM_G_CVAR_REGISTER), nullptr, "qmm_version", "v" QMM_VERSION, QMM_ENG_MSG(QMM_CVAR_ROM) | QMM_ENG_MSG(QMM_CVAR_SERVERINFO));

        // load mod
        std::string cfg_mod = cfg_get_string(g_cfg, "mod", "auto");
        // check command line arguments for a mod filename
        cfg_mod = util_get_cmdline_arg("--qmm_mod", cfg_mod);
        if (!gameinfo.LoadMod(cfg_mod)) {
            if (!gameinfo.is_shutdown) {
                gameinfo.is_shutdown = true;
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
        if (cgameinfo.syscall) {
            cgameinfo.vmMain = (mod_vmMain)dll_symbol(g_mod.dll, "vmMain");
            QMMLOG(QMM_LOG_DEBUG, "QMM") << "Storing cgame vmMain = " << cgameinfo.vmMain << "\n";

            // pass original cgame syscall to dllEntry in mod
            mod_dllEntry pfndllEntry = (mod_dllEntry)dll_symbol(g_mod.dll, "dllEntry");
            QMMLOG(QMM_LOG_DEBUG, "QMM") << "Passing cgame syscall to dllEntry = " << pfndllEntry << "\n";
            pfndllEntry(cgameinfo.syscall);
        }

        // load plugins
        QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to load plugins\n";
        for (std::string& plugin_path : cfg_get_array_str(g_cfg, "plugins")) {
            QMMLOG(QMM_LOG_INFO, "QMM") << "Attempting to load plugin \"" << plugin_path << "\"...\n";
            if (gameinfo.LoadPlugin(plugin_path)) {
                QMMLOG(QMM_LOG_INFO, "QMM") << "Plugin \"" << plugin_path << "\" loaded\n";
            }
            else {
                QMMLOG(QMM_LOG_INFO, "QMM") << "Plugin \"" << plugin_path << "\" not loaded\n";
            }
        }
        QMMLOG(QMM_LOG_NOTICE, "QMM") << "Successfully loaded " << g_plugins.size() << " plugin(s)\n";

        // exec the qmmexec cfg
        std::string cfg_execcfg = cfg_get_string(g_cfg, "execcfg", "qmmexec.cfg");
        if (!cfg_execcfg.empty()) {
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Executing config file \"" << cfg_execcfg << "\"\n";
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_SEND_CONSOLE_COMMAND), QMM_ENG_MSG(QMM_EXEC_APPEND), fmt::format("exec {}\n", cfg_execcfg).c_str());
        }

        // we're done!
        QMMLOG(QMM_LOG_NOTICE, "QMM") << "Startup successful!\n";
    }

    else if (cmd == GameInfo::msg_GAME_CONSOLE_COMMAND) {
        char arg_cmd[10];
        int argn = 0;
        // get command
        ArgV(argn, arg_cmd, sizeof(arg_cmd));

        // if command is "sv", then get the next arg
        // idTech2 games use "sv" to run a gamedll command
        if (str_striequal("sv", arg_cmd)) {
            argn++;
            ArgV(argn, arg_cmd, sizeof(arg_cmd));
        }
        // check for "qmm" command
        if (str_striequal("qmm", arg_cmd) || str_striequal("/qmm", arg_cmd)) {
            // because of "sv", pass 0 or 1 which gets added to argn in the handler function
            HandleQMMCommand(argn);
            return 1;
        }
    }

    // route call to plugins and mod
    intptr_t ret = gameinfo.Route(false, cmd, args); // true = is_syscall

    // handle shut down (this is after the plugins and mod get called with GAME_SHUTDOWN)
    if (cmd == GameInfo::msg_GAME_SHUTDOWN) {
        QMMLOG(QMM_LOG_NOTICE, "QMM") << "Shutdown initiated!\n";

        // cgame passthrough hack:
        // hack to keep single player games shutting down correctly between levels/cutscenes/etc
        if (cgameinfo.syscall) {
            cgameinfo.is_shutdown = true;
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

    QMMLOG(QMM_LOG_TRACE, "QMM") << "vmMain(" << gameinfo.game->ModMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";

    return ret;
}


intptr_t qmm_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "syscall(" << gameinfo.game->EngMsgName(cmd) << "(" << cmd << ")) called\n";

    // route call to plugins and mod
    intptr_t ret = gameinfo.Route(true, cmd, args); // true = is_syscall

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "syscall(" << gameinfo.game->EngMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";

    return ret;
}


static void HandleQMMCommand(intptr_t arg_start) {
    char arg1[10] = "", arg2[10] = "";

    int argc = (int)ENG_SYSCALL(QMM_ENG_MSG(QMM_G_ARGC));
    ArgV(arg_start + 1, arg1, sizeof(arg1));
    if (argc > arg_start + 2)
        ArgV(arg_start + 2, arg2, sizeof(arg2));

    if (str_striequal("status", arg1) || str_striequal("info", arg1)) {
        CONSOLE_PRINT ("(QMM) QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ")\n");
        CONSOLE_PRINTF("(QMM) Game       : {}/\"{}\" ({}) (Source: {})\n", gameinfo.game->GameCode(), gameinfo.game->GameName(), APIType_Function(gameinfo.api), gameinfo.is_auto_detected ? "Auto-detected" : "Config file");
        CONSOLE_PRINTF("(QMM) ModDir     : {}\n", gameinfo.mod_dir);
        CONSOLE_PRINTF("(QMM) Config file: \"{}\" {}\n", gameinfo.cfg_path, g_cfg.empty() ? "(error)" : "");
        CONSOLE_PRINT ("(QMM) Built      : " QMM_COMPILE " by " QMM_BUILDER "\n");
        CONSOLE_PRINT ("(QMM) URL        : " QMM_URL "\n");
        CONSOLE_PRINT ("(QMM) PIFV       : " STRINGIFY(QMM_PIFV_MAJOR) ":" STRINGIFY(QMM_PIFV_MINOR) "\n");
        CONSOLE_PRINTF("(QMM) Plugins    : {}\n", g_plugins.size());
        CONSOLE_PRINTF("(QMM) Loaded mod : {} ({})\n", g_mod.path, APIType_Function(g_mod.api));
        if (g_mod.vm.memory) {
            CONSOLE_PRINT ("(QMM)\n");
            CONSOLE_PRINT ("(QMM) QVM mod information\n");
            CONSOLE_PRINT ("(QMM) -------------------\n");
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
    else if (str_striequal("list", arg1)) {
        CONSOLE_PRINT("(QMM) id - plugin [version]\n");
        CONSOLE_PRINT("(QMM) ---------------------\n");
        int num = 1;
        for (Plugin& p : g_plugins) {
            CONSOLE_PRINTF("(QMM) {:>2} - {} [{}]\n", num, p.plugininfo->name, p.plugininfo->version);
            num++;
        }
    }
    else if (str_striequal("plugin", arg1) || str_striequal("plugininfo", arg1)) {
        if (argc == arg_start + 2) {
            CONSOLE_PRINT("(QMM) qmm info <id> - outputs info on plugin with id\n");
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
    else if (str_striequal("loglevel", arg1)) {
        if (argc == arg_start + 2) {
            CONSOLE_PRINT("(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
            return;
        }
        int severity = log_severity_from_name(arg2);
        log_set_severity(severity);
        CONSOLE_PRINTF("(QMM) Log level set to {}\n", log_name_from_severity(severity));
    }
    else if (str_striequal("reload", arg1)) {
        g_cfg = cfg_load(gameinfo.cfg_path);
        CONSOLE_PRINT("(QMM) Configuration file reloaded!\n");
    }
    else if (str_striequal("credits", arg1) || str_striequal("thanks", arg1)) {
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
        CONSOLE_PRINT("(QMM) Usage: qmm <command> [params]\n");
        CONSOLE_PRINT("(QMM) Available commands:\n");
        CONSOLE_PRINT("(QMM) qmm info - displays information about QMM\n");
        CONSOLE_PRINT("(QMM) qmm list - displays information about loaded QMM plugins\n");
        CONSOLE_PRINT("(QMM) qmm plugin <id> - outputs info on plugin with id\n");
        CONSOLE_PRINT("(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
        CONSOLE_PRINT("(QMM) qmm reload - reloads the QMM configuration file\n");
        CONSOLE_PRINT("(QMM) qmm credits - QMM credits\n");
    }
}


void ArgV(intptr_t argn, char* buf, intptr_t buflen) {
    if (!buf || !buflen)
        return;

    // char* (*argv)(int argn);
    // void trap_Argv(int argn, char* buffer, int bufferSize);
    // some games don't return pointers because of QVM interaction, so if this returns anything but null
    // (or true?), we probably are in an api game, and need to get the arg from the return value instead
    intptr_t ret = gameinfo.game->syscall(gameinfo.game->QMMEngMsg(QMM_G_ARGV), argn, buf, buflen);
    if (ret > 1)
        strncpyz(buf, (const char*)ret, (size_t)buflen);
}


#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
C_DLLEXPORT void* GetCGameAPI(void* import) {
    // Q2R cgame hack:
    // if the game is already detected, then this is the later GetCGameAPI load which takes place in the menus after QMM
    // is loaded, so just get the return value from the mod's GetCGameAPI() function directly
    if (gameinfo.game) {
        // ??
        if (!g_mod.dll) {
            QMMLOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Mod DLL not loaded?\n";
            return nullptr;
        }
        QMMLOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Passing on call to mod DLL.\n";
        mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)dll_symbol(g_mod.dll, "GetCGameAPI");
        return pfnGCGA ? pfnGCGA(import, nullptr) : nullptr;
    }

    // client-side-only load. just get QMM file info and slap "qmm_" in front of the qmm filename
    gameinfo.DetectEnv();

    std::string modpath = fmt::format("{}/qmm_{}", gameinfo.qmm_dir, gameinfo.qmm_file);
    void* dll = dll_load(modpath.c_str());
    if (!dll)
        return nullptr;

    mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)dll_symbol(dll, "GetCGameAPI");

    // return CGame export from mod DLL
    // note we do not unload the DLL
    return pfnGCGA ? pfnGCGA(import, nullptr) : nullptr;
}
#endif // QMM_OS_WINDOWS && QMM_ARCH_64
