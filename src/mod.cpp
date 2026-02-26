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
#include <cstring>
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
static bool s_mod_load_vmmain(mod& mod);
static bool s_mod_load_getgameapi(mod& mod);


bool mod_load(mod& mod, std::string file) {
    // if this mod somehow already has a dll or qvm pointer, wipe it first
    if (mod.dll || mod.qvm.memory)
        mod_unload(mod);

    mod.path = file;

    std::string ext = path_baseext(file);

    // only allow qvm mods if the game engine supports it
    if (str_striequal(ext, EXT_QVM) && g_gameinfo.game->pfnqvmsyscall)
        return s_mod_load_qvm(mod);

    // if DLL
    else {
        // load DLL
        if (!(mod.dll = dlopen(file.c_str(), RTLD_NOW))) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL load failed: {}\n", file, dlerror());
            return false;
        }

        // if this DLL is the same as QMM, cancel
        if ((void*)mod.dll == g_gameinfo.qmm_module_ptr) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL is actually QMM?\n", file);
            mod_unload(mod);
            return false;
        }

        // pass off to engine-specific loading function
        if (g_gameinfo.game->pfnGetGameAPI)
            return s_mod_load_getgameapi(mod);
        else
            return s_mod_load_vmmain(mod);
    }

    LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unknown file format\n", file);
    return false;
}


void mod_unload(mod& mod) {
    // call the game-specific mod unload callback
    if (g_gameinfo.game->pfnModUnload)
        g_gameinfo.game->pfnModUnload();
    qvm_unload(&mod.qvm);
    if (mod.dll)
        dlclose(mod.dll);
    mod = ::mod();
}


// entry point into QVM mods. stored in mod_t->pfnvmMain for QVM mods
static intptr_t s_mod_qvm_vmmain(intptr_t cmd, ...) {
    // if qvm isn't loaded, we need to error
    if (!g_mod.qvm.memory) {
        if (!g_gameinfo.isshutdown) {
            g_gameinfo.isshutdown = true;
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("s_mod_vmmain({}): QVM unloaded during previous execution due to a run-time error\n", g_gameinfo.game->mod_msg_names(cmd));
            ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], "\n\n=========\nFatal QMM Error:\nThe QVM was unloaded during previous execution due to a run-time error.\n=========\n");
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
    return qvm_exec(&g_mod.qvm, QVM_MAX_VMMAIN_ARGS + 1, qvmargs);
}


// handle syscalls from the QVM. passed to qvm_load
static int s_mod_qvm_syscall(uint8_t* membase, int cmd, int* args) {
    // check for plugin qvm function registration
    if (cmd >= QMM_QVM_FUNC_STARTING_ID && g_registered_qvm_funcs.count(cmd)) {
        plugin* p = g_registered_qvm_funcs[cmd];

        // make sure plugin has the handler function (shouldn't have been registered, but check anyway)
        if (!p->QMM_QVMHandler)
            return 0;

        // pass the negative-1 form since that's the number the plugin probably stored and expects
        return p->QMM_QVMHandler(-cmd - 1, args);
    }

    // if no game-specific qvm handler, we need to error
    if (!g_gameinfo.game->pfnqvmsyscall) {
        if (!g_gameinfo.isshutdown) {
            g_gameinfo.isshutdown = true;
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("s_mod_qvm_syscall({}): No QVM syscall handler found\n", g_gameinfo.game->eng_msg_names(cmd));
            ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], "\n\n=========\nFatal QMM Error:\nNo QVM syscall handler found.\n=========\n");
        }
        return 0;
    }

    // call the game-specific QVM syscall handler
    return g_gameinfo.game->pfnqvmsyscall(membase, cmd, args);
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
    loaded = qvm_load(&mod.qvm, filemem.data(), filemem.size(), s_mod_qvm_syscall, verify_data, nullptr);
    if (!loaded) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): QVM load failed\n", mod.path);
        goto fail;
    }

    mod.vmbase = (intptr_t)mod.qvm.datasegment;

    // pass the qvm vmMain function pointer to the game-specific mod load handler
    if (!g_gameinfo.game->pfnModLoad((void*)s_mod_qvm_vmmain)) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Mod load failed?\n", mod.path);
        goto fail;
    }

    return true;

fail:
    mod_unload(mod);
    return false;
}


// load a GetGameAPI DLL mod
static bool s_mod_load_getgameapi(mod& mod) {
    // look for GetGameAPI function
    mod_GetGameAPI pfnGGA = (mod_GetGameAPI)dlsym(mod.dll, "GetGameAPI");

    if (!pfnGGA) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"GetGameAPI\" function\n", mod.path);
        goto fail;
    }

    mod.vmbase = 0;

    // pass the GetGameAPI function pointer to the game-specific mod load handler
    if (!g_gameinfo.game->pfnModLoad((void*)pfnGGA)) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): \"GetGameAPI\" function failed\n", mod.path);
        goto fail;
    }

    return true;

fail:
    mod_unload(mod);
    return false;
}


// load a vmMain DLL mod
static bool s_mod_load_vmmain(mod& mod) {
    mod_dllEntry pfndllEntry = nullptr;
    mod_vmMain pfnvmMain = nullptr;

    // look for dllEntry function
    if (!(pfndllEntry = (mod_dllEntry)dlsym(mod.dll, "dllEntry"))) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"dllEntry\" function\n", mod.path);
        goto fail;
    }

    // look for vmMain function
    if (!(pfnvmMain = (mod_vmMain)dlsym(mod.dll, "vmMain"))) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"vmMain\" function\n", mod.path);
        goto fail;
    }

    // pass qmm_syscall to mod's dllEntry function
    pfndllEntry(qmm_syscall);
    
    mod.vmbase = 0;

    // pass the vmMain function pointer to the game-specific mod load handler
    if (!g_gameinfo.game->pfnModLoad((void*)pfnvmMain)) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Mod load failed?\n", mod.path);
        goto fail;
    }

    return true;

fail:
    mod_unload(mod);
    return false;
}
