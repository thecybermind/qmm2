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
#include "format.hpp"
#include "qmmapi.h"
#include "game_api.hpp"
#include "gameinfo.hpp"
#include "config.hpp"
#include "main.hpp"         // qmm_syscall
#include "mod.hpp"
#include "qvm.h"
#include "plugin.hpp"
#include "util.hpp"

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
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::Load(\"{}\"): DLL load failed: {}\n", file, dll_error());
            goto fail;
        }

        // if this DLL is the same as QMM, cancel
        if (this->dll == gameinfo.qmm_module_ptr) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::Load(\"{}\"): DLL is actually QMM?\n", file);
            goto fail;
        }

        this->vmbase = 0;

        if (this->LoadDLL(QMM_API_GETGAMEAPI))
            return true;
        if (this->LoadDLL(QMM_API_GETMODULEAPI))
            return true;
        if (this->LoadDLL(QMM_API_DLLENTRY))
            return true;

        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::Load(\"{}\"): Unable to locate mod entry point\n", file);
        goto fail;
    }
    else {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::Load(\"{}\"): Unknown mod file format\n", file);
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
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("s_mod_vmmain({}): QVM unloaded during previous execution due to a run-time error\n", gameinfo.game->ModMsgName(cmd));
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
    return qvm_exec(&g_mod.vm, sizeof(qvmargs) / sizeof(qvmargs[0]), qvmargs);
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
    bool loaded;

    // load file using engine functions to read into pk3s if necessary
    filelen = ENG_SYSCALL(QMM_ENG_MSG(QMM_G_FS_FOPEN_FILE), this->path.c_str(), &fpk3, QMM_ENG_MSG(QMM_FS_READ));
    if (filelen <= 0 || !fpk3) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::LoadQVM(\"{}\"): Could not open QVM for reading\n", this->path);
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
    loaded = qvm_load(&this->vm, filemem.data(), filemem.size(), QVM_syscall, verify_data, nullptr);
    if (!loaded) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::LoadQVM(\"{}\"): QVM load failed\n", this->path);
        goto fail;
    }

    this->vmbase = (intptr_t)this->vm.datasegment;

    // pass the qvm vmMain function pointer to the game-specific mod load handler
    if (!gameinfo.game->ModLoad((void*)QVM_vmMain, QMM_API_QVM))
    {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::LoadQVM(\"{}\"): Mod load failed?\n", this->path);
        goto fail;
    }

    this->api = QMM_API_QVM;

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
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::LoadDLL(\"{}\"): Could not locate mod entry point \"{}\"\n", this->path, APIType_Function(dll_api));
            return false;
        }

        // pass GGA/GMA function to game-specific mod load handler
        if (gameinfo.game->ModLoad((void*)pfnGGA, dll_api)) {
            // if mod load handler says good to go, we do too
            return true;
        }

        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::LoadDLL(\"{}\"): {}_GameSupport::ModLoad returned false\n", this->path, gameinfo.game->GameCode());

        return false;
    }
    case QMM_API_DLLENTRY: {
        mod_dllEntry pfndllEntry = (mod_dllEntry)dll_symbol(this->dll, "dllEntry");
        mod_vmMain pfnvmMain = (mod_vmMain)dll_symbol(this->dll, "vmMain");
        if (!pfndllEntry || !pfnvmMain) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::LoadDLL(\"{}\"): Could not locate mod entry point \"dllEntry\" and/or \"vmMain\"\n", this->path);
            return false;
        }

        // pass vmMain to game-specific mod load handler
        if (gameinfo.game->ModLoad((void*)pfnvmMain, dll_api)) {
            // if mod load handler says good to go, we also need to pass qmm_syscall to mod's dllEntry function
            pfndllEntry(qmm_syscall);
            return true;
        }

        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("Mod::LoadDLL(\"{}\"): {}_GameSupport::ModLoad returned false\n", this->path, gameinfo.game->GameCode());

        return false;
    }
    default:
        return false;
    };
}
