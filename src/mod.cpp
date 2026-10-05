/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <cstdint>
#include <string>
#include "log.hpp"
#include "qmmapi.h"
#include "gameapi.hpp"
#include "qmm.hpp"
#include "config.hpp"
#include "mod.hpp"
#include "plugin.hpp"       // g_plugins
#include "main.hpp"         // qmm_syscall
#include "qvm.h"
#include "util.hpp"


namespace Mod {
    qvm vm = {};
    void* dll = nullptr;
    std::string path;
    APIType api = QMM_API_UNKNOWN;

    intptr_t QVM_vmMain(intptr_t cmd, ...);
    int QVM_syscall(uint8_t* membase, int cmd, int* args);
    bool LoadQVM(std::string file);
    bool InitDLL(std::string file, void* handle, APIType dll_api);


    bool Load(std::string file, APIType mod_api) {
        // if this mod somehow already has a dll or qvm pointer, wipe it first
        if (dll || vm.memory)
            Unload();

        std::string ext = Util::path_baseext(file);

        // only allow qvm mods if the game engine supports it
        if (Util::str_striequal(ext, EXT_QVM) && QMM::game->DefaultQVMName()) {
            return LoadQVM(file);
        }
        // if DLL
        else if (Util::str_striequal(ext, EXT_DLL)) {
            // load DLL
            void* handle = Util::dll_load(file.c_str());
            if (!handle) {
                QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << file << "\"): DLL load failed: " << Util::dll_error() << "\n";
                return false;
            }

            // if this DLL is the same as QMM, cancel
            if (handle == QMM::qmm_module_ptr) {
                QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << Util::path_basename(file) << "\"): DLL is actually QMM?\n";
                Util::dll_close(handle);
                return false;
            }

            if (mod_api == QMM_API_UNKNOWN) {
                if (InitDLL(file, handle, QMM_API_GETGAMEAPI))
                    return true;
                if (InitDLL(file, handle, QMM_API_GETMODULEAPI))
                    return true;
                if (InitDLL(file, handle, QMM_API_DLLENTRY))
                    return true;

                Util::dll_close(handle);
                return false;
            }
            else if (InitDLL(file, handle, mod_api)) {
                return true;
            }

            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << Util::path_basename(file) << "\"): Unable to locate a valid mod entry point\n";
            Util::dll_close(handle);
            return false;
        }
        else {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::Load(\"" << Util::path_basename(file) << "\"): Unknown mod file format\n";
        }

        return false;
    }


    void Unload() {
        // call the game-specific mod unload callback only if a mod was actually loaded
        if (QMM::game && (dll || vm.memory))
            QMM::game->ModUnload(api);
        Util::dll_close(dll);
        dll = nullptr;
        qvm_unload(&vm);
        qvm_init(&vm);
        api = QMM_API_UNKNOWN;
    }


    // Entry point into QVM mods. Passed to GameSupport::ModLoad as if it were a DLL's vmMain function
    intptr_t QVM_vmMain(intptr_t cmd, ...) {
        // if qvm isn't loaded, we need to error
        if (!vm.memory) {
            if (!QMM::is_shutdown) {
                QMM::is_shutdown = true;
                QMMLOG(QMM_LOG_FATAL, "QMM") << "Mod::QVM_vmMain(" << QMM::game->ModMsgName(cmd) << "(" << cmd << ")): QVM unloaded during previous execution due to a run-time error\n";
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
        int ret = qvm_exec(&vm, sizeof(qvmargs) / sizeof(qvmargs[0]), qvmargs);

        // if qvm isn't loaded, we need to error
        if (!vm.memory) {
            if (!QMM::is_shutdown) {
                QMM::is_shutdown = true;
                QMMLOG(QMM_LOG_FATAL, "QMM") << "Mod::QVM_vmMain(" << QMM::game->ModMsgName(cmd) << "(" << cmd << ")): QVM unloaded during execution due to a run-time error\n";
                ENG_SYSCALL(QMM_ENG_MSG(QMM_G_ERROR), "\nFatal QMM Error:\nThe QVM was unloaded during execution due to a run-time error.\n");
            }
            return 0;
        }
        return ret;
    }


    // Exit point from QVM mods. Handle syscalls from the QVM.
    int QVM_syscall(uint8_t* membase, int cmd, int* args) {
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
        return QMM::game->QVMSyscall(membase, cmd, args);
    }


    /**
    * @brief Attempt to load a QVM mod
    *
    * @param file Path to QVM mod file
    * @return true if mod load was successful, false otherwise
    */
    bool LoadQVM(std::string file) {
        QMM::EngineFileRead f;       // read QVM file using engine functions to see into .pk3s
        bool verify_data;
        size_t hunk_size;

        // load file using engine functions to read into pk3s if necessary
        uint8_t* filedata = f.Open(file);
        if (!filedata) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << file << "\"): Could not open QVM for reading\n";
            return false;
        }

        // get data verification setting from config
        verify_data = Config::cfg_get_bool(g_cfg, "qvmverifydata", true);
        // get hunk size setting from config
        hunk_size = (size_t)Config::cfg_get_int(g_cfg, "qvmhunksize", 0);

        // attempt to load mod
        if (!qvm_load(&vm, filedata, f.Size(), Mod::QVM_syscall, verify_data, hunk_size, nullptr)) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << file << "\"): QVM load failed\n";
            return false;
        }

        // pass the qvm vmMain function pointer to the game-specific mod load handler
        if (!QMM::game->ModLoad((void*)Mod::QVM_vmMain, QMM_API_QVM)) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::LoadQVM(\"" << Util::path_basename(file) << "\"): Mod load failed?\n";
            // call ModUnload to allow game support code to reset
            QMM::game->ModUnload(QMM_API_QVM);
            return false;
        }

        QMMLOG(QMM_LOG_DEBUG, "QMM") << "Mod::LoadQVM(\"" << Util::path_basename(file) << "\"): QVM loaded successfully with verify_data " << (vm.verify_data ? "on" : "off") << " and hunk size " << vm.hunksize << "\n";

        api = QMM_API_QVM;
        path = file;

        return true;
    }


    /**
    * @brief Attempt to initialize Mod::dll as a DLL mod with the given API type
    *
    * @param file Path to DLL mod file
    * @param handle Pointer to loaded DLL
    * @param dll_api API type to load the mod
    * @return true if mod load was successful, false otherwise
    */
    bool InitDLL(std::string file, void* handle, APIType dll_api) {
        switch (dll_api) {
        case QMM_API_GETGAMEAPI:
        case QMM_API_GETMODULEAPI:
        case QMM_API_GETCGAMEAPI: {
            // these are together because they work the same, just with a different function name

            // look for GetGameAPI/GetModuleAPI/GetCGameAPI function
            mod_GetGameAPI pfnGGA = (mod_GetGameAPI)Util::dll_symbol(handle, APIType_Function(dll_api));
            if (!pfnGGA) {
                QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << Util::path_basename(file) << "\", " << APIType_Name(dll_api) << "): Could not locate mod entry point \"" << APIType_Function(dll_api) << "\"\n";
                return false;
            }

            // pass GGA/GMA function to game-specific mod load handler
            if (!QMM::game->ModLoad((void*)pfnGGA, dll_api)) {
                // if it failed, call ModUnload to allow game support code to reset
                QMM::game->ModUnload(dll_api);

                QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << Util::path_basename(file) << "\", " << APIType_Name(dll_api) << "): " << QMM::game->GameCode() << "_GameSupport::ModLoad returned false\n";

                return false;
            }

            break;
        }
        case QMM_API_DLLENTRY: {
            mod_dllEntry pfndllEntry = (mod_dllEntry)Util::dll_symbol(handle, "dllEntry");
            if (!pfndllEntry) {
                QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << Util::path_basename(file) << "\", " << APIType_Name(dll_api) << "): Could not locate mod entry point \"dllEntry\"\n";
                return false;
            }

            mod_vmMain pfnvmMain = (mod_vmMain)Util::dll_symbol(handle, "vmMain");
            if (!pfnvmMain) {
                QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << Util::path_basename(file) << "\", " << APIType_Name(dll_api) << "): Could not locate mod entry point \"vmMain\"\n";
                return false;
            }

            // pass vmMain to game-specific mod load handler
            if (!QMM::game->ModLoad((void*)pfnvmMain, dll_api)) {
                // if it failed, call ModUnload to allow game support code to reset
                QMM::game->ModUnload(dll_api);

                QMMLOG(QMM_LOG_ERROR, "QMM") << "Mod::InitDLL(\"" << Util::path_basename(file) << "\", " << APIType_Name(dll_api) << "): " << QMM::game->GameCode() << "_GameSupport::ModLoad returned false\n";

                return false;
            }

            // we need to pass qmm_syscall to mod's dllEntry function
            pfndllEntry(qmm_syscall);
            break;
        }
        default:
            return false;
        };

        // mod load handler was successful
        api = dll_api;
        dll = handle;
        path = file;
        return true;
    }

}   // namespace Mod
