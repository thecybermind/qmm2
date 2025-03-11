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
	qvm_t* qvm = nullptr;
	mod_vmMain_t pfnvmMain = nullptr;
	std::string path;
} mod_t;

extern mod_t g_mod;

typedef enum {
	HEADER_UNKNOWN = 0,
	HEADER_DLL = 1,
	HEADER_SO = 1,
	HEADER_QVM,

	HEADER_MAX
} headertype_t;

headertype_t check_header(std::string);

#endif // __QMM2_MOD_H__

