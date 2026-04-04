/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_MOD_H
#define QMM2_MOD_H

#include <string>
#include "gameapi.hpp"
#include "qvm.h"

struct Mod {
    std::string path;				// mod file path
    qvm vm = {};					// QVM object
    void* dll = nullptr;			// OS DLL handle
    intptr_t vmbase = 0;			// base data segment address for QVMs (0 if DLL mod)
    APIType api = QMM_API_ERROR;	// api the mod DLL was loaded with

    bool Load(std::string file);	// load the given file
    void Unload();					// unload any loaded mod

private:
    // entry point into QVM mods. stored in mod_t->pfnvmMain for QVM mods
    static intptr_t QVM_vmMain(intptr_t cmd, ...);

    // handle syscalls from the QVM. passed to qvm_load
    static int QVM_syscall(uint8_t* membase, int cmd, int* args);

    // load a QVM mod
    bool LoadQVM();

    // attempt to load a DLL mod with the given api type
    bool LoadDLL(APIType dll_api);
};

extern Mod g_mod;					// the mod file that QMM has loaded

#endif // QMM2_MOD_H

