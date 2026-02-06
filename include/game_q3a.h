/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAME_Q3A_H
#define QMM2_GAME_Q3A_H

// these function ids are defined either in the g_syscalls.asm file or in qcommon.h from q3 source,
// but they do not appear in the enum in q3a/game/g_public.h
enum {
	G_MEMSET = 100,
	G_MEMCPY = 101,
	G_STRNCPY = 102,
	G_SIN = 103,
	G_COS = 104,
	G_ATAN2 = 105,
	G_SQRT = 106,
	G_MATRIXMULTIPLY = 107,
	G_ANGLEVECTORS = 108,
	G_PERPENDICULARVECTOR = 109,
	G_FLOOR = 110,
	G_CEIL = 111,
	G_TESTPRINTINT = 112,
	G_TESTPRINTFLOAT = 113
};

// these import messages do not have an exact analogue in Q3A
enum {
	G_ARGS = -100,					// char* (void)
};

#endif // QMM2_GAME_Q3A_H
