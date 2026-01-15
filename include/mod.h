/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_MOD_H__
#define __QMM2_MOD_H__

#include <string>
#include "qmmapi.h"
#include "qvm.h"

struct mod_t {
    void* dll = nullptr;
    qvm_t qvm = {};
    mod_vmMain_t pfnvmMain = nullptr;
    intptr_t vmbase = 0;
    std::string path;
};

extern mod_t g_mod;

bool mod_load(mod_t& mod, std::string file);
void mod_unload(mod_t& mod);

#endif // __QMM2_MOD_H__

