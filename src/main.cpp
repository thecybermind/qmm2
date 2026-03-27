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
// #include <vector>
// #include <string>
#include "log.hpp"
#include "format.hpp"
#include "config.hpp"
#include "gameinfo.hpp"
// #include "game_api.hpp"
// #include "qmmapi.h"
#include "plugin.hpp"   // g_plugins
#include "main.hpp"     // ArgV
#include "mod.hpp"      // g_mod
#include "util.hpp"


// handle "qmm" console command
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

/* Entry point: engine->qmm
   This is the first function called when a vmMain DLL is loaded. The address of the engine's syscall callback is given,
   but it is not guaranteed to be initialized and ready for calling until vmMain() is called later. For now, all we can
   do is store the syscall, load the config file, and attempt to figure out what game engine we are in. This is either
   determined by the config file, or by getting the filename of the QMM DLL itself.
*/
C_DLLEXPORT void dllEntry(void* syscall) {
    // cgame passthrough hack:
    // QMM is already loaded, so this is a cgame passthrough situation. since the mod DLL isn't loaded yet, we can
    // just store the syscall pointer and pass it to the mod once it's loaded in vmMain(GAME_INIT)
    if (gameinfo.game && gameinfo.api == QMM_API_GETGAMEAPI) {
        cgameinfo.syscall = (eng_syscall)syscall;
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QMM passthrough_syscall = {}\n", syscall);
        return;
    }

    // store the given syscall pointer as a backup.
    // this is used in case we couldn't detect a game and have to call syscall(G_ERROR) to shutdown in vmMain
    gameinfo.syscall = (eng_syscall)syscall;

    gameinfo.HandleEntry(syscall, nullptr, QMM_API_DLLENTRY);
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
   7. return game_export_t pointer to engine
   8. engine stores qmm_export pointer, checks qmm_export->apiversion to match GAME_API_VERSION

   export (engine->mod) call flow
   1. call is handled by lambda inside game_export_t struct that was returned to engine
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


/* Entry point: engine->qmm
   This is the first function called when a GetGameAPI DLL is loaded. This system is based on the API model used by
   Quake 2, where a struct of function pointers is given from the engine to the mod, and the mod returns a struct of
   function pointers back to the engine.
   To best integrate this with QMM, game_xyz.cpp/.h create an enum for each import (syscall) and export (vmMain)
   function/variable.
   A game_export_t is given to the engine which has lambdas for each pointer that calls QMM's vmMain(enum, ...).
   A game_import_t is given to the mod which has lambdas for each pointer that calls QMM's syscall(enum, ...).

   The original import/export tables are stored. When QMM and plugins need to call the mod or engine,
   gameinfo.pfnvmMain or gameinfo.pfnsyscall point to game-specific functions which will take the cmd, and route
   to the proper function pointer in the struct.

   SOF2SP engine passes an apiversion as the first arg, and import is the second arg
*/
C_DLLEXPORT void* GetGameAPI(void* import, void* extra) {
    return gameinfo.HandleEntry(import, extra, QMM_API_GETGAMEAPI);
}


// this is the same as the 2-arg GetGameAPI but OpenJK renamed it
C_DLLEXPORT void* GetModuleAPI(void* import, void* extra) {
    return gameinfo.HandleEntry(import, extra, QMM_API_GETMODULEAPI);
}


/* Entry point: engine->qmm
   This is the "vmMain" function called by the engine as an entry point into the mod. First thing, we check if the
   game info is not stored. This means that the engine could not be determined, so we fail with G_ERROR and tell the
   user to set the game in the config file. If the engine was determined, it performs some internal tasks on a few
   events, and then routes the function call according to the "overall control flow" comment above.

   The internal events we track:
   GAME_INIT (pre): load mod file, load plugins, and optionally execute a cfg file
   GAME_CONSOLE_COMMAND (pre): handle "qmm" server command
   GAME_SHUTDOWN (post): handle game shutting down
*/
/* About syscall/mod constants (QMM_G_PRINT, QMM_GAME_INIT, etc):
   Because some engine syscall and mod entry point constants might change between games, we store arrays of all the ones
   QMM uses internally in each game's support file (game_XYZ.cpp). When the game is determined (automatically or via config),
   that game-specific arrays are accessed with the QMM_ENG_MSG and QMM_MOD_MSG macros and they are indexed with a QMM_
   constant, like: QMM_G_PRINT, QMM_GAME_CONSOLE_COMMAND, etc. This allows the code at point-of-use to be game-agnostic.
*/
// cache the dynamic msg values when we load the game so we aren't recalculating every vmMain call
C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

    // if this is a call from cgame and we need to pass this call onto the mod
    if (cgameinfo.syscall && !cgameinfo.is_from_QMM) {
        // cancel if cgame portion of mod isn't actually loaded yet
        if (!cgameinfo.vmMain)
            return 0;

#ifdef _DEBUG
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Passthrough vmMain({}) called\n", cmd);
#endif
        intptr_t ret = cgameinfo.vmMain(cmd, QMM_PUT_VMMAIN_ARGS());
#ifdef _DEBUG
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Passthrough vmMain({}) returning {}\n", cmd, ret);
#endif
        // next call into combined mod after GAME_SHUTDOWN should be CGAME_SHUTDOWN so unload mod now
        if (cgameinfo.is_shutdown) {
            // unload mod (dlclose)
            LOG(QMM_LOG_NOTICE, "QMM") << "Shutting down mod\n";
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
            LOG(QMM_LOG_FATAL, "QMM") << "QMM was unable to determine the game engine. Please set the \"game\" option in qmm2.json. Refer to the documentation for more information.";
            // if syscall passed to dllEntry was null, revert to std::exit because *shrug*
            if (!gameinfo.syscall) {
                printf("\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n");
                std::exit(-1);
            }
            gameinfo.syscall(QMM_FAIL_G_ERROR, "\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n");
        }
        return 0;
    }

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("vmMain({} {}) called\n", gameinfo.game->ModMsgName(cmd), cmd);
#endif

    if (cmd == msg_GAME_INIT) {
        // initialize our polyfill milliseconds tracker so that now is 0
        (void)util_get_milliseconds();

        // add engine G_PRINT logger (info level and above)
        log_add_sink([](const AixLog::Metadata& metadata, const std::string& message) {
            ENG_SYSCALL(msg_G_PRINT, log_format(metadata, message, false).c_str());
            }, AixLog::Severity::info);

        LOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ") initializing\n";

        // get mod dir from engine
        char moddir[256];
        ENG_SYSCALL(QMM_ENG_MSG(QMM_G_CVAR_VARIABLE_STRING_BUFFER), gameinfo.game->ModCvar(), moddir, sizeof(moddir));
        moddir[sizeof(moddir) - 1] = '\0';
        gameinfo.mod_dir = moddir;
        // the default mod (including all singleplayer games) returns "" for the fs_game, so grab the default mod dir from game info instead
        if (gameinfo.mod_dir.empty())
            gameinfo.mod_dir = gameinfo.game->DefaultModDir();

        LOG(QMM_LOG_INFO, "QMM") << fmt::format("Game: {}/\"{}\" (Source: {})\n", gameinfo.game->GameCode(), gameinfo.game->GameName(), gameinfo.is_auto_detected ? "Auto-detected" : "Config file");
        LOG(QMM_LOG_INFO, "QMM") << fmt::format("ModDir: {}\n", gameinfo.mod_dir);
        LOG(QMM_LOG_INFO, "QMM") << fmt::format("Config file: \"{}\" {}\n", gameinfo.cfg_path, g_cfg.is_discarded() ? "(error)" : "");

        LOG(QMM_LOG_INFO, "QMM") << "Built: " QMM_COMPILE " by " QMM_BUILDER "\n";
        LOG(QMM_LOG_INFO, "QMM") << "URL: " QMM_URL "\n";

        // create qmm_version cvar
        ENG_SYSCALL(QMM_ENG_MSG(QMM_G_CVAR_REGISTER), nullptr, "qmm_version", "v" QMM_VERSION, QMM_ENG_MSG(QMM_CVAR_ROM) | QMM_ENG_MSG(QMM_CVAR_SERVERINFO));

        // load mod
        std::string cfg_mod = cfg_get_string(g_cfg, "mod", "auto");
        // check command line arguments for a mod filename
        cfg_mod = util_get_cmdline_arg("--qmm_mod", cfg_mod);
        if (!gameinfo.LoadMod(cfg_mod)) {
            gameinfo.is_shutdown = true;
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("vmMain({}): Unable to load mod using \"{}\"\n", gameinfo.game->ModMsgName(cmd), cfg_mod);
            ENG_SYSCALL(QMM_FAIL_G_ERROR, "\nFatal QMM Error:\nQMM was unable to load the mod file.\nPlease set the \"mod\" option in qmm2.json.\nRefer to the documentation for more information.\n");
            return 0;
        }
        LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Successfully loaded {} mod \"{}\"\n", APIType_Function(g_mod.api), g_mod.path);

        // cgame passthrough hack:
        // mod DLL is loaded, so find the vmMain and dllEntry functions and call dllEntry.
        // JASP+JK2SP's cgame dllEntry functions actually call into the syscall almost immediately,
        // so make sure we store vmMain first in case there's some re-entrancy
        if (cgameinfo.syscall) {
            cgameinfo.vmMain = (mod_vmMain)dll_symbol(g_mod.dll, "vmMain");
            LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Storing cgame vmMain = {}\n", fmt::ptr(cgameinfo.vmMain));

            // pass original cgame syscall to dllEntry in mod
            mod_dllEntry pfndllEntry = (mod_dllEntry)dll_symbol(g_mod.dll, "dllEntry");
            LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Passing cgame syscall to dllEntry = {}\n", fmt::ptr(pfndllEntry));
            pfndllEntry(cgameinfo.syscall);
        }

        // load plugins
        LOG(QMM_LOG_INFO, "QMM") << "Attempting to load plugins\n";
        for (std::string& plugin_path : cfg_get_array_str(g_cfg, "plugins")) {
            LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to load plugin \"{}\"...\n", plugin_path);
            if (gameinfo.LoadPlugin(plugin_path))
                LOG(QMM_LOG_INFO, "QMM") << fmt::format("Plugin \"{}\" loaded\n", plugin_path);
            else
                LOG(QMM_LOG_INFO, "QMM") << fmt::format("Plugin \"{}\" not loaded\n", plugin_path);
        }
        LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Successfully loaded {} plugin(s)\n", g_plugins.size());

        // exec the qmmexec cfg
        std::string cfg_execcfg = cfg_get_string(g_cfg, "execcfg", "qmmexec.cfg");
        if (!cfg_execcfg.empty()) {
            LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Executing config file \"{}\"\n", cfg_execcfg);
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_SEND_CONSOLE_COMMAND), QMM_ENG_MSG(QMM_EXEC_APPEND), fmt::format("exec {}\n", cfg_execcfg).c_str());
        }

        // we're done!
        LOG(QMM_LOG_NOTICE, "QMM") << "Startup successful!\n";
    }

    else if (cmd == msg_GAME_CONSOLE_COMMAND) {
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
            // pass 0 or 1 which gets added to argn in the handler function
            HandleQMMCommand(argn);
            return 1;
        }
    }

    // route call to plugins and mod
    intptr_t ret = gameinfo.Route(false, cmd, args); // true = is_syscall

    // handle shut down (this is after the plugins and mod get called with GAME_SHUTDOWN)
    if (cmd == msg_GAME_SHUTDOWN) {
        LOG(QMM_LOG_NOTICE, "QMM") << "Shutdown initiated!\n";

        // cgame passthrough hack:
        // hack to keep single player games shutting down correctly between levels/cutscenes/etc
        if (cgameinfo.syscall) {
            cgameinfo.is_shutdown = true;
            LOG(QMM_LOG_NOTICE, "QMM") << "Delaying shutting down mod so cgame shutdown can run\n";
        }
        else {
            // unload mod
            LOG(QMM_LOG_NOTICE, "QMM") << "Shutting down mod\n";
            g_mod.Unload();
        }

        // unload each plugin (call QMM_Detach, and then dlclose)
        LOG(QMM_LOG_NOTICE, "QMM") << "Shutting down plugins\n";
        for (Plugin& p : g_plugins) {
            plugin_unload(p);
        }
        g_plugins.clear();

        LOG(QMM_LOG_NOTICE, "QMM") << "Finished shutting down\n";
    }

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("vmMain({} {}) returning {}\n", gameinfo.game->ModMsgName(cmd), cmd, ret);
#endif

    return ret;
}


/* Entry point: mod->qmm
   This is the "syscall" function called by the mod as a way to pass info to or get info from the engine.
   It routes the function call according to the "overall control flow" comment above.
   Named qmm_syscall to avoid conflict with POSIX syscall function
*/
intptr_t qmm_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("syscall({} {}) called\n", gameinfo.game->EngMsgName(cmd), cmd);
#endif

    // route call to plugins and mod
    intptr_t ret = gameinfo.Route(true, cmd, args); // true = is_syscall

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("syscall({} {}) returning {}\n", gameinfo.game->EngMsgName(cmd), cmd, ret);
#endif

    return ret;
}



// handle "qmm" console command
static void HandleQMMCommand(intptr_t arg_start) {
    char arg1[10] = "", arg2[10] = "";

    int argc = (int)ENG_SYSCALL(QMM_ENG_MSG(QMM_G_ARGC));
    ArgV(arg_start + 1, arg1, sizeof(arg1));
    if (argc > arg_start + 2)
        ArgV(arg_start + 2, arg2, sizeof(arg2));

    if (str_striequal("status", arg1) || str_striequal("info", arg1)) {
        CONSOLE_PRINT("(QMM) QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ")\n");
        CONSOLE_PRINTF("(QMM) Game: {}/\"{}\" ({}) (Source: {})\n", gameinfo.game->GameCode(), gameinfo.game->GameName(), APIType_Function(gameinfo.api), gameinfo.is_auto_detected ? "Auto-detected" : "Config file");
        CONSOLE_PRINTF("(QMM) ModDir: {}\n", gameinfo.mod_dir);
        CONSOLE_PRINTF("(QMM) Config file: \"{}\" {}\n", gameinfo.cfg_path, g_cfg.empty() ? "(error)" : "");
        CONSOLE_PRINT("(QMM) Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
        CONSOLE_PRINT("(QMM) URL: " QMM_URL "\n");
        CONSOLE_PRINT("(QMM) Plugin interface: " STRINGIFY(QMM_PIFV_MAJOR) ":" STRINGIFY(QMM_PIFV_MINOR) "\n");
        CONSOLE_PRINTF("(QMM) Plugins loaded: {}\n", g_plugins.size());
        CONSOLE_PRINTF("(QMM) Loaded mod: {} ({})\n", g_mod.path, APIType_Function(g_mod.api));
        if (g_mod.vmbase) {
            CONSOLE_PRINTF("(QMM) QVM magic number   : {:x} ({})\n", g_mod.vm.magic, g_mod.vm.magic == QVM_MAGIC ? "QVM_MAGIC" : "QVM_MAGIC_VER2");
            CONSOLE_PRINTF("(QMM) QVM file size      : {}\n", g_mod.vm.filesize);
            CONSOLE_PRINTF("(QMM) QVM memory base    : {}\n", fmt::ptr(g_mod.vm.memory));
            CONSOLE_PRINTF("(QMM) QVM memory size    : {}\n", g_mod.vm.memorysize);
            CONSOLE_PRINTF("(QMM) QVM instr count    : {}\n", g_mod.vm.instructioncount);
            CONSOLE_PRINTF("(QMM) QVM codeseg size   : {}\n", g_mod.vm.codeseglen);
            CONSOLE_PRINTF("(QMM) QVM dataseg size   : {}\n", g_mod.vm.dataseglen);
            CONSOLE_PRINTF("(QMM) QVM stack size     : {}\n", g_mod.vm.stacksize);
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
        AixLog::Severity severity = log_severity_from_name(arg2);
        log_set_severity(severity);
        CONSOLE_PRINTF("(QMM) Log level set to {}\n", log_name_from_severity(severity));
    }
    else if (str_striequal("reload", arg1)) {
        g_cfg = cfg_load(gameinfo.cfg_path);
        CONSOLE_PRINT("(QMM) Configuration file reloaded!\n");
    }
    else {
        CONSOLE_PRINT("(QMM) Usage: qmm <command> [params]\n");
        CONSOLE_PRINT("(QMM) Available commands:\n");
        CONSOLE_PRINT("(QMM) qmm info - displays information about QMM\n");
        CONSOLE_PRINT("(QMM) qmm list - displays information about loaded QMM plugins\n");
        CONSOLE_PRINT("(QMM) qmm plugin <id> - outputs info on plugin with id\n");
        CONSOLE_PRINT("(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
        CONSOLE_PRINT("(QMM) qmm reload - reloads the QMM configuration file\n");
    }
}


// get a given argument with G_ARGV, based on game engine type
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

/* Entry point: engine->qmm (Q2R only)
   Quake 2 Remastered has a heavily modified engine + API, and includes Game and CGame in the same DLL, with this GetCGameAPI as
   the entry point. This is the first function called when a Q2R mod DLL is loaded. The only thing the engine does at first,
   however, is get the export struct returned from this function, and then calls export->Init() and export->Shutdown(). After
   this, the engine unloads the mod (QMM). Then, it loads it again as a server DLL through GetGameAPI, and then when a game is
   started, it calls GetCGameAPI to load the DLL as a client. Since at this point, QMM has everything loaded already, it just
   grabs the function from g_mod.dll and gets the GetCGameAPI function easily.

   Since QMM does not care about CGame, we check if QMM has already loaded things. If it has, QMM has already loaded
   the mod DLL so we can easily route to the mod's GetCGameAPI function. If not, we load the mod DLL and find GetCGameAPI,
   pass the import pointer to it, and return the mod's export pointer. If QMM has not loaded things, then we have 2 situations:
   this is the initial load at startup where just Init() and Shutdown() are called, or the player is joining a remote server to
   play. Either way, QMM won't be loaded at all during this load so just do a quick DLL load, with no config, no logfile, etc.
*/
C_DLLEXPORT void* GetCGameAPI(void* import) {
    // Q2R cgame hack:
    // if the game is already detected, then this is the later GetCGameAPI load which takes place in the menus after QMM
    // is loaded, so just get the return value from the mod's GetCGameAPI() function directly
    if (gameinfo.game) {
        // ??
        if (!g_mod.dll) {
            LOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Mod DLL not loaded?\n";
            return nullptr;
        }
        LOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Passing on call to mod DLL.\n";
        mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)dll_symbol(g_mod.dll, "GetCGameAPI");
        return pfnGCGA ? pfnGCGA(import, nullptr) : nullptr;
    }

    // client-side-only load. just get file info and slap "qmm_" in front of the qmm filename
    main_detect_env();

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
