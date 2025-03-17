/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifdef _WIN32
// 'typedef ': ignored on left of '<unnamed-enum>' when no variable is declared
#pragma warning(disable:4091)
#endif

#include <stvoymp/game/q_shared.h>
#include <stvoymp/game/g_public.h>

#include "game_api.h"
#include "main.h"

GEN_QMM_MSGS(STVOYMP);

// these function ids are defined either in the g_syscalls.asm file (or in qcommon.h from q3 source),
// but they do not appear in the enum in stvoymp/game/g_public.h
enum {
	G_MEMSET                = 100,

};

const char* STVOYMP_eng_msg_names(int cmd) {
	switch(cmd) {
		case 0:

		default:
			return "unknown";
	}
}

const char* STVOYMP_mod_msg_names(int cmd) {
	switch(cmd) {
		case 0:

		default:
			return "unknown";
	}
}

/* Entry point: qvm mod->qmm
   This is the syscall function called by a QVM mod as a way to pass info to or get info from the engine.
   It modifies pointer arguments (if they are not NULL, the QVM data segment base address is added), and then the call is passed to the normal syscall() function that DLL mods call.
*/
// vec3_t are arrays, so convert them as pointers
// for double pointers (gentity_t** and vec3_t*), convert them once with vmptr()
int STVOYMP_vmsyscall(byte* membase, int cmd, int* args) {
	switch(cmd) {
		case 0:

		default:
			return 0;
	}
}
