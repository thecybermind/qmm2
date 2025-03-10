/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_CMODMGR_H__
#define __QMM2_CMODMGR_H__

#include "CMod.h"
#include "game_api.h"
#include "qmmapi.h"

class CModMgr {
	public:
		CModMgr(eng_syscall_t qmm_syscall);
		~CModMgr();

		int LoadMod();
		void UnloadMod();

		eng_syscall_t QMM_SysCall();

		CMod* Mod();

		static CModMgr* GetInstance(eng_syscall_t qmm_syscall);

	private:
		CMod* mod;
		eng_syscall_t qmm_syscall;
		CMod* newmod(const char*);

		static CModMgr* instance;
};

#endif //__QMM2_CMODMGR_H__
