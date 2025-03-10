/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include <string>
#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/format.h"
#include "qmm.h"
#include "CModMgr.h"
#include "game_api.h"
#include "main.h"
#include "CDLLMod.h"
#include "util.h"

CDLLMod::CDLLMod() {
}

CDLLMod::~CDLLMod() {
}

// - file is either the full path or relative to the install directory
//homepath stuff is all handled in CModMgr::LoadMod()
int CDLLMod::LoadMod(std::string file) {
	if (file.empty())
		return 0;

	this->file = file;
	
	//file not found
	if (!this->dll.Load(this->file)) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CDLLMod::LoadMod(\"{}\"): Unable to load mod file: {}\n", file, dlerror()).c_str());
		return 0;
	}

	//locate dllEntry() function or fail
	if ((this->pfndllEntry = (mod_dllEntry_t)this->dll.GetProc("dllEntry")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CDLLMod::LoadMod(\"{}\"): Unable to locate dllEntry() mod entry point\n", file).c_str());
		return 0;
	}

	//locate vmMain() function or fail
	if ((this->pfnvmMain = (mod_vmMain_t)this->dll.GetProc("vmMain")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CDLLMod::LoadMod(\"{}\"): Unable to locate vmMain() mod entry point\n", file).c_str());
		return 0;
	}

	//call mod's dllEntry
	this->pfndllEntry(g_ModMgr->QMM_SysCall());

	return 1;
}

int CDLLMod::vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	return this->pfnvmMain ? this->pfnvmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) : 0;
}

int CDLLMod::IsVM() {
	return 0;
}

std::string& CDLLMod::File() {
	return this->file;
}

int CDLLMod::GetBase() {
	return 0;
}

void CDLLMod::Status() {
	ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] dllEntry() offset: {}\n", (void*)(this->pfndllEntry)).c_str());
	ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] vmMain() offset: {}\n", (void*)(this->pfnvmMain)).c_str());
}
