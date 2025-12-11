/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include <string.h>
#include <stvoysp/game/q_shared.h>
#include <stvoysp/game/g_public.h>
#include "game_api.h"
#include "log.h"
// QMM-specific STVOYSP header
#include "game_stvoysp.h"
#include "main.h"
#include "util.h"


GEN_QMM_MSGS(STVOYSP);
GEN_EXTS(STVOYSP);

// a copy of the original import struct that comes from the game engine. this is given to plugins
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod. this is given to plugins
static game_export_t* orig_export = nullptr;


// struct with lambdas that call QMM's syscall function. this is given to the mod
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
		GEN_IMPORT(AppendToSaveGame, G_APPENDTOSAVEGAME),
		GEN_IMPORT(ReadFromSaveGame, G_READFROMSAVEGAME),
		GEN_IMPORT(ReadFromSaveGameOptional, G_READFROMSAVEGAMEOPTIONAL),
		GEN_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND_EX),
		GEN_IMPORT(DropClient, G_DROP_CLIENT),
		GEN_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND),
		GEN_IMPORT(SetConfigstring, G_SET_CONFIGSTRING),
		GEN_IMPORT(GetConfigstring, G_GET_CONFIGSTRING),
		GEN_IMPORT(GetUserinfo, G_GET_USERINFO),
		GEN_IMPORT(SetUserinfo, G_SET_USERINFO),
		GEN_IMPORT(GetServerinfo, G_GET_SERVERINFO),
		GEN_IMPORT(SetBrushModel, G_SET_BRUSH_MODEL),
		GEN_IMPORT(trace, G_TRACE),
		GEN_IMPORT(pointcontents, G_POINT_CONTENTS),
		GEN_IMPORT(inPVS, G_IN_PVS),
		GEN_IMPORT(inPVSIgnorePortals, G_IN_PVS_IGNOREPORTALS),
		GEN_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE),
		GEN_IMPORT(AreasConnected, G_AREAS_CONNECTED),
		GEN_IMPORT(linkentity, G_LINKENTITY),
		GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
		GEN_IMPORT(EntitiesInBox, G_ENTITIES_IN_BOX),
		GEN_IMPORT(EntityContact, G_ENTITY_CONTACT),
		nullptr,	// int* S_Override
		GEN_IMPORT(Malloc, G_MALLOC),
		GEN_IMPORT(Free, G_FREE),
};


// these are "pre" hooks for storing some data for polyfills.
// we need these to be called BEFORE plugins' prehooks get called so they have to be done in the qmm_export table

// track entstrings for our G_GET_ENTITY_TOKEN syscall
static std::vector<std::string> s_entity_tokens;
static void STVOYSP_Init(const char* mapname, const char* spawntarget, int checkSum, const char* entstring, int levelTime, int randomSeed, int globalTime, SavedGameJustLoaded_e eSavedGameJustLoaded, qboolean qbLoadTransition) {
	s_entity_tokens = util_parse_tokens(entstring);
	is_QMM_vmMain_call = true;
	vmMain(GAME_INIT, mapname, spawntarget, checkSum, entstring, levelTime, randomSeed, globalTime, eSavedGameJustLoaded, qbLoadTransition);
}


// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
	GAME_API_VERSION,	// apiversion
	STVOYSP_Init,
	GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
	GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
	GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
	GEN_EXPORT(GameAllowedToSaveHere, GAME_GAMEALLOWEDTOSAVEHERE),
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
intptr_t STVOYSP_syscall(intptr_t cmd, ...) {
	QMM_GET_SYSCALL_ARGS();

	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("STVOYSP_syscall({} {}) called\n", STVOYSP_eng_msg_names(cmd), cmd);

	// store copy of mod's export pointer. this is stored in g_gameinfo.api_info in s_mod_load_getgameapi(),
	// or set to nullptr in mod_unload()
	orig_export = (game_export_t*)(g_gameinfo.api_info.orig_export);

	// before the engine is called into by the mod, some of the variables in the mod's exports may have changed
	// and these changes need to be available to the engine, so copy those values before entering the engine
	if (orig_export) {
		qmm_export.gentities = orig_export->gentities;
		qmm_export.gentitySize = orig_export->gentitySize;
		qmm_export.num_entities = orig_export->num_entities;
	}

	intptr_t ret = 0;

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
		ROUTE_IMPORT(AppendToSaveGame, G_APPENDTOSAVEGAME);
		ROUTE_IMPORT(ReadFromSaveGame, G_READFROMSAVEGAME);
		ROUTE_IMPORT(ReadFromSaveGameOptional, G_READFROMSAVEGAMEOPTIONAL);
		ROUTE_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND_EX);
		ROUTE_IMPORT(DropClient, G_DROP_CLIENT);
		ROUTE_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND);
		ROUTE_IMPORT(SetConfigstring, G_SET_CONFIGSTRING);
		ROUTE_IMPORT(GetConfigstring, G_GET_CONFIGSTRING);
		ROUTE_IMPORT(GetUserinfo, G_GET_USERINFO);
		ROUTE_IMPORT(SetUserinfo, G_SET_USERINFO);
		ROUTE_IMPORT(GetServerinfo, G_GET_SERVERINFO);
		ROUTE_IMPORT(SetBrushModel, G_SET_BRUSH_MODEL);
		ROUTE_IMPORT(trace, G_TRACE);
		ROUTE_IMPORT(pointcontents, G_POINT_CONTENTS);
		ROUTE_IMPORT(inPVS, G_IN_PVS);
		ROUTE_IMPORT(inPVSIgnorePortals, G_IN_PVS_IGNOREPORTALS);
		ROUTE_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE);
		ROUTE_IMPORT(AreasConnected, G_AREAS_CONNECTED);
		ROUTE_IMPORT(linkentity, G_LINKENTITY);
		ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
		ROUTE_IMPORT(EntitiesInBox, G_ENTITIES_IN_BOX);
		ROUTE_IMPORT(EntityContact, G_ENTITY_CONTACT);
		ROUTE_IMPORT(Malloc, G_MALLOC);
		ROUTE_IMPORT(Free, G_FREE);

		// handle cmds for variables, this is how a plugin would get these values if needed
		ROUTE_IMPORT_VAR(S_Override, GVP_S_OVERRIDE);

		// handle special cmds which QMM uses but STVOYSP doesn't have an analogue for
		case G_CVAR_REGISTER: {
			// stvoysp: cvar_t* (*cvar)(const char* varName, const char* varValue, int varFlags)
			// q3a: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
			// qmm always passes NULL for vmCvar so don't worry about it
			const char* varName = (const char*)(args[1]);
			const char* defaultValue = (const char*)(args[2]);
			int flags = args[3];
			(void)orig_import.cvar(varName, defaultValue, flags);
			break;
		}
		case G_SEND_CONSOLE_COMMAND: {
			// STVOYSP: void (*SendConsoleCommand)(const char *text);
			// qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
			const char* text = (const char*)(args[1]);
			orig_import.SendConsoleCommand(text);
			break;
		}
		// help plugins not need separate logic for entity/client pointers
		case G_LOCATE_GAME_DATA: {
			// void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient);
			// this is just to be hooked by plugins, so ignore everything
			break;
		}
		case G_GET_ENTITY_TOKEN: {
			// qboolean trap_GetEntityToken(char *buffer, int bufferSize);
			static size_t token = 0;
			if (token >= s_entity_tokens.size()) {
				ret = qfalse;
				break;
			}

			char* buffer = (char*)args[0];
			intptr_t bufferSize = args[1];

			strncpyz(buffer, s_entity_tokens[token++].c_str(), bufferSize);
			ret = qtrue;
			break;
		}

		default:
			break;
	};

	// do anything that needs to be done after function call here

	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("STVOYSP_syscall({} {}) returning {}\n", STVOYSP_eng_msg_names(cmd), cmd, ret);

	return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t STVOYSP_vmMain(intptr_t cmd, ...) {
	QMM_GET_VMMAIN_ARGS();

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("STVOYSP_vmMain({} {}) called\n", STVOYSP_mod_msg_names(cmd), cmd);

	// store copy of mod's export pointer. this is stored in g_gameinfo.api_info in s_mod_load_getgameapi(),
	// or set to nullptr in mod_unload()
	orig_export = (game_export_t*)(g_gameinfo.api_info.orig_export);
	if (!orig_export)
		return 0;

	// store return value since we do some stuff after the function call is over
	intptr_t ret = 0;

	switch (cmd) {
		ROUTE_EXPORT(Init, GAME_INIT);
		ROUTE_EXPORT(Shutdown, GAME_SHUTDOWN);
		ROUTE_EXPORT(WriteLevel, GAME_WRITE_LEVEL);
		ROUTE_EXPORT(ReadLevel, GAME_READ_LEVEL);
		ROUTE_EXPORT(GameAllowedToSaveHere, GAME_GAMEALLOWEDTOSAVEHERE);
		ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
		ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
		ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
		ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
		ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
		ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
		ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
		ROUTE_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND);

		// handle cmds for variables, this is how a plugin would get these values if needed
		ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
		ROUTE_EXPORT_VAR(gentities, GAMEVP_GENTITIES);
		ROUTE_EXPORT_VAR(gentitySize, GAMEV_GENTITYSIZE);
		ROUTE_EXPORT_VAR(num_entities, GAMEV_NUM_ENTITIES);

		default:
			break;
	};

	// if entity data changed, send a G_LOCATE_GAME_DATA so plugins can hook it
	if (qmm_export.gentities != orig_export->gentities
		|| qmm_export.gentitySize != orig_export->gentitySize
		|| qmm_export.num_entities != orig_export->num_entities
		) {

		gentity_t* gentities = orig_export->gentities;

		if (gentities) {
			playerState_s* clients = gentities[1].client;
			intptr_t clientsize = (std::byte*)(gentities[2].client) - (std::byte*)clients;
			// this will trigger this message to be fired to plugins, and then it will be handled
			// by the empty "case G_LOCATE_GAME_DATA" above in QUAKE2_syscall
			qmm_syscall(G_LOCATE_GAME_DATA, (intptr_t)gentities, orig_export->num_entities, orig_export->gentitySize, (intptr_t)clients, clientsize);
		}
	}

	// after the mod is called into by the engine, some of the variables in the mod's exports may have changed (num_entities and errorMessage in particular)
	// and these changes need to be available to the engine, so copy those values again now before returning from the mod
	qmm_export.gentities = orig_export->gentities;
	qmm_export.gentitySize = orig_export->gentitySize;
	qmm_export.num_entities = orig_export->num_entities;

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("STVOYSP_vmMain({} {}) returning {}\n", STVOYSP_mod_msg_names(cmd), cmd, ret);

	return ret;
}


void* STVOYSP_GetGameAPI(void* import) {
	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("STVOYSP_GetGameAPI({}) called\n", import);

	// original import struct from engine
	// the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
	game_import_t* gi = (game_import_t*)import;
	orig_import = *gi;

	// fill in variables of our hooked import struct to pass to the mod
	qmm_import.S_Override = orig_import.S_Override;

	// this gets passed to the mod's GetGameAPI() function in mod.cpp:mod_load()
	g_gameinfo.api_info.qmm_import = &qmm_import;

	// this isn't used anywhere except returning from this function, but store it in g_gameinfo.api_info for consistency
	g_gameinfo.api_info.qmm_export = &qmm_export;

	// pointer to wrapper vmMain function that calls actual mod func from orig_export
	// this gets assigned to g_mod->pfnvmMain in mod.cpp:mod_load()
	g_gameinfo.api_info.orig_vmmain = STVOYSP_vmMain;

	// pointer to wrapper syscall function that calls actual engine func from orig_import
	g_gameinfo.pfnsyscall = STVOYSP_syscall;

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("STVOYSP_GetGameAPI({}) returning {}\n", import, (void*)&qmm_export);

	// struct full of export lambdas to QMM's vmMain
	// this gets returned to the game engine, but we haven't loaded the mod yet.
	// the only thing in this struct the engine uses before calling Init is the apiversion
	return &qmm_export;
}


const char* STVOYSP_eng_msg_names(intptr_t cmd) {
	switch(cmd) {
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
		GEN_CASE(G_APPENDTOSAVEGAME);
		GEN_CASE(G_READFROMSAVEGAME);
		GEN_CASE(G_READFROMSAVEGAMEOPTIONAL);
		GEN_CASE(G_SEND_CONSOLE_COMMAND_EX);
		GEN_CASE(G_DROP_CLIENT);
		GEN_CASE(G_SEND_SERVER_COMMAND);
		GEN_CASE(G_SET_CONFIGSTRING);
		GEN_CASE(G_GET_CONFIGSTRING);
		GEN_CASE(G_GET_USERINFO);
		GEN_CASE(G_SET_USERINFO);
		GEN_CASE(G_GET_SERVERINFO);
		GEN_CASE(G_SET_BRUSH_MODEL);
		GEN_CASE(G_TRACE);
		GEN_CASE(G_POINT_CONTENTS);
		GEN_CASE(G_IN_PVS);
		GEN_CASE(G_IN_PVS_IGNOREPORTALS);
		GEN_CASE(G_ADJUSTAREAPORTALSTATE);
		GEN_CASE(G_AREAS_CONNECTED);
		GEN_CASE(G_LINKENTITY);
		GEN_CASE(G_UNLINKENTITY);
		GEN_CASE(G_ENTITIES_IN_BOX);
		GEN_CASE(G_ENTITY_CONTACT);
		GEN_CASE(GVP_S_OVERRIDE);
		GEN_CASE(G_MALLOC);
		GEN_CASE(G_FREE);

		// polyfills
		GEN_CASE(G_CVAR_REGISTER);
		GEN_CASE(G_SEND_CONSOLE_COMMAND);

		GEN_CASE(G_LOCATE_GAME_DATA);
		GEN_CASE(G_GET_ENTITY_TOKEN);

		default:
			return "unknown";
	}
}

const char* STVOYSP_mod_msg_names(intptr_t cmd) {
	switch(cmd) {
		GEN_CASE(GAMEV_APIVERSION);
		GEN_CASE(GAME_INIT);
		GEN_CASE(GAME_SHUTDOWN);
		GEN_CASE(GAME_WRITE_LEVEL);
		GEN_CASE(GAME_READ_LEVEL);
		GEN_CASE(GAME_GAMEALLOWEDTOSAVEHERE);
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
