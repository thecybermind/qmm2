/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_MOD_H__
#define __QMM2_MOD_H__

#include <string>
#include "qmmapi.h"
#include "qvm.h"

typedef struct mod_s {
	void* dll = nullptr;
	qvm_t qvm;
	mod_vmMain_t pfnvmMain = nullptr;
	int vmbase = 0;
	std::string path;
} mod_t;

extern mod_t g_mod;

bool mod_load(mod_t*, std::string);
void mod_unload(mod_t*);

bool mod_is_loaded(mod_t*);

#endif // __QMM2_MOD_H__

