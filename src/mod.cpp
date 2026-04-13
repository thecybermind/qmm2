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


Mod::Mod() : vm({}), dll(nullptr), api(QMM_API_ERROR) {
}


Mod::~Mod() {
    this->Unload();
}


Mod::Mod(Mod&& other) noexcept : Mod() {
    if (this == &other)
        return;

    *this = std::move(other);
}


Mod& Mod::operator=(Mod&& other) noexcept {
    if (this == &other)
        return *this;

    // swap since a Mod "owns" the QVM
    std::swap(this->vm, other.vm);
    // swap since a Mod "owns" the dll handle
    std::swap(this->dll, other.dll);
    this->path = other.path;
    this->api = other.api;

    return *this;
}


bool Mod::Load(std::string file) {
    // if this mod somehow already has a dll or qvm pointer, wipe it first
    if (this->dll || this->vm.memory)
        this->Unload();

    std::string ext = path_baseext(file);

    // only allow qvm mods if the game engine supports it
    if (str_striequal(ext, EXT_QVM) && gameinfo.game->DefaultQVMName()) {
        return this->LoadQVM(file);
    }
    // if DLL
    else if (str_striequal(ext, EXT_DLL)) {
        // load DLL
        void* handle = dll_load(file.c_str());
        if (!handle) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << file << "\"): DLL load failed: " << dll_error() << "\n";
            return false;
        }

        // if this DLL is the same as QMM, cancel
        if (handle == gameinfo.qmm_module_ptr) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << path_basename(file) << "\"): DLL is actually QMM?\n";
            return false;
        }

        if (this->InitDLL(file, handle, QMM_API_GETGAMEAPI))
            return true;
        if (this->InitDLL(file, handle, QMM_API_GETMODULEAPI))
            return true;
        if (this->InitDLL(file, handle, QMM_API_DLLENTRY))
            return true;

        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << path_basename(file) << "\"): Unable to locate mod entry point\n";
    }
    else {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << path_basename(file) << "\"): Unknown mod file format\n";
    }

    return false;
}


void Mod::Unload() {
    // call the game-specific mod unload callback
    if (gameinfo.game)
        gameinfo.game->ModUnload();
    if (this->dll)
        dll_close(this->dll);
    qvm_unload(&this->vm);
    qvm_init(&this->vm);
    this->dll = nullptr;
    this->api = QMM_API_ERROR;
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


bool Mod::LoadQVM(std::string file) {
    EngineFileRead f;
    bool verify_data;
    size_t hunk_size;

    // load file using engine functions to read into pk3s if necessary
    uint8_t* filedata = f.Open(file);
    if (!filedata) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << file << "\"): Could not open QVM for reading\n";
        return false;
    }

    // get data verification setting from config
    verify_data = cfg_get_bool(g_cfg, "qvmverifydata", true);
    // get hunk size setting from config
    hunk_size = (size_t)cfg_get_int(g_cfg, "qvmhunksize", 0);

    // attempt to load mod
    if (!qvm_load(&this->vm, filedata, f.Size(), Mod::QVM_syscall, verify_data, hunk_size, nullptr)) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << file << "\"): QVM load failed\n";
        return false;
    }

    // pass the qvm vmMain function pointer to the game-specific mod load handler
    if (!gameinfo.game->ModLoad((void*)Mod::QVM_vmMain, QMM_API_QVM)) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << path_basename(file) << "\"): Mod load failed?\n";
        return false;
    }

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "Mod::LoadQVM(\"" << path_basename(file) << "\"): QVM loaded successfully with verify_data " << (this->vm.verify_data ? "on" : "off") << " and hunk size " << this->vm.hunksize << "\n";

    this->api = QMM_API_QVM;
    this->path = file;

    return true;
}


bool Mod::InitDLL(std::string file, void* handle, APIType dll_api) {
    switch (dll_api) {
    case QMM_API_GETGAMEAPI:
    case QMM_API_GETMODULEAPI: {
        // these are together because they work the same, just with a different function name

        // look for GetGameAPI/GetModuleAPI function
        mod_GetGameAPI pfnGGA = (mod_GetGameAPI)dll_symbol(handle, APIType_Function(dll_api));
        if (!pfnGGA) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << path_basename(file) << "\"): Could not locate mod entry point \"" << APIType_Function(dll_api) << "\"\n";
            return false;
        }

        // pass GGA/GMA function to game-specific mod load handler
        if (gameinfo.game->ModLoad((void*)pfnGGA, dll_api)) {
            // if mod load handler says good to go, we do too
            this->api = dll_api;
            this->dll = handle;
            this->path = file;
            return true;
        }

        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << path_basename(file) << "\"): " << gameinfo.game->GameCode() << "_GameSupport::ModLoad returned false\n";

        return false;
    }
    case QMM_API_DLLENTRY: {
        mod_dllEntry pfndllEntry = (mod_dllEntry)dll_symbol(handle, "dllEntry");
        if (!pfndllEntry) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << path_basename(file) << "\"): Could not locate mod entry point \"dllEntry\"\n";
            return false;
        }

        mod_vmMain pfnvmMain = (mod_vmMain)dll_symbol(handle, "vmMain");
        if (!pfnvmMain) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << path_basename(file) << "\"): Could not locate mod entry point \"vmMain\"\n";
            return false;
        }

        // pass vmMain to game-specific mod load handler
        if (gameinfo.game->ModLoad((void*)pfnvmMain, dll_api)) {
            // if mod load handler says good to go, we also need to pass qmm_syscall to mod's dllEntry function
            pfndllEntry(qmm_syscall);
            this->api = dll_api;
            this->dll = handle;
            this->path = file;
            return true;
        }

        QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << path_basename(file) << "\"): " << gameinfo.game->GameCode() << "_GameSupport::ModLoad returned false\n";

        return false;
    }
    default:
        return false;
    };
}
