/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_CMOD_H__
#define __QMM2_CMOD_H__

#include "osdef.h"
#include <string>

class CMod {
	public:
		virtual ~CMod() {};

		virtual int LoadMod(std::string) = 0;

		virtual int vmMain(int, int, int, int, int, int, int, int, int, int, int, int, int) = 0;
		virtual int IsVM() = 0;
		virtual std::string& File() = 0;

		virtual int GetBase() = 0;

		virtual void Status() = 0;

	private:
		char file[PATH_MAX];
};

#endif //__QMM2_CMOD_H__
