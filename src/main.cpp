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

gameinfo g_gameinfo;

// cache some dynamic message values that get called a lot
static intptr_t msg_G_PRINT, msg_GAME_INIT, msg_GAME_CONSOLE_COMMAND, msg_GAME_SHUTDOWN;

static void s_main_detect_env();
static void s_main_load_config();
static void s_main_detect_game(std::string cfg_game, bool is_GetGameAPI_mode);
static bool s_main_load_mod(std::string cfg_mod);
static bool s_main_load_plugin(std::string plugin_path);
static void s_main_handle_command_qmm(intptr_t arg_start);
static intptr_t s_main_route_vmmain(intptr_t cmd, intptr_t* args);
static intptr_t s_main_route_syscall(intptr_t cmd, intptr_t* args);


/* About overall control flow for dllEntry/vmMain games:
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
   1. QVM system calls <GAME>_qvmsyscall function
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
*/

/* About cgame passthrough hack (not Quake 2 Remaster (Q2R), see comments before GetCGameAPI() for that):
   Some single player games, like Star Trek Voyager: Elite Force (STVOYSP), Jedi Knight 2 (JK2SP) and Jedi Academy (JASP),
   place the game (server side) and cgame (client side) in the same DLL. The game system uses GetGameAPI and the cgame
   system uses dllEntry/vmMain/syscall.

   Since we don't care about the cgame system, QMM will forward the dllEntry call to the mod (with the real cgame syscall
   pointer), and then forward all the incoming cgame vmMain calls directly to the mod's vmMain function.

   We do this with a few fields in the "cgame" struct. First, the "cgame.syscall" pointer variable is used to store the
   syscall pointer if dllEntry is called after QMM was already loaded from GetGameAPI. Then, dllEntry exits.

   Next, the "cgame.is_from_QMM" bool is used to flag incoming calls to vmMain as coming from a game-specific
   GetGameAPI vmMain wrapper struct (i.e. qmm_export) meaning the call actually came from the game system (as opposed to
   cgame). This flag is set in the GEN_EXPORT macro and all of the custom static polyfill functions that route to vmMain.
   It is set back to false immediately after checking and handling passthrough calls.

   Next, when the GAME_INIT event comes through, and we load the actual mod DLL, we also check to see if "cgame.syscall"
   is set. If it is, we look for "dllEntry" in the DLL, and pass "cgame.syscall" to it. Next, we look for "vmMain" in
   the DLL and then store it in the "cgame.vmMain" pointer.

   Whenever control enters vmMain and "cgame.is_from_QMM" is false (meaning it was called directly by the engine
   for the cgame system), it routes the call to the mod's vmMain stored in "cgame.vmMain".

   The final piece is that single player games shutdown and init the DLL a lot, particularly at every new level or
   between-level cutscene. When QMM detects that it is being shutdown and "cgame.syscall" is set, it no longer unloads
   the mod DLL and sets "cgame.shutdown" bool to true. Then, when a vmMain call is being handled as a passthrough, and
   "cgame.shutdown" is true, it will then unload the mod DLL. This allows the cgame system to shutdown properly.
*/
cgameinfo cgame = {
    nullptr,    // syscall
    nullptr,    // vmMain
    false,      // is_from_QMM
    false,      // shutdown
};


/* Entry point: engine->qmm
   This is the first function called when a vmMain DLL is loaded. The address of the engine's syscall callback is given,
   but it is not guaranteed to be initialized and ready for calling until vmMain() is called later. For now, all we can
   do is store the syscall, load the config file, and attempt to figure out what game engine we are in. This is either
   determined by the config file, or by getting the filename of the QMM DLL itself.
*/
C_DLLEXPORT void dllEntry(eng_syscall syscall) {
    // cgame passthrough hack:
    // QMM is already loaded, so this is a cgame passthrough situation. since the mod DLL isn't loaded yet, we can
    // just store the syscall pointer and pass it to the mod once it's loaded in vmMain(GAME_INIT)
    if (g_gameinfo.game) {
        cgame.syscall = syscall;
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QMM passthrough_syscall = {}\n", (void*)cgame.syscall);
        return;
    }

    s_main_detect_env();

    log_init(fmt::format("{}/qmm2.log", g_gameinfo.qmm_dir));

    LOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ") (dllEntry) loaded!\n";
    LOG(QMM_LOG_INFO, "QMM") << fmt::format("QMM path: \"{}\"\n", g_gameinfo.qmm_path);
    LOG(QMM_LOG_INFO, "QMM") << fmt::format("Engine path: \"{}\"\n", g_gameinfo.exe_path);
    LOG(QMM_LOG_INFO, "QMM") << fmt::format("Mod directory (?): \"{}\"\n", g_gameinfo.mod_dir);

    // ???
    if (!syscall) {
        LOG(QMM_LOG_FATAL, "QMM") << "dllEntry(): syscall is NULL!\n";
        return;
    }

    s_main_load_config();

    // update log severity for file output
    std::string cfg_loglevel = cfg_get_string(g_cfg, "loglevel", "");
    if (!cfg_loglevel.empty())
        log_set_severity(log_severity_from_name(cfg_loglevel));

    std::string cfg_game = cfg_get_string(g_cfg, "game", "auto");
    s_main_detect_game(cfg_game, false);	// false = not looking for GetGameAPI

    // failed to get engine information
    if (!g_gameinfo.game) {
        LOG(QMM_LOG_FATAL, "QMM") << fmt::format("dllEntry(): Unable to determine game engine using \"{}\"\n", cfg_game);
        return;
    }

    // supported games table is missing game-specific dllEntry handler?
    if (!g_gameinfo.game->pfndllEntry) {
        LOG(QMM_LOG_FATAL, "QMM") << fmt::format("dllEntry(): pfndllEntry handler for game \"{}\" is NULL!\n", g_gameinfo.game->gamename_short);
        return;
    }

    // now that the game is detected, cache some dynamic message values that get evaluated a lot
    msg_G_PRINT = QMM_ENG_MSG[QMM_G_PRINT];
    msg_GAME_INIT = QMM_MOD_MSG[QMM_GAME_INIT];
    msg_GAME_CONSOLE_COMMAND = QMM_MOD_MSG[QMM_GAME_CONSOLE_COMMAND];
    msg_GAME_SHUTDOWN = QMM_MOD_MSG[QMM_GAME_SHUTDOWN];

    // call the game-specific dllEntry function (e.g. Q3A_dllEntry) which will set up the functions to handle vmMain
    // and syscalls from the mod, engine, and plugins
    g_gameinfo.game->pfndllEntry(syscall);
}


/* About overall control flow for GetGameAPI games:
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
   g_gameinfo.pfnvmMain or g_gameinfo.pfnsyscall point to game-specific functions which will take the cmd, and route
   to the proper function pointer in the struct.
*/
C_DLLEXPORT void* GetGameAPI(void* import, void* extra) {
    s_main_detect_env();

    log_init(fmt::format("{}/qmm2.log", g_gameinfo.qmm_dir));

    LOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ") (GetGameAPI) loaded!\n";
    LOG(QMM_LOG_INFO, "QMM") << fmt::format("QMM path: \"{}\"\n", g_gameinfo.qmm_path);
    LOG(QMM_LOG_INFO, "QMM") << fmt::format("Engine path: \"{}\"\n", g_gameinfo.exe_path);
    LOG(QMM_LOG_INFO, "QMM") << fmt::format("Mod directory (?): \"{}\"\n", g_gameinfo.mod_dir);

    // ???
    // return nullptr to error out now, Init() will never be called
    if (!import) {
        LOG(QMM_LOG_FATAL, "QMM") << "GetGameAPI(): import is NULL!\n";
        return nullptr;
    }

    s_main_load_config();

    // update log severity for file output
    std::string cfg_loglevel = cfg_get_string(g_cfg, "loglevel", "");
    if (!cfg_loglevel.empty())
        log_set_severity(log_severity_from_name(cfg_loglevel));

    std::string cfg_game = cfg_get_string(g_cfg, "game", "auto");
    s_main_detect_game(cfg_game, true);	// true = looking for GetGameAPI

    // failed to get engine information
    // return nullptr to error out now, Init() will never be called
    if (!g_gameinfo.game) {
        LOG(QMM_LOG_FATAL, "QMM") << fmt::format("GetGameAPI(): Unable to determine game engine using \"{}\"\n", cfg_game);
        return nullptr;
    }

    // supported games table is missing game-specific GetGameAPI handler?
    if (!g_gameinfo.game->pfnGetGameAPI) {
        LOG(QMM_LOG_FATAL, "QMM") << fmt::format("GetGameAPI(): pfnGetGameAPI handler for game \"{}\" is NULL!\n", g_gameinfo.game->gamename_short);
        return nullptr;
    }

    // now that the game is detected, cache some dynamic message values that get evaluated a lot
    msg_G_PRINT = QMM_ENG_MSG[QMM_G_PRINT];
    msg_GAME_INIT = QMM_MOD_MSG[QMM_GAME_INIT];
    msg_GAME_CONSOLE_COMMAND = QMM_MOD_MSG[QMM_GAME_CONSOLE_COMMAND];
    msg_GAME_SHUTDOWN = QMM_MOD_MSG[QMM_GAME_SHUTDOWN];

    // call the game-specific GetGameAPI function (e.g. MOHAA_GetGameAPI) which will set up the exports for
    // returning here back to the game engine, as well as save the imports in preparation of loading the mod
    return g_gameinfo.game->pfnGetGameAPI(import, extra);
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
    if (cgame.syscall && !cgame.is_from_QMM) {
        // cancel if cgame portion of mod isn't actually loaded yet
        if (!cgame.vmMain)
            return 0;

#ifdef _DEBUG
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Passthrough vmMain({}) called\n", cmd);
#endif

        intptr_t ret = cgame.vmMain(cmd, QMM_PUT_VMMAIN_ARGS());

#ifdef _DEBUG
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Passthrough vmMain({}) returning {}\n", cmd, ret);
#endif

        if (cgame.shutdown) {
            // unload mod (dlclose)
            LOG(QMM_LOG_NOTICE, "QMM") << "Shutting down mod\n";
            mod_unload(g_mod);
        }

        return ret;
    }

    // clear passthrough flag
    cgame.is_from_QMM = false;

    // couldn't load engine info, so we will just call syscall(G_ERROR) to exit
    if (!g_gameinfo.game) {
        if (!g_gameinfo.isshutdown) {
            g_gameinfo.isshutdown = true;
            ENG_SYSCALL(QMM_FAIL_G_ERROR, "\n\n=========\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n=========\n");
        }
        return 0;
    }

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("vmMain({} {}) called\n", g_gameinfo.game->mod_msg_names(cmd), cmd);
#endif

    if (cmd == msg_GAME_INIT) {
        // initialize our polyfill milliseconds tracker so that now is 0
        (void)util_get_milliseconds();

        // add engine G_PRINT logger (info level)
        log_add_sink([](const AixLog::Metadata& metadata, const std::string& message) {
            ENG_SYSCALL(msg_G_PRINT, log_format(metadata, message, false).c_str());
            }, AixLog::Severity::info);

        LOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ") initializing\n";

        // get mod dir from engine
        char moddir[256];
        ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_STRING_BUFFER], "fs_game", moddir, sizeof(moddir));
        moddir[sizeof(moddir) - 1] = '\0';
        g_gameinfo.mod_dir = moddir;
        // the default mod (including all singleplayer games) returns "" for the fs_game, so grab the default mod dir from game info instead
        if (g_gameinfo.mod_dir.empty())
            g_gameinfo.mod_dir = g_gameinfo.game->moddir;

        LOG(QMM_LOG_INFO, "QMM") << fmt::format("Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file");
        LOG(QMM_LOG_INFO, "QMM") << fmt::format("ModDir: {}\n", g_gameinfo.mod_dir);
        LOG(QMM_LOG_INFO, "QMM") << fmt::format("Config file: \"{}\" {}\n", g_gameinfo.cfg_path, g_cfg.is_discarded() ? "(error)" : "");

        LOG(QMM_LOG_INFO, "QMM") << "Built: " QMM_COMPILE " by " QMM_BUILDER "\n";
        LOG(QMM_LOG_INFO, "QMM") << "URL: " QMM_URL "\n";

        // create qmm_version cvar
        ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_REGISTER], nullptr, "qmm_version", "v" QMM_VERSION, QMM_ENG_MSG[QMM_CVAR_ROM] | QMM_ENG_MSG[QMM_CVAR_SERVERINFO]);

        // load mod
        std::string cfg_mod = cfg_get_string(g_cfg, "mod", "auto");
        if (!s_main_load_mod(cfg_mod)) {
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("vmMain({}): Unable to load mod using \"{}\"\n", g_gameinfo.game->mod_msg_names(cmd), cfg_mod);
            return 0;
        }
        LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Successfully loaded {} mod \"{}\"\n", g_mod.vmbase ? "VM" : "DLL", g_mod.path);

        // cgame passthrough hack:
        // JASP (and others?) calls into the cgame syscall before a cgame vmMain GAME_INIT call is made
        // so if we have a passthrough situation, init the mod's cgame functions now
        if (cgame.syscall) {
            // pass original cgame syscall to dllEntry in mod
            mod_dllEntry pfndllEntry = (mod_dllEntry)dlsym(g_mod.dll, "dllEntry");
            pfndllEntry(cgame.syscall);
            LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Passing syscall passthrough to mod. dllEntry = {}\n", (void*)pfndllEntry);

            cgame.vmMain = (mod_vmMain)dlsym(g_mod.dll, "vmMain");
            LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Passthrough vmMain = {}\n", (void*)cgame.vmMain);
        }

        // load plugins
        LOG(QMM_LOG_INFO, "QMM") << "Attempting to load plugins\n";
        for (std::string& plugin_path : cfg_get_array_str(g_cfg, "plugins")) {
            LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to load plugin \"{}\"...\n", plugin_path);
            if (s_main_load_plugin(plugin_path))
                LOG(QMM_LOG_INFO, "QMM") << fmt::format("Plugin \"{}\" loaded\n", plugin_path);
            else
                LOG(QMM_LOG_INFO, "QMM") << fmt::format("Plugin \"{}\" not loaded\n", plugin_path);
        }
        LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Successfully loaded {} plugin(s)\n", g_plugins.size());

        // exec the qmmexec cfg
        std::string cfg_execcfg = cfg_get_string(g_cfg, "execcfg", "qmmexec.cfg");
        if (!cfg_execcfg.empty()) {
            LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Executing config file \"{}\"\n", cfg_execcfg);
            ENG_SYSCALL(QMM_ENG_MSG[QMM_G_SEND_CONSOLE_COMMAND], QMM_ENG_MSG[QMM_EXEC_APPEND], fmt::format("exec {}\n", cfg_execcfg).c_str());
        }

        // we're done!
        LOG(QMM_LOG_NOTICE, "QMM") << "Startup successful!\n";
    }

    else if (cmd == msg_GAME_CONSOLE_COMMAND) {
        char arg_cmd[10];
        int argn = 0;
        // get command
        qmm_argv(argn, arg_cmd, sizeof(arg_cmd));

        // if command is "sv", then get the next arg
        // idTech2 games use "sv" to run a gamedll command
        if (str_striequal("sv", arg_cmd)) {
            argn++;
            qmm_argv(argn, arg_cmd, sizeof(arg_cmd));
        }
        // check for "qmm" command
        if (str_striequal("qmm", arg_cmd) || str_striequal("/qmm", arg_cmd)) {
            // pass 0 or 1 which gets added to argn in the handler function
            s_main_handle_command_qmm(argn);
            return 1;
        }
    }

    // route call to plugins and mod
    intptr_t ret = s_main_route_vmmain(cmd, args);

    // handle shut down (this is after the plugins and mod get called with GAME_SHUTDOWN)
    if (cmd == msg_GAME_SHUTDOWN) {
        LOG(QMM_LOG_NOTICE, "QMM") << "Shutdown initiated!\n";

        // cgame passthrough hack:
        // hack to keep single player games shutting down correctly between levels/cutscenes/etc
        if (cgame.syscall) {
            cgame.shutdown = true;
            LOG(QMM_LOG_NOTICE, "QMM") << "Delaying shutting down mod so cgame shutdown can run\n";
        }
        else {
            // unload mod
            LOG(QMM_LOG_NOTICE, "QMM") << "Shutting down mod\n";
            mod_unload(g_mod);
        }

        // unload each plugin (call QMM_Detach, and then dlclose)
        LOG(QMM_LOG_NOTICE, "QMM") << "Shutting down plugins\n";
        for (plugin& p : g_plugins) {
            plugin_unload(p);
        }
        g_plugins.clear();

        LOG(QMM_LOG_NOTICE, "QMM") << "Finished shutting down\n";
    }

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("vmMain({} {}) returning {}\n", g_gameinfo.game->mod_msg_names(cmd), cmd, ret);
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
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("syscall({} {}) called\n", g_gameinfo.game->eng_msg_names(cmd), cmd);
#endif

    // route call to plugins and mod
    intptr_t ret = s_main_route_syscall(cmd, args);

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("syscall({} {}) returning {}\n", g_gameinfo.game->eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// get a given argument with G_ARGV, based on game engine type
void qmm_argv(intptr_t argn, char* buf, intptr_t buflen) {
    if (!buf || !buflen)
        return;
    
    // char* (*argv)(int argn);
    // void trap_Argv(int argn, char* buffer, int bufferSize);
    // some games don't return pointers because of QVM interaction, so if this returns anything but null
    // (or true?), we probably are in an api game, and need to get the arg from the return value instead
    intptr_t ret = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], argn, buf, buflen);
    if (ret > 1)
        strncpyz(buf, (const char*)ret, (size_t)buflen);
}


// general code to get path/module/binary/etc information
static void s_main_detect_env() {
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
static void s_main_load_config() {
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
static void s_main_detect_game(std::string cfg_game, bool is_GetGameAPI_mode) {
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
static bool s_main_load_mod(std::string cfg_mod) {
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
static bool s_main_load_plugin(std::string plugin_path) {
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
static void s_main_handle_command_qmm(intptr_t arg_start) {
    char arg1[10] = "", arg2[10] = "";

    int argc = (int)ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGC]);
    qmm_argv(arg_start + 1, arg1, sizeof(arg1));
    if (argc > arg_start + 2)
        qmm_argv(arg_start + 2, arg2, sizeof(arg2));

    if (str_striequal("status", arg1) || str_striequal("info", arg1)) {
        PRINT_CONSOLE("(QMM) QMM v" QMM_VERSION " (" QMM_OS " " QMM_ARCH ") loaded\n");
        PRINT_CONSOLE("(QMM) Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file");
        PRINT_CONSOLE("(QMM) ModDir: {}\n", g_gameinfo.mod_dir);
        PRINT_CONSOLE("(QMM) Config file: \"{}\" {}\n", g_gameinfo.cfg_path, g_cfg.is_discarded() ? " (error)" : "");
        PRINT_CONSOLE("(QMM) Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
        PRINT_CONSOLE("(QMM) URL: " QMM_URL "\n");
        PRINT_CONSOLE("(QMM) Plugin interface: {}:{}\n", QMM_PIFV_MAJOR, QMM_PIFV_MINOR);
        PRINT_CONSOLE("(QMM) Plugins loaded: {}\n", g_plugins.size());
        PRINT_CONSOLE("(QMM) Loaded mod file: {}\n", g_mod.path);
        if (g_mod.vmbase) {
            PRINT_CONSOLE("(QMM) QVM file size      : {}\n", g_mod.vm.filesize);
            PRINT_CONSOLE("(QMM) QVM memory base    : {}\n", (void*)g_mod.vm.memory);
            PRINT_CONSOLE("(QMM) QVM memory size    : {}\n", g_mod.vm.memorysize);
            PRINT_CONSOLE("(QMM) QVM instr count    : {}\n", g_mod.vm.instructioncount);
            PRINT_CONSOLE("(QMM) QVM codeseg size   : {}\n", g_mod.vm.codeseglen);
            PRINT_CONSOLE("(QMM) QVM dataseg size   : {}\n", g_mod.vm.dataseglen);
            PRINT_CONSOLE("(QMM) QVM stack size     : {}\n", g_mod.vm.stacksize);
            PRINT_CONSOLE("(QMM) QVM data validation: {}\n", g_mod.vm.verify_data ? "on" : "off");
        }
    }
    else if (str_striequal("list", arg1)) {
        PRINT_CONSOLE("(QMM) id - plugin [version]\n");
        PRINT_CONSOLE("(QMM) ---------------------\n");
        int num = 1;
        for (plugin& p : g_plugins) {
            PRINT_CONSOLE("(QMM) {:>2} - {} [{}]\n", num, p.plugininfo->name, p.plugininfo->version);
            num++;
        }
    }
    else if (str_striequal("plugin", arg1) || str_striequal("plugininfo", arg1)) {
        if (argc == arg_start + 2) {
            PRINT_CONSOLE("(QMM) qmm info <id> - outputs info on plugin with id\n");
            return;
        }
		size_t pid = (size_t)atoi(arg2);
        if (pid > 0 && pid <= g_plugins.size()) {
            plugin& p = g_plugins[pid - 1];
            PRINT_CONSOLE("(QMM) Plugin info for #{}:\n", arg2);
            PRINT_CONSOLE("(QMM) Name: {}\n", p.plugininfo->name);
            PRINT_CONSOLE("(QMM) Version: {}\n", p.plugininfo->version);
            PRINT_CONSOLE("(QMM) URL: {}\n", p.plugininfo->url);
            PRINT_CONSOLE("(QMM) Author: {}\n", p.plugininfo->author);
            PRINT_CONSOLE("(QMM) Desc: {}\n", p.plugininfo->desc);
            PRINT_CONSOLE("(QMM) Logtag: {}\n", p.plugininfo->logtag);
            PRINT_CONSOLE("(QMM) Interface version: {}:{}\n", p.plugininfo->pifv_major, p.plugininfo->pifv_minor);
            PRINT_CONSOLE("(QMM) Path: {}\n", p.path);
        }
        else {
            PRINT_CONSOLE("(QMM) Unable to find plugin #{}\n", arg2);
        }
    }
    else if (str_striequal("loglevel", arg1)) {
        if (argc == arg_start + 2) {
            PRINT_CONSOLE("(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
            return;
        }
        AixLog::Severity severity = log_severity_from_name(arg2);
        log_set_severity(severity);
        PRINT_CONSOLE("(QMM) Log level set to {}\n", log_name_from_severity(severity));
    }
    else {
        PRINT_CONSOLE("(QMM) Usage: qmm <command> [params]\n");
        PRINT_CONSOLE("(QMM) Available commands:\n");
        PRINT_CONSOLE("(QMM) qmm info - displays information about QMM\n");
        PRINT_CONSOLE("(QMM) qmm list - displays information about loaded QMM plugins\n");
        PRINT_CONSOLE("(QMM) qmm plugin <id> - outputs info on plugin with id\n");
        PRINT_CONSOLE("(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
    }
}


// route vmMain call to plugins and mod
static intptr_t s_main_route_vmmain(intptr_t cmd, intptr_t* args) {
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
static intptr_t s_main_route_syscall(intptr_t cmd, intptr_t* args) {
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


#ifdef _WIN64
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
    if (g_gameinfo.game) {
        // ??
        if (!g_mod.dll)
            return nullptr;
        LOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Passing on call to mod DLL.\n";
        mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)dlsym(g_mod.dll, "GetCGameAPI");
        return pfnGCGA ? pfnGCGA(import, nullptr) : nullptr;
    }

    // client-side-only load. just get file info and slap "qmm_" in front of the qmm filename
    s_main_detect_env();

    std::string modpath = fmt::format("{}/qmm_{}", g_gameinfo.qmm_dir, g_gameinfo.qmm_file);
    void* dll = dlopen(modpath.c_str(), RTLD_NOW);
    if (!dll)
        return nullptr;

    mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)dlsym(dll, "GetCGameAPI");

    // return CGame export from mod DLL
    // note we do not unload the DLL
    return pfnGCGA ? pfnGCGA(import, nullptr) : nullptr;
}
#endif // _WIN64
