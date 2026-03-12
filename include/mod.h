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
#include "qmmapi.h"
#include "qvm.h"

struct qmm_mod {
    std::string path;				// mod file path
    qvm vm = {};					// QVM object
    void* dll = nullptr;			// OS DLL handle
    intptr_t vmbase = 0;			// base data segment address for QVMs (0 if DLL mod)
    APIType api = QMM_API_ERROR;	// engine api the mod DLL was loaded with
};

extern qmm_mod g_mod;               // the mod file that QMM has loaded

// load the given file into the mod object
bool mod_load(qmm_mod& mod, std::string file);
// unload the given mod object
void mod_unload(qmm_mod& mod);

#endif // QMM2_MOD_H

