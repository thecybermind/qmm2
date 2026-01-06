/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_GAME_STVOYMP_H__
#define __QMM2_GAME_STVOYMP_H__


// these function ids are defined either in the g_syscalls.asm file or in qcommon.h from ioef source,
// but they do not appear in the enum in stvoyhm/game/g_public.h
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

// these import messages do not have an exact analogue in STVOYMP
enum {
	G_ARGS = -100,					// char* (void)
};

#endif // __QMM2_GAME_STVOYMP_H__
