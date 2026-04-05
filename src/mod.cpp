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
#include "log.hpp"
#include "qmmapi.h"
#include "gameapi.hpp"
#include "gameinfo.hpp"
#include "config.hpp"
#include "main.hpp"         // qmm_syscall
#include "mod.hpp"
#include "qvm.h"
#include "plugin.hpp"
#include "util.hpp"

// The game mod
Mod g_mod;


bool Mod::Load(std::string file) {
    // if this mod somehow already has a dll or qvm pointer, wipe it first
    if (this->dll || this->vm.memory)
        this->Unload();

    this->path = file;

    std::string ext = path_baseext(file);

    // only allow qvm mods if the game engine supports it
    if (str_striequal(ext, EXT_QVM) && gameinfo.game->DefaultQVMName()) {
        return this->LoadQVM();
    }
    // if DLL
    else if (str_striequal(ext, EXT_DLL)) {
        // load DLL
        this->dll = dll_load(file.c_str());
        if (!this->dll) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << file << "\"): DLL load failed: " << dll_error() << "\n";
            goto fail;
        }

        // if this DLL is the same as QMM, cancel
        if (this->dll == gameinfo.qmm_module_ptr) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << path_basename(file) << "\"): DLL is actually QMM?\n";
            goto fail;
        }

        this->vmbase = 0;

        if (this->LoadDLL(QMM_API_GETGAMEAPI))
            return true;
        if (this->LoadDLL(QMM_API_GETMODULEAPI))
            return true;
        if (this->LoadDLL(QMM_API_DLLENTRY))
            return true;

        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << path_basename(file) << "\"): Unable to locate mod entry point\n";
        goto fail;
    }
    else {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << path_basename(file) << "\"): Unknown mod file format\n";
    }

fail:
    this->Unload();
    return false;
}


void Mod::Unload() {
    // call the game-specific mod unload callback
    gameinfo.game->ModUnload();
    qvm_unload(&this->vm);
    if (this->dll)
        dll_close(this->dll);
    *this = Mod();
}


intptr_t Mod::QVM_vmMain(intptr_t cmd, ...) {
    // if qvm isn't loaded, we need to error
    if (!g_mod.vm.memory) {
        if (!gameinfo.is_shutdown) {
            gameinfo.is_shutdown = true;
            QMMLOG(QMM_LOG_FATAL, "QMM") << "Mod::QVM_vmMain(" << gameinfo.game->ModMsgName(cmd) << "(" << cmd << ")): QVM unloaded during previous execution due to a run-time error\n";
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_ERROR), "\nFatal QMM Error:\nThe QVM was unloaded during previous execution due to a run-time error.\n");
        }
        return 0;
    }

    QMM_GET_VMMAIN_ARGS();

    // generate new 32-bit int array from the intptr_t args, and also include cmd at the front
    int qvmargs[QVM_MAX_VMMAIN_ARGS + 1] = { (int)cmd };
    for (int i = 0; i < QVM_MAX_VMMAIN_ARGS; i++) {
        qvmargs[i + 1] = (int)args[i];
    }

    // pass array and size to qvm
    int ret = qvm_exec(&g_mod.vm, sizeof(qvmargs) / sizeof(qvmargs[0]), qvmargs);

    // if qvm isn't loaded, we need to error
    if (!g_mod.vm.memory) {
        if (!gameinfo.is_shutdown) {
            gameinfo.is_shutdown = true;
            QMMLOG(QMM_LOG_FATAL, "QMM") << "Mod::QVM_vmMain(" << gameinfo.game->ModMsgName(cmd) << "(" << cmd << ")): QVM unloaded during execution due to a run-time error\n";
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_ERROR), "\nFatal QMM Error:\nThe QVM was unloaded during execution due to a run-time error.\n");
        }
        return 0;
    }
    return ret;
}


int Mod::QVM_syscall(uint8_t* membase, int cmd, int* args) {
    // check for plugin qvm function registration
    if (cmd >= QMM_QVM_FUNC_STARTING_ID && g_registered_qvm_funcs.count(cmd)) {
        Plugin* p = g_registered_qvm_funcs[cmd];

        // make sure plugin has the handler function (shouldn't have been registered, but check anyway)
        if (!p->QMM_QVMHandler)
            return 0;

        // pass the negative-1 form since that's the number the plugin probably stored and expects
        return p->QMM_QVMHandler(-cmd - 1, args);
    }

    // call the game-specific QVM syscall handler
    return gameinfo.game->QVMSyscall(membase, cmd, args);
}


bool Mod::LoadQVM() {
    int fpk3 = 0;
    intptr_t filelen;
    std::vector<uint8_t> filemem;
    bool verify_data;
    size_t hunk_size = 0;
    bool loaded;

    // load file using engine functions to read into pk3s if necessary
    filelen = ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FOPEN_FILE), this->path.c_str(), &fpk3, QMM_ENG_MSG(QMM_FS_READ));
    if (filelen <= 0 || !fpk3) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << this->path << "\"): Could not open QVM for reading\n";
        if (fpk3)
            ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FCLOSE_FILE), fpk3);
        goto fail;
    }
    filemem.resize((size_t)filelen);

    ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_READ), filemem.data(), filelen, fpk3);
    ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FCLOSE_FILE), fpk3);

    // get data verification setting from config
    verify_data = cfg_get_bool(g_cfg, "qvmverifydata", true);
    // get hunk size setting from config
    hunk_size = (size_t)cfg_get_int(g_cfg, "qvmhunksize", 0);

    // attempt to load mod
    loaded = qvm_load(&this->vm, filemem.data(), filemem.size(), Mod::QVM_syscall, verify_data, hunk_size, nullptr);
    if (!loaded) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << this->path << "\"): QVM load failed\n";
        goto fail;
    }

    this->vmbase = (intptr_t)this->vm.datasegment;

    // pass the qvm vmMain function pointer to the game-specific mod load handler
    if (!gameinfo.game->ModLoad((void*)Mod::QVM_vmMain, QMM_API_QVM))
    {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << path_basename(this->path) << "\"): Mod load failed?\n";
        goto fail;
    }

    this->api = QMM_API_QVM;

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "Mod::LoadQVM(\"" << path_basename(this->path) << "\"): QVM loaded successfully with verify_data " << (this->vm.verify_data ? "on" : "off") << " and hunk size " << this->vm.hunksize << "\n";

    return true;

fail:
    this->Unload();
    return false;
}


bool Mod::LoadDLL(APIType dll_api) {
    this->api = dll_api;

    switch (dll_api) {
    case QMM_API_GETGAMEAPI:
    case QMM_API_GETMODULEAPI: {
        // these are together because they work the same, just with a different function name

        // look for GetGameAPI/GetModuleAPI function
        mod_GetGameAPI pfnGGA = (mod_GetGameAPI)dll_symbol(this->dll, APIType_Function(dll_api));
        if (!pfnGGA) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadDLL(\"" << path_basename(this->path) << "\"): Could not locate mod entry point \"" << APIType_Function(dll_api) << "\"\n";
            return false;
        }

        // pass GGA/GMA function to game-specific mod load handler
        if (gameinfo.game->ModLoad((void*)pfnGGA, dll_api)) {
            // if mod load handler says good to go, we do too
            return true;
        }

        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadDLL(\"" << path_basename(this->path) << "\"): " << gameinfo.game->GameCode() << "_GameSupport::ModLoad returned false\n";

        return false;
    }
    case QMM_API_DLLENTRY: {
        mod_dllEntry pfndllEntry = (mod_dllEntry)dll_symbol(this->dll, "dllEntry");
        if (!pfndllEntry) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadDLL(\"" << path_basename(this->path) << "\"): Could not locate mod entry point \"dllEntry\"\n";
            return false;
        }

        mod_vmMain pfnvmMain = (mod_vmMain)dll_symbol(this->dll, "vmMain");
        if (!pfnvmMain) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadDLL(\"" << path_basename(this->path) << "\"): Could not locate mod entry point \"vmMain\"\n";
            return false;
        }

        // pass vmMain to game-specific mod load handler
        if (gameinfo.game->ModLoad((void*)pfnvmMain, dll_api)) {
            // if mod load handler says good to go, we also need to pass qmm_syscall to mod's dllEntry function
            pfndllEntry(qmm_syscall);
            return true;
        }

        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadDLL(\"" << path_basename(this->path) << "\"): " << gameinfo.game->GameCode() << "_GameSupport::ModLoad returned false\n";

        return false;
    }
    default:
        return false;
    };
}
