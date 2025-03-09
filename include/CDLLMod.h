/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_CDLLMOD_H__
#define __QMM2_CDLLMOD_H__

#include "osdef.h"
#include "CMod.h"
#include "CDLL.h"
#include "qmmapi.h"

class CDLLMod : public CMod {
	public:
		CDLLMod();
		~CDLLMod();

		int LoadMod(std::string file);

		int vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11);

		int IsVM();

		std::string& File();
		int GetBase();

		void Status();

	private:
		std::string file;
		CDLL dll;
		mod_vmMain_t pfnvmMain;
		mod_dllEntry_t pfndllEntry;
};

#endif //__QMM2_CDLLMOD_H__
