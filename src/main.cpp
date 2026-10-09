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
#include "qmm.hpp"
#include "qmmapi.h"
#include "mod.hpp"
#include "util.hpp"     // used in 64-bit GetCGameAPI only


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

    (void)QMM::HandleEntry((void*)syscall, nullptr, QMM_API_DLLENTRY);
}


C_DLLEXPORT void* GetGameAPI(void* import, void* extra) {
    return QMM::HandleEntry(import, extra, QMM_API_GETGAMEAPI);
}


C_DLLEXPORT void* GetModuleAPI(int apiversion, void* import) {
    return QMM::HandleEntry((void*)(intptr_t)apiversion, import, QMM_API_GETMODULEAPI);
}


#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
C_DLLEXPORT void* GetCGameAPI(void* import) {
    // Q2R cgame hack:
    // if the game is already detected, then this is the later GetCGameAPI load which takes place in the menus after QMM
    // is loaded, or when hosting a listen server, so just get the return value from the mod's GetCGameAPI() function directly
    if (QMM::game) {
        // ??
        if (!Mod::dll) {
            QMMLOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Mod DLL not loaded?\n";
            return nullptr;
        }
        QMMLOG(QMM_LOG_DEBUG, "QMM") << "GetCGameAPI() called! Passing on call to mod DLL.\n";
        mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)Util::dll_symbol(Mod::dll, "GetCGameAPI");
        return pfnGCGA ? pfnGCGA(import, nullptr) : nullptr;
    }

    return QMM::HandleEntry(import, nullptr, QMM_API_GETCGAMEAPI);
}
#endif // QMM_OS_WINDOWS && QMM_ARCH_64


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
            Mod::Unload();
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
                puts("\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n");
                fputs("\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n", stderr);
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
