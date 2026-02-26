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

struct mod {
    void* dll = nullptr;
    qvm_t qvm = {};
    intptr_t vmbase = 0;
    std::string path;
};

extern mod g_mod;

bool mod_load(mod& mod, std::string file);
void mod_unload(mod& mod);

#endif // QMM2_MOD_H

