/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifdef QMM_GETGAMEAPI_SUPPORT

#define _CRT_SECURE_NO_WARNINGS 1
#include <string.h>
#include <stvoysp/game/q_shared.h>
// fix for type mismatch of GetGameAPI in g_public.h
// the actual type should be: game_export_t *GetGameAPI(game_import_t *import)
// but to avoid having to include game-specific headers in main.cpp, our export is void *GetGameAPI(void *import)
#define GetGameAPI GetGameAPI2 
#define GAME_DLL
#include <stvoysp/game/g_public.h>
#undef GetGameAPI
#undef GAME_DLL
#include "game_api.h"
// QMM-specific STVOYSP header
#include "game_stvoysp.h"
#include "main.h"

GEN_QMM_MSGS(STVOYSP);

// a copy of the original import struct that comes from the game engine. this is given to plugins
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod. this is given to plugins
static game_export_t* orig_export = nullptr;

// struct with lambdas that call QMM's syscall function. this is given to the mod
#define GEN_IMPORT(field, code) (decltype(qmm_import. field)) +[](int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8) { return syscall(code, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }
static game_import_t qmm_import = {

};

// struct with lambdas that call QMM's vmMain function. this is given to the game engine
#define GEN_EXPORT(field, code)	(decltype(qmm_export. field)) +[](int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { return vmMain(code, arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0, 0, 0, 0, 0); }
static game_export_t qmm_export = {
	GAME_API_VERSION,	// apiversion

};

// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
#define ROUTE_IMPORT(field, code)		case code: ret = ((pfn_import_t)(orig_import. field))(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]); break
#define ROUTE_IMPORT_VAR(field, code)	case code: ret = (int)(orig_import. field); break
int STVOYSP_syscall(int cmd, ...) {
	int ret = 0;
	va_list arglist;
	int args[9] = {};	// pull 9 args out of ...
	va_start(arglist, cmd);
	for (unsigned int i = 0; i < (sizeof(args) / sizeof(args[0])); ++i)
		args[i] = va_arg(arglist, int);
	va_end(arglist);

	switch (cmd) {
		case 0:

		// handle codes for variables, this is how a plugin would get these values if needed

		// handle special codes which QMM uses but MOHAA doesn't have an analogue for

		default:
			break;
	};

	// do anything that needs to be done after function call here

	return ret;
}

// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
#define ROUTE_EXPORT(field, code)		case code: ret = ((pfn_export_t)(orig_export-> field))(arg0, arg1, arg2, arg3, arg4, arg5, arg6); break
#define ROUTE_EXPORT_VAR(field, code)	case code: ret = (int)(orig_export-> field); break
int STVOYSP_vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	// store return value since we do some stuff after the function call is over
	int ret = 0;
	switch (cmd) {

		// handle codes for variables, this is how a plugin would get these values if needed
		ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);

		default:
			break;
	};

	// after the mod is called into by the engine, some of the variables in the mod's exports may have changed (num_entities and errorMessage in particular)
	// and these changes need to be available to the engine, so copy those values again now before returning from the mod
	//qmm_export.errorMessage = orig_export->errorMessage;

	return ret;
}

void* STVOYSP_GetGameAPI(void* import) {
	// original import struct from engine
	// the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
	game_import_t* gi = (game_import_t*)import;
	orig_import = *gi;
	g_gameinfo.api_info.orig_import = (void*)&orig_import;

	// fill in variables of our hooked import struct to pass to the mod
	//qmm_import.DebugLines = gi->DebugLines;

	// this gets passed to the mod's GetGameAPI() function in mod.cpp:mod_load()
	g_gameinfo.api_info.qmm_import = &qmm_import;

	// pointer to wrapper vmMain function that calls actual mod func from orig_export
	// this gets assigned to g_mod->pfnvmMain in mod.cpp:mod_load()
	g_gameinfo.api_info.orig_vmmain = STVOYSP_vmMain;

	// pointer to wrapper syscall function that calls actual engine func from orig_import
	g_gameinfo.pfnsyscall = STVOYSP_syscall;

	// struct full of export lambdas to QMM's vmMain
	// this gets returned to the game engine, but we haven't loaded the mod yet.
	// the only thing in this struct the engine uses before calling Init is the apiversion
	return &qmm_export;
}

const char* STVOYSP_eng_msg_names(int cmd) {
	switch (cmd) {
		case 0:

		default:
			return "unknown";
	}
}

const char* STVOYSP_mod_msg_names(int cmd) {
	switch (cmd) {
		case 0:

		default:
			return "unknown";
	}
}

#endif // QMM_GETGAMEAPI_SUPPORT
