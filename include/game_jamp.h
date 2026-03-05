/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAME_JAMP_H
#define QMM2_GAME_JAMP_H

// these import messages do not exist in the "legacy" JAMP API but they do in GetModuleAPI
enum {
	G_SV_REGISTER_SHARED_MEMORY = 90,	// void (char *memory)
};

// these import messages do not have an exact analogue in JAMP
enum {
	G_ARGS = -100,					// char* (void)
};

#endif // QMM2_GAME_JAMP_H
