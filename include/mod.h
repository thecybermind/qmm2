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

#include "qmmapi.h"

typedef struct mod_s {
	mod_vmMain_t pfnvmMain;
} mod_t;

extern mod_t g_mod;

#endif // __QMM2_MOD_H__

