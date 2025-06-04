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

namespace mod {
	typedef void (*mod_dllEntry_t)(eng_syscall_t syscall);
	typedef void* (*mod_GetGameAPI_t)(void* import);

	typedef struct mod_s {
		void* dll = nullptr;
		qvm::qvm_t qvm = {};
		mod_vmMain_t pfnvmMain = nullptr;
		intptr_t vmbase = 0;
		std::string path;
	} mod_t;

	extern mod_t g_mod;

	bool mod_load(mod_t& mod, std::string file);
	void mod_unload(mod_t& mod);
}

#endif // __QMM2_MOD_H__

