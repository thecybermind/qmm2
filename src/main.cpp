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
#include "qmm.hpp"
#include "plugin.hpp"   // g_plugins
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
 * something or know something, this is called with a particular "cmd" value and associated arguments.
 *
 * intptr_t qmm_syscall(intptr_t cmd, ...):
 * Primary entry point for actual game-related functions from mod->engine. Whenever the mod wants the engine to do
 * something or know something, this is called with a particular "cmd" value and associated arguments.
 */

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
    if (QMM::game && QMM::api == QMM_API_GETGAMEAPI) {
        QMM::CGame::syscall = syscall;
        QMMLOG(QMM_LOG_DEBUG, "QMM") << "Passthrough syscall = " << syscall << "\n";
        return;
    }

    // store the given syscall pointer as a backup.
    // this is used in case we couldn't detect a game and have to call syscall(G_ERROR) to shutdown in vmMain
    QMM::syscall = syscall;

    QMM::HandleEntry((void*)syscall, nullptr, QMM_API_DLLENTRY);
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
    return QMM::HandleEntry(import, extra, QMM_API_GETGAMEAPI);
}


C_DLLEXPORT void* GetModuleAPI(int apiversion, void* import) {
    return QMM::HandleEntry((void*)(intptr_t)apiversion, import, QMM_API_GETMODULEAPI);
}


C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

    // if this is a call from cgame and we need to pass this call onto the mod
    if (QMM::CGame::syscall) {
        // cancel if cgame portion of mod isn't actually loaded yet
        if (!QMM::CGame::vmMain)
            return 0;

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Passthrough vmMain(" << cmd << ") called\n";

        intptr_t ret = QMM::CGame::vmMain(cmd, QMM_PUT_VMMAIN_ARGS());

        QMMLOG(QMM_LOG_TRACE, "QMM") << "Passthrough vmMain(" << cmd << ") returning " << ret << "\n";

        // next call into combined mod DLL after GAME_SHUTDOWN should be CGAME_SHUTDOWN so unload mod now
        if (QMM::CGame::is_shutdown) {
            // unload mod (dlclose)
            QMMLOG(QMM_LOG_NOTICE, "QMM") << "Shutting down mod\n";
            g_mod.Unload();
        }

        return ret;
    }

    // couldn't load engine info, so we will just call syscall(G_ERROR) to exit
    if (!QMM::game) {
        if (!QMM::is_shutdown) {
            QMM::is_shutdown = true;
            QMMLOG(QMM_LOG_FATAL, "QMM") << "QMM was unable to determine the game engine. Please set the \"game\" option in qmm2.json. Refer to the documentation for more information.\n";
            // if syscall passed to dllEntry was null, revert to std::exit because *shrug*
            if (!QMM::syscall) {
                printf("\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n");
                std::exit(-1);
            }
            QMM::syscall(QMM::FAIL_G_ERROR, "\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n");
        }
        return 0;
    }

    return QMM::vmMain_args(cmd, args);
}


intptr_t qmm_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

    return QMM::syscall_args(cmd, args);
}


#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
GEN_GAME_EXTS(Q2R);
C_DLLEXPORT void* GetCGameAPI(void* import) {
    // Q2R cgame hack:
    // if the game is already detected, then this is the later GetCGameAPI load which takes place in the menus after QMM
    // is loaded, so just get the return value from the mod's GetCGameAPI() function directly
    if (QMM::game) {
        // ??
        if (!g_mod.dll) {
            QMMLOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Mod DLL not loaded?\n";
            return nullptr;
        }
        QMMLOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Passing on call to mod DLL.\n";
        mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)Util::dll_symbol(g_mod.dll, "GetCGameAPI");
        return pfnGCGA ? pfnGCGA(import, nullptr) : nullptr;
    }

    // client-side-only load. just load the default filename with "qmm_" in front
    QMM::game = Q2R_gamesupport;
    QMM::DetectEnv();

    std::string modpath = fmt::format("{}/qmm_{}", QMM::qmm_dir, QMM::game->DefaultDLLName());
    void* dll = Util::dll_load(modpath.c_str());
    if (!dll)
        return nullptr;

    mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)Util::dll_symbol(dll, "GetCGameAPI");

    // return CGame export from mod DLL
    // note we do not unload the DLL
    return pfnGCGA ? pfnGCGA(import, nullptr) : nullptr;
}
#endif // QMM_OS_WINDOWS && QMM_ARCH_64
