/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include "CDLL.h"
#include "osdef.h"

CDLL::CDLL() {
	this->hDLL = NULL;
}

CDLL::~CDLL() {
	this->Unload();
}

bool CDLL::Load(std::string file) {
	if (this->hDLL)
		return 0;
	
	this->hDLL = dlopen(file.c_str(), RTLD_NOW);

	return !!this->hDLL;
}

void* CDLL::GetProc(const char* func) {
	return this->hDLL ? dlsym(this->hDLL, func) : NULL;
}

void CDLL::Unload() {
	if (this->hDLL)
		dlclose(this->hDLL);
};
