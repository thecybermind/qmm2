/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "CModMgr.h"
#include "qmm.h"
#include "qmmapi.h"
#include "util.h"

static int s_plugin_helper_WriteGameLog(const char* text, int len = -1) {
	return log_write(text, len);
}

static char* s_plugin_helper_VarArgs(char* format, ...) {
	va_list	argptr;
	static char str[8][1024];
	static int index = 0;
	int i = index;

	va_start(argptr, format);
	vsnprintf(str[i], sizeof(str[i]), format, argptr);
	va_end(argptr);

	index = (index + 1) & 7;
	return str[i];
}

static int s_plugin_helper_IsVM() {
	return g_ModMgr->Mod()->IsVM();
}

static const char* s_plugin_helper_EngMsgName(int x) {
	return ENG_MSGNAME(x);
}

static const char* s_plugin_helper_ModMsgName(int x) {
	return MOD_MSGNAME(x);
}

static int s_plugin_helper_GetIntCvar(const char* cvar) {
	return get_int_cvar(cvar);
}

static const char* s_plugin_helper_GetStrCvar(const char* cvar) {
	return get_str_cvar(cvar);
}

pluginfuncs_t* get_pluginfuncs() {
	static pluginfuncs_t pluginfuncs = {
		&s_plugin_helper_WriteGameLog,
		&s_plugin_helper_VarArgs,
		&s_plugin_helper_IsVM,
		&s_plugin_helper_EngMsgName,
		&s_plugin_helper_ModMsgName,
		&s_plugin_helper_GetIntCvar,
		&s_plugin_helper_GetStrCvar,
	};
	return &pluginfuncs;
}
