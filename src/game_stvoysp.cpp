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
#define GEN_IMPORT(field, code) (decltype(qmm_import. field)) +[](int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { return syscall(code, arg0, arg1, arg2, arg3, arg4, arg5, arg6); }
static game_import_t qmm_import = {
		GEN_IMPORT(Printf, G_PRINTF),
		GEN_IMPORT(WriteCam, G_WRITECAM),
		GEN_IMPORT(Error, G_ERROR),
		GEN_IMPORT(Milliseconds, G_MILLISECONDS),
		GEN_IMPORT(cvar, G_CVAR),
		GEN_IMPORT(cvar_set, G_CVAR_SET),
		GEN_IMPORT(Cvar_VariableIntegerValue, G_CVAR_VARIABLE_INTEGER_VALUE),
		GEN_IMPORT(Cvar_VariableStringBuffer, G_CVAR_VARIABLE_STRING_BUFFER),
		GEN_IMPORT(argc, G_ARGC),
		GEN_IMPORT(argv, G_ARGV),
		GEN_IMPORT(FS_FOpenFile, G_FS_FOPEN_FILE),
		GEN_IMPORT(FS_Read, G_FS_READ),
		GEN_IMPORT(FS_Write, G_FS_WRITE),
		GEN_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE),
		GEN_IMPORT(FS_ReadFile, G_FS_READFILE),
		GEN_IMPORT(FS_FreeFile, G_FS_FREEFILE),
		GEN_IMPORT(FS_GetFileList, G_FS_GETFILELIST),
		GEN_IMPORT(AppendToSaveGame, G_APPEND_TO_SAVEGAME),
		GEN_IMPORT(ReadFromSaveGame, G_READ_FROM_SAVEGAME),
		GEN_IMPORT(ReadFromSaveGameOptional, G_READ_FROM_SAVEGAME_OPTIONAL),
		GEN_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND),
		GEN_IMPORT(DropClient, G_DROP_CLIENT),
		GEN_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND),
		GEN_IMPORT(SetConfigstring, G_SETCONFIGSTRING),
		GEN_IMPORT(GetConfigstring, G_GETCONFIGSTRING),
		GEN_IMPORT(GetUserinfo, G_GETUSERINFO),
		GEN_IMPORT(SetUserinfo, G_SETUSERINFO),
		GEN_IMPORT(GetServerinfo, G_GETSERVERINFO),
		GEN_IMPORT(SetBrushModel, G_SETBRUSHMODEL),
		GEN_IMPORT(trace, G_TRACE),
		GEN_IMPORT(pointcontents, G_POINTCONTENTS),
		GEN_IMPORT(inPVS, G_INPVS),
		GEN_IMPORT(inPVSIgnorePortals, G_INPVSIGNOREPORTALS),
		GEN_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE),
		GEN_IMPORT(AreasConnected, G_AREASCONNECTED),
		GEN_IMPORT(linkentity, G_LINKENTITY),
		GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
		GEN_IMPORT(EntitiesInBox, G_ENTITIESINBOX),
		GEN_IMPORT(EntityContact, G_ENTITYCONTACT),
		nullptr,	// S_Override
		GEN_IMPORT(Malloc, G_MALLOC),
		GEN_IMPORT(Free, G_FREE),
};

// struct with lambdas that call QMM's vmMain function. this is given to the game engine
#define GEN_EXPORT(field, code)	(decltype(qmm_export. field)) +[](int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { return vmMain(code, arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0, 0, 0, 0, 0); }
static game_export_t qmm_export = {
	GAME_API_VERSION,	// apiversion
	GEN_EXPORT(Init, GAME_INIT),
	GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
	GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
	GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
	GEN_EXPORT(GameAllowedToSaveHere, GAME_GAME_ALLOWED_TO_SAVE_HERE),
	GEN_EXPORT(ClientConnect, GAME_CLIENT_CONNECT),
	GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
	GEN_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED),
	GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
	GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
	GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
	GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
	GEN_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND),

	// the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
	nullptr,	// gentities
	0,			// gentitySize
	0,			// num_entities
};

// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
typedef int(*pfn_import_t)(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);
#define ROUTE_IMPORT(field, code)		case code: ret = ((pfn_import_t)(orig_import. field))(args[0], args[1], args[2], args[3], args[4], args[5], args[6]); break
#define ROUTE_IMPORT_VAR(field, code)	case code: ret = (int)(orig_import. field); break
int STVOYSP_syscall(int cmd, ...) {
	int ret = 0;
	va_list arglist;
	int args[7] = {};	// pull 7 args out of varargs for trace
	va_start(arglist, cmd);
	for (unsigned int i = 0; i < (sizeof(args) / sizeof(args[0])); ++i)
		args[i] = va_arg(arglist, int);
	va_end(arglist);

	switch (cmd) {
		ROUTE_IMPORT(Printf, G_PRINTF);
		ROUTE_IMPORT(WriteCam, G_WRITECAM);
		ROUTE_IMPORT(Error, G_ERROR);
		ROUTE_IMPORT(Milliseconds, G_MILLISECONDS);
		ROUTE_IMPORT(cvar, G_CVAR);
		ROUTE_IMPORT(cvar_set, G_CVAR_SET);
		ROUTE_IMPORT(Cvar_VariableIntegerValue, G_CVAR_VARIABLE_INTEGER_VALUE);
		ROUTE_IMPORT(Cvar_VariableStringBuffer, G_CVAR_VARIABLE_STRING_BUFFER);
		ROUTE_IMPORT(argc, G_ARGC);
		ROUTE_IMPORT(argv, G_ARGV);
		ROUTE_IMPORT(FS_FOpenFile, G_FS_FOPEN_FILE);
		ROUTE_IMPORT(FS_Read, G_FS_READ);
		ROUTE_IMPORT(FS_Write, G_FS_WRITE);
		ROUTE_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE);
		ROUTE_IMPORT(FS_ReadFile, G_FS_READFILE);
		ROUTE_IMPORT(FS_FreeFile, G_FS_FREEFILE);
		ROUTE_IMPORT(FS_GetFileList, G_FS_GETFILELIST);
		ROUTE_IMPORT(AppendToSaveGame, G_APPEND_TO_SAVEGAME);
		ROUTE_IMPORT(ReadFromSaveGame, G_READ_FROM_SAVEGAME);
		ROUTE_IMPORT(ReadFromSaveGameOptional, G_READ_FROM_SAVEGAME_OPTIONAL);
		ROUTE_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND);
		ROUTE_IMPORT(DropClient, G_DROP_CLIENT);
		ROUTE_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND);
		ROUTE_IMPORT(SetConfigstring, G_SETCONFIGSTRING);
		ROUTE_IMPORT(GetConfigstring, G_GETCONFIGSTRING);
		ROUTE_IMPORT(GetUserinfo, G_GETUSERINFO);
		ROUTE_IMPORT(SetUserinfo, G_SETUSERINFO);
		ROUTE_IMPORT(GetServerinfo, G_GETSERVERINFO);
		ROUTE_IMPORT(SetBrushModel, G_SETBRUSHMODEL);
		ROUTE_IMPORT(trace, G_TRACE);
		ROUTE_IMPORT(pointcontents, G_POINTCONTENTS);
		ROUTE_IMPORT(inPVS, G_INPVS);
		ROUTE_IMPORT(inPVSIgnorePortals, G_INPVSIGNOREPORTALS);
		ROUTE_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE);
		ROUTE_IMPORT(AreasConnected, G_AREASCONNECTED);
		ROUTE_IMPORT(linkentity, G_LINKENTITY);
		ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
		ROUTE_IMPORT(EntitiesInBox, G_ENTITIESINBOX);
		ROUTE_IMPORT(EntityContact, G_ENTITYCONTACT);

		// handle codes for variables, this is how a plugin would get these values if needed
		ROUTE_IMPORT_VAR(S_Override, GVP_S_OVERRIDE);

		// handle special codes which QMM uses but STVOYSP doesn't have an analogue for
		case G_CVAR_REGISTER: {
			// stvoysp: cvar_t* (*cvar)( const char *var_name, const char *value, int flags );
			// q3a: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
			// qmm always passes NULL for vmCvar so don't worry about it
			const char* varName = (const char*)(args[1]);
			const char* defaultValue = (const char*)(args[2]);
			int flags = args[3];
			(void)orig_import.cvar(varName, defaultValue, flags);
			break;
		}

		default:
			break;
	};

	// do anything that needs to be done after function call here

	return ret;
}

// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
typedef int(*pfn_export_t)(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);
#define ROUTE_EXPORT(field, code)		case code: ret = ((pfn_export_t)(orig_export-> field))(arg0, arg1, arg2, arg3, arg4, arg5, arg6); break
#define ROUTE_EXPORT_VAR(field, code)	case code: ret = (int)(orig_export-> field); break
int STVOYSP_vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	// store return value since we do some stuff after the function call is over
	int ret = 0;
	switch (cmd) {
		ROUTE_EXPORT(Init, GAME_INIT);
		ROUTE_EXPORT(Shutdown, GAME_SHUTDOWN);
		ROUTE_EXPORT(WriteLevel, GAME_WRITE_LEVEL);
		ROUTE_EXPORT(ReadLevel, GAME_READ_LEVEL);
		ROUTE_EXPORT(GameAllowedToSaveHere, GAME_GAME_ALLOWED_TO_SAVE_HERE);
		ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
		ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
		ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
		ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
		ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
		ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
		ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
		ROUTE_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND);

		// handle codes for variables, this is how a plugin would get these values if needed
		ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
		ROUTE_EXPORT_VAR(gentities, GAMEVP_GENTITIES);
		ROUTE_EXPORT_VAR(gentitySize, GAMEV_GENTITYSIZE);
		ROUTE_EXPORT_VAR(num_entities, GAMEV_NUM_ENTITIES);

		default:
			break;
	};

	// after the mod is called into by the engine, some of the variables in the mod's exports may have changed (num_entities and errorMessage in particular)
	// and these changes need to be available to the engine, so copy those values again now before returning from the mod
	qmm_export.gentities = orig_export->gentities;
	qmm_export.gentitySize = orig_export->gentitySize;
	qmm_export.num_entities = orig_export->num_entities;

	return ret;
}

void* STVOYSP_GetGameAPI(void* import) {
	// original import struct from engine
	// the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
	game_import_t* gi = (game_import_t*)import;
	orig_import = *gi;
	g_gameinfo.api_info.orig_import = (void*)&orig_import;

	// fill in variables of our hooked import struct to pass to the mod
	qmm_import.S_Override = gi->S_Override;

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
		GEN_CASE(G_PRINTF);
		GEN_CASE(G_WRITECAM);
		GEN_CASE(G_ERROR);
		GEN_CASE(G_MILLISECONDS);
		GEN_CASE(G_CVAR);
		GEN_CASE(G_CVAR_SET);
		GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
		GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
		GEN_CASE(G_ARGC);
		GEN_CASE(G_ARGV);
		GEN_CASE(G_FS_FOPEN_FILE);
		GEN_CASE(G_FS_READ);
		GEN_CASE(G_FS_WRITE);
		GEN_CASE(G_FS_FCLOSE_FILE);
		GEN_CASE(G_FS_READFILE);
		GEN_CASE(G_FS_FREEFILE);
		GEN_CASE(G_FS_GETFILELIST);
		GEN_CASE(G_APPEND_TO_SAVEGAME);
		GEN_CASE(G_READ_FROM_SAVEGAME);
		GEN_CASE(G_READ_FROM_SAVEGAME_OPTIONAL);
		GEN_CASE(G_SEND_CONSOLE_COMMAND);
		GEN_CASE(G_DROP_CLIENT);
		GEN_CASE(G_SEND_SERVER_COMMAND);
		GEN_CASE(G_SETCONFIGSTRING);
		GEN_CASE(G_GETCONFIGSTRING);
		GEN_CASE(G_GETUSERINFO);
		GEN_CASE(G_SETUSERINFO);
		GEN_CASE(G_GETSERVERINFO);
		GEN_CASE(G_SETBRUSHMODEL);
		GEN_CASE(G_TRACE);
		GEN_CASE(G_POINTCONTENTS);
		GEN_CASE(G_INPVS);
		GEN_CASE(G_INPVSIGNOREPORTALS);
		GEN_CASE(G_ADJUSTAREAPORTALSTATE);
		GEN_CASE(G_AREASCONNECTED);
		GEN_CASE(G_LINKENTITY);
		GEN_CASE(G_UNLINKENTITY);
		GEN_CASE(G_ENTITIESINBOX);
		GEN_CASE(G_ENTITYCONTACT);
		GEN_CASE(GVP_S_OVERRIDE);
		GEN_CASE(G_MALLOC);
		GEN_CASE(G_FREE);

		// special codes
		GEN_CASE(G_CVAR_REGISTER);

		default:
			return "unknown";
	}
}

const char* STVOYSP_mod_msg_names(int cmd) {
	switch (cmd) {
		GEN_CASE(GAMEV_APIVERSION);
		GEN_CASE(GAME_INIT);
		GEN_CASE(GAME_SHUTDOWN);
		GEN_CASE(GAME_WRITE_LEVEL);
		GEN_CASE(GAME_READ_LEVEL);
		GEN_CASE(GAME_GAME_ALLOWED_TO_SAVE_HERE);
		GEN_CASE(GAME_CLIENT_CONNECT);
		GEN_CASE(GAME_CLIENT_BEGIN);
		GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
		GEN_CASE(GAME_CLIENT_DISCONNECT);
		GEN_CASE(GAME_CLIENT_COMMAND);
		GEN_CASE(GAME_CLIENT_THINK);
		GEN_CASE(GAME_RUN_FRAME);
		GEN_CASE(GAME_CONSOLE_COMMAND);
		GEN_CASE(GAMEVP_GENTITIES);
		GEN_CASE(GAMEV_GENTITYSIZE);
		GEN_CASE(GAMEV_NUM_ENTITIES);
		default:
			return "unknown";
	}
}

#endif // QMM_GETGAMEAPI_SUPPORT
