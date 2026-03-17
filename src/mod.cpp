/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <cstdint>
#include <vector>
#include <string>
#include "log.h"
#include "format.h"
#include "qmmapi.h"
#include "game_api.h"
#include "main.h"
#include "config.h"
#include "mod.h"
#include "qvm.h"
#include "plugin.h"
#include "util.h"

qmm_mod g_mod;

static intptr_t s_mod_qvm_vmmain(intptr_t cmd, ...);
static int s_mod_qvm_syscall(uint8_t* membase, int cmd, int* args);
static bool s_mod_load_qvm(qmm_mod& mod);
static bool s_mod_load_dll(qmm_mod& mod, APIType api);


bool mod_load(qmm_mod& mod, std::string file) {
    // if this mod somehow already has a dll or qvm pointer, wipe it first
    if (mod.dll || mod.vm.memory)
        mod_unload(mod);

    mod.path = file;

    std::string ext = path_baseext(file);

    // only allow qvm mods if the game engine supports it
    if (str_striequal(ext, EXT_QVM) && g_gameinfo.game->DefaultQVMName()) {
        return s_mod_load_qvm(mod);
    }
    // if DLL
    else if (str_striequal(ext, EXT_DLL)) {
        // load DLL
        mod.dll = dll_load(file.c_str());
        if (!mod.dll) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL load failed: {}\n", file, dll_error());
            goto fail;
        }

        // if this DLL is the same as QMM, cancel
        if (mod.dll == g_gameinfo.qmm_module_ptr) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL is actually QMM?\n", file);
            goto fail;
        }

        mod.vmbase = 0;

        if (s_mod_load_dll(mod, QMM_API_GETGAMEAPI))
            return true;
        if (s_mod_load_dll(mod, QMM_API_GETMODULEAPI))
            return true;
        if (s_mod_load_dll(mod, QMM_API_DLLENTRY))
            return true;

        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to locate mod entry point\n", file);
        goto fail;
    }
    else {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unknown mod file format\n", file);
    }

fail:
    mod_unload(mod);
    return false;
}


void mod_unload(qmm_mod& mod) {
    // call the game-specific mod unload callback
    g_gameinfo.game->ModUnload();
    qvm_unload(&mod.vm);
    if (mod.dll)
        dll_close(mod.dll);
    mod = qmm_mod();
}


// entry point into QVM mods. stored in mod_t->pfnvmMain for QVM mods
static intptr_t s_mod_qvm_vmmain(intptr_t cmd, ...) {
    // if qvm isn't loaded, we need to error
    if (!g_mod.vm.memory) {
        if (!g_gameinfo.is_shutdown) {
            g_gameinfo.is_shutdown = true;
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("s_mod_vmmain({}): QVM unloaded during previous execution due to a run-time error\n", g_gameinfo.game->ModMsgName(cmd));
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_ERROR), "\nFatal QMM Error:\nThe QVM was unloaded during previous execution due to a run-time error.\n");
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
    // check for plugin qvm function registration
    if (cmd >= QMM_QVM_FUNC_STARTING_ID && g_registered_qvm_funcs.count(cmd)) {
        qmm_plugin* p = g_registered_qvm_funcs[cmd];

        // make sure plugin has the handler function (shouldn't have been registered, but check anyway)
        if (!p->QMM_QVMHandler)
            return 0;

        // pass the negative-1 form since that's the number the plugin probably stored and expects
        return p->QMM_QVMHandler(-cmd - 1, args);
    }

    // call the game-specific QVM syscall handler
    return g_gameinfo.game->QVMSyscall(membase, cmd, args);
}


// load a QVM mod into the given qmm_mod object
static bool s_mod_load_qvm(qmm_mod& mod) {
    int fpk3 = 0;
    intptr_t filelen;
    std::vector<uint8_t> filemem;
    bool verify_data;
    bool loaded;

    // load file using engine functions to read into pk3s if necessary
    filelen = ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FOPEN_FILE), mod.path.c_str(), &fpk3, QMM_ENG_MSG(QMM_FS_READ));
    if (filelen <= 0 || !fpk3) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Could not open QVM for reading\n", mod.path);
        if (fpk3)
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FCLOSE_FILE), fpk3);
        goto fail;
    }
    filemem.resize((size_t)filelen);

    ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_READ), filemem.data(), filelen, fpk3);
    ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FCLOSE_FILE), fpk3);

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
    if (!g_gameinfo.game->ModLoad((void*)s_mod_qvm_vmmain, QMM_API_QVM))
    {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Mod load failed?\n", mod.path);
        goto fail;
    }

    mod.api = QMM_API_QVM;

    return true;

fail:
    mod_unload(mod);
    return false;
}


// attempt to load a DLL mod with the given api type into the given qmm_mod object
static bool s_mod_load_dll(qmm_mod& mod, APIType api) {
    switch (api) {
    case QMM_API_GETGAMEAPI:
    case QMM_API_GETMODULEAPI: {
        // these are together because they work the same, just with a different function name

        // look for GetGameAPI/GetModuleAPI function
        mod_GetGameAPI pfnGGA = (mod_GetGameAPI)dll_symbol(mod.dll, APIType_Function(api));
        if (!pfnGGA)
            return false;

        // if mod load handler says good to go, we do too
        if (g_gameinfo.game->ModLoad((void*)pfnGGA, api)) {
            mod.api = api;
            return true;
        }

        return false;
    }
    case QMM_API_DLLENTRY: {
        mod_dllEntry pfndllEntry = (mod_dllEntry)dll_symbol(mod.dll, "dllEntry");
        mod_vmMain pfnvmMain = (mod_vmMain)dll_symbol(mod.dll, "vmMain");
        if (!pfndllEntry || !pfnvmMain)
            return false;

        // pass vmMain to game-specific mod load handler
        if (g_gameinfo.game->ModLoad((void*)pfnvmMain, api)) {
            // pass qmm_syscall to mod's dllEntry function
            pfndllEntry(qmm_syscall);
            mod.api = api;
            return true;
        }

        return false;
    }
    default:
        return false;
    };
}
