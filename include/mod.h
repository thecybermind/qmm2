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
    std::string path;
    qvm vm = {};
    void* dll = nullptr;
    intptr_t vmbase = 0;
    bool is_GetGameAPI = false;
};

extern qmm_mod g_mod;

bool mod_load(qmm_mod& mod, std::string file);
void mod_unload(qmm_mod& mod);

#endif // QMM2_MOD_H

