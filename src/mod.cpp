/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "osdef.h"
#include <cstdint>
#include <vector>
#include <string>
#include "log.h"
#include "qmmapi.h"
#include "game_api.h"
#include "main.h"
#include "config.h"
#include "mod.h"
#include "qvm.h"
#include "plugin.h"
#include "util.h"

mod g_mod;

static intptr_t s_mod_qvm_vmmain(intptr_t cmd, ...);
static int s_mod_qvm_syscall(uint8_t* membase, int cmd, int* args);
static bool s_mod_load_qvm(mod& mod);


bool mod_load(mod& mod, std::string file) {
    // if this mod somehow already has a dll or qvm pointer, wipe it first
    if (mod.dll || mod.vm.memory)
        mod_unload(mod);

    mod.path = file;

    std::string ext = path_baseext(file);

    // only allow qvm mods if the game engine supports it
    if (str_striequal(ext, EXT_QVM) && g_gameinfo.game->funcs->pfnqvmsyscall)
        return s_mod_load_qvm(mod);

    // if DLL
    else if (str_striequal(ext, EXT_DLL)) {
        // load DLL
        if (!(mod.dll = dlopen(file.c_str(), RTLD_NOW))) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL load failed: {}\n", file, dlerror());
            return false;
        }

        // if this DLL is the same as QMM, cancel
        if (mod.dll == g_gameinfo.qmm_module_ptr) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL is actually QMM?\n", file);
            dlclose(mod.dll);
            return false;
        }

        mod.vmbase = 0;

        // if game supports GetGameAPI, look for GetGameAPI function
        if (g_gameinfo.game->funcs->pfnGetGameAPI) {
            mod_GetGameAPI pfnGGA = (mod_GetGameAPI)dlsym(mod.dll, "GetGameAPI");

            // try for "GetModuleAPI", which is what OpenJK uses
            if (!pfnGGA) {
                pfnGGA = (mod_GetGameAPI)dlsym(mod.dll, "GetModuleAPI");
            }

            if (pfnGGA) {
                // pass the GetGameAPI function pointer to the game-specific mod load handler
                if (g_gameinfo.game->funcs->pfnModLoad &&
                    g_gameinfo.game->funcs->pfnModLoad((void*)pfnGGA, true)) {     // true = is_GetGameAPI
                    return true;
                }
                else {
                    LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): \"GetGameAPI\" function failed\n", mod.path);
                }
            }
            else {
                LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"GetGameAPI\" function\n", mod.path);
            }
        }

        // if game supports dllEntry, look for dllEntry function
        if (g_gameinfo.game->funcs->pfndllEntry) {
            mod_dllEntry pfndllEntry = (mod_dllEntry)dlsym(mod.dll, "dllEntry");
            mod_vmMain pfnvmMain = (mod_vmMain)dlsym(mod.dll, "vmMain");

            if (pfndllEntry && pfnvmMain) {
                if (g_gameinfo.game->funcs->pfnModLoad) {
                    // pass the vmMain function pointer to the game-specific mod load handler
                    if (g_gameinfo.game->funcs->pfnModLoad((void*)pfnvmMain, false)) {   // false = !is_GetGameAPI
                        // pass qmm_syscall to mod's dllEntry function
                        pfndllEntry(qmm_syscall);
                        return true;
                    }
                    else {
                        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Mod load failed?\n", mod.path);
                    }
                }
                // hack in case there isn't a game-specific dllEntry or ModLoad function
                else if (!g_gameinfo.pfnvmMain) {
                    g_gameinfo.pfnvmMain = pfnvmMain;
                }
            }
            else {
                LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"dllEntry\" and/or \"vmMain\" function\n", mod.path);
            }
        }
    }

    mod_unload(mod);

    LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unknown file format\n", file);
    return false;
}


void mod_unload(mod& mod) {
    // call the game-specific mod unload callback
    if (g_gameinfo.game->funcs->pfnModUnload)
        g_gameinfo.game->funcs->pfnModUnload();
    qvm_unload(&mod.vm);
    if (mod.dll)
        dlclose(mod.dll);
    mod = ::mod();
}


// entry point into QVM mods. stored in mod_t->pfnvmMain for QVM mods
static intptr_t s_mod_qvm_vmmain(intptr_t cmd, ...) {
    // if qvm isn't loaded, we need to error
    if (!g_mod.vm.memory) {
        if (!g_gameinfo.is_shutdown) {
            g_gameinfo.is_shutdown = true;
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("s_mod_vmmain({}): QVM unloaded during previous execution due to a run-time error\n", g_gameinfo.game->funcs->pfnModMsgNames(cmd));
            ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], "\nFatal QMM Error:\nThe QVM was unloaded during previous execution due to a run-time error.\n");
        }
        return 0;
    }

    QMM_GET_VMMAIN_ARGS();

    // generate new int array from the intptr_t args, and also include cmd at the front
    int qvmargs[QVM_MAX_VMMAIN_ARGS + 1] = { (int)cmd };
    for (int i = 0; i < QVM_MAX_VMMAIN_ARGS; i++) {
        qvmargs[i + 1] = (int)args[i];
    }

    // pass array and size to qvm
    return qvm_exec(&g_mod.vm, QVM_MAX_VMMAIN_ARGS + 1, qvmargs);
}


// handle syscalls from the QVM. passed to qvm_load
static int s_mod_qvm_syscall(uint8_t* membase, int cmd, int* args) {
    // if no game-specific qvm handler, we need to error
    if (!g_gameinfo.game->funcs->pfnqvmsyscall) {
        if (!g_gameinfo.is_shutdown) {
            g_gameinfo.is_shutdown = true;
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("s_mod_qvm_syscall({}): No QVM syscall handler found\n", g_gameinfo.game->funcs->pfnEngMsgNames(cmd));
            ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], "\nFatal QMM Error:\nNo QVM syscall handler found.\n");
        }
        return 0;
    }

    // check for plugin qvm function registration
    if (cmd >= QMM_QVM_FUNC_STARTING_ID && g_registered_qvm_funcs.count(cmd)) {
        plugin* p = g_registered_qvm_funcs[cmd];

        // make sure plugin has the handler function (shouldn't have been registered, but check anyway)
        if (!p->QMM_QVMHandler)
            return 0;

        // pass the negative-1 form since that's the number the plugin probably stored and expects
        return p->QMM_QVMHandler(-cmd - 1, args);
    }

    // call the game-specific QVM syscall handler
    return g_gameinfo.game->funcs->pfnqvmsyscall(membase, cmd, args);
}


// load a QVM mod
static bool s_mod_load_qvm(mod& mod) {
    int fpk3 = 0;
    intptr_t filelen;
    std::vector<uint8_t> filemem;
    bool verify_data;
    bool loaded;

    // load file using engine functions to read into pk3s if necessary
    filelen = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FOPEN_FILE], mod.path.c_str(), &fpk3, QMM_ENG_MSG[QMM_FS_READ]);
    if (filelen <= 0 || !fpk3) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Could not open QVM for reading\n", mod.path);
        if (fpk3)
            ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);
        goto fail;
    }
    filemem.resize((size_t)filelen);

    ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_READ], filemem.data(), filelen, fpk3);
    ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);

    // get data verification setting from config
    verify_data = cfg_get_bool(g_cfg, "qvmverifydata", true);

    // attempt to load mod
    loaded = qvm_load(&mod.vm, filemem.data(), filemem.size(), s_mod_qvm_syscall, verify_data, nullptr);
    if (!loaded) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): QVM load failed\n", mod.path);
        goto fail;
    }

    mod.vmbase = (intptr_t)mod.vm.datasegment;

    // pass the qvm vmMain function pointer to the game-specific mod load handler
    if (g_gameinfo.game->funcs->pfnModLoad &&
        !g_gameinfo.game->funcs->pfnModLoad((void*)s_mod_qvm_vmmain, false))	// false = !is_GetGameAPI
    {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Mod load failed?\n", mod.path);
        goto fail;
    }

    // hack in case there isn't a game-specific dllEntry or ModLoad function
    if (!g_gameinfo.pfnvmMain) {
        g_gameinfo.pfnvmMain = s_mod_qvm_vmmain;
    }

    return true;

fail:
    mod_unload(mod);
    return false;
}
