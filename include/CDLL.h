/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_CDLL_H__
#define __QMM2_CDLL_H__

#include <string>

class CDLL {
	public:
		CDLL();
		~CDLL();

		bool Load(std::string);
		void* GetProc(const char*);
		void Unload();

	private:
		void* hDLL;
};

#endif //__QMM2_CDLL_H__
