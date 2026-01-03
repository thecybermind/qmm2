/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1
#include <string.h>
#include <stdio.h>
#include <quake2/game/q_shared.h>
#include <quake2/game/game.h>
#include "game_api.h"
#include "log.h"
// QMM-specific QUAKE2 header
#include "game_quake2.h"
#include "main.h"
#include "util.h"

GEN_QMM_MSGS(QUAKE2);
GEN_EXTS(QUAKE2);

// a copy of the original import struct that comes from the game engine. this is given to plugins
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod. this is given to plugins
static game_export_t* orig_export = nullptr;


// these are "pre" hooks for storing some data for polyfills.
// we need these to be called BEFORE plugins' prehooks get called so they have to be done in the qmm_import table

// track configstrings for our G_GET_CONFIGSTRING syscall
static std::map<int, std::string> s_configstrings;
static void QUAKE2_configstring(int num, char* configstring) {
	// if configstring is null, remove entry in map. otherwise store in map
	if (configstring)
		s_configstrings[num] = configstring;
	else if (s_configstrings.count(num))
		s_configstrings.erase(num);
	qmm_syscall(G_CONFIGSTRING, num, configstring);
}


// struct with lambdas that call QMM's syscall function. this is given to the mod
static game_import_t qmm_import = {
	GEN_IMPORT(bprintf, G_BPRINTF),
	GEN_IMPORT(dprintf, G_DPRINTF),
	GEN_IMPORT(cprintf, G_CPRINTF),
	GEN_IMPORT(centerprintf, G_CENTERPRINTF),
	GEN_IMPORT_6(sound, G_SOUND, void, edict_t*, int, int, float, float, float),
	GEN_IMPORT_7(positioned_sound, G_POSITIONED_SOUND, void, vec3_t, edict_t*, int, int, float, float, float),
	QUAKE2_configstring,
	GEN_IMPORT(error, G_ERROR),
	GEN_IMPORT(modelindex, G_MODELINDEX),
	GEN_IMPORT(soundindex, G_SOUNDINDEX),
	GEN_IMPORT(imageindex, G_IMAGEINDEX),
	GEN_IMPORT(setmodel, G_SETMODEL),
	GEN_IMPORT(trace, G_TRACE),
	GEN_IMPORT(pointcontents, G_POINT_CONTENTS),
	GEN_IMPORT(inPVS, G_IN_PVS),
	GEN_IMPORT(inPHS, G_IN_PHS),
	GEN_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE),
	GEN_IMPORT(AreasConnected, G_AREAS_CONNECTED),
	GEN_IMPORT(linkentity, G_LINKENTITY),
	GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
	GEN_IMPORT(BoxEdicts, G_BOXEDICTS),
	GEN_IMPORT(Pmove, G_PMOVE),
	GEN_IMPORT(multicast, G_MULTICAST),
	GEN_IMPORT(unicast, G_UNICAST),
	GEN_IMPORT(WriteChar, G_MSG_WRITECHAR),
	GEN_IMPORT(WriteByte, G_MSG_WRITEBYTE),
	GEN_IMPORT(WriteShort, G_MSG_WRITESHORT),
	GEN_IMPORT(WriteLong, G_MSG_WRITELONG),
	GEN_IMPORT_1(WriteFloat, G_MSG_WRITEFLOAT, void, float),
	GEN_IMPORT(WriteString, G_MSG_WRITESTRING),
	GEN_IMPORT(WritePosition, G_MSG_WRITEPOSITION),
	GEN_IMPORT(WriteDir, G_MSG_WRITEDIR),
	GEN_IMPORT_1(WriteAngle, G_MSG_WRITEANGLE, void, float),
	GEN_IMPORT(TagMalloc, G_TAGMALLOC),
	GEN_IMPORT(TagFree, G_TAGFREE),
	GEN_IMPORT(FreeTags, G_FREETAGS),
	GEN_IMPORT(cvar, G_CVAR),
	GEN_IMPORT(cvar_set, G_CVAR_SET),
	GEN_IMPORT(cvar_forceset, G_CVAR_FORCESET),
	GEN_IMPORT(argc, G_ARGC),
	GEN_IMPORT(argv, G_ARGV),
	GEN_IMPORT(args, G_ARGS),
	GEN_IMPORT(AddCommandString, G_ADDCOMMANDSTRING),
	GEN_IMPORT_2(DebugGraph, G_DEBUGGRAPH, void, float, int),
};


// these are "pre" hooks for storing some data for polyfills.
// we need these to be called BEFORE plugins' prehooks get called so they have to be done in the qmm_export table

// track userinfo for our G_GET_USERINFO syscall
static std::map<intptr_t, std::string> s_userinfo;
static qboolean QUAKE2_ClientConnect(edict_t* ent, char* userinfo) {
	// get client number (ent->s.number is not set until CLIENT_BEGIN, so calculate based on edict_t*)
	intptr_t entnum = ((intptr_t)ent - (intptr_t)orig_export->edicts) / orig_export->edict_size;
	intptr_t clientnum = entnum - 1;
	// if userinfo is null, remove entry in map. otherwise store in map
	if (userinfo)
		s_userinfo.emplace(clientnum, userinfo);
	else if (s_userinfo.count(clientnum))
		s_userinfo.erase(clientnum);
	is_QMM_vmMain_call = true;
	return vmMain(GAME_CLIENT_CONNECT, ent, userinfo);
}


static void QUAKE2_ClientUserinfoChanged(edict_t* ent, char* userinfo) {
	// get client number (ent->s.number is not set until CLIENT_BEGIN, so calculate based on edict_t*)
	intptr_t entnum = ((intptr_t)ent - (intptr_t)orig_export->edicts) / orig_export->edict_size;
	intptr_t clientnum = entnum - 1;
	// if userinfo is null, remove entry in map. otherwise store in map
	if (userinfo)
		s_userinfo.emplace(clientnum, userinfo);
	else if (s_userinfo.count(clientnum))
		s_userinfo.erase(clientnum);
	is_QMM_vmMain_call = true;
	vmMain(GAME_CLIENT_USERINFO_CHANGED, ent, userinfo);
}


// track entstrings for our G_GET_ENTITY_TOKEN syscall
static std::vector<std::string> s_entity_tokens;
static size_t s_tokencount = 0;
static void QUAKE2_SpawnEntities(char* mapname, char* entstring, char* spawnpoint) {
	if (entstring) {
		s_entity_tokens = util_parse_entstring(entstring);
		s_tokencount = 0;
	}
	is_QMM_vmMain_call = true;
	vmMain(GAME_SPAWN_ENTITIES, mapname, entstring, spawnpoint);
}


// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
	GAME_API_VERSION,	// apiversion
	GEN_EXPORT(Init, GAME_INIT),
	GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
	QUAKE2_SpawnEntities,
	GEN_EXPORT(WriteGame, GAME_WRITE_GAME),
	GEN_EXPORT(ReadGame, GAME_READ_GAME),
	GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
	GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
	QUAKE2_ClientConnect,
	GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
	QUAKE2_ClientUserinfoChanged,
	GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
	GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
	GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
	GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
	GEN_EXPORT(ServerCommand, GAME_SERVER_COMMAND),
	// the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
	nullptr,	// edicts
	0,			// edict_size
	0,			// num_edicts
	0,			// max_edicts
};


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t QUAKE2_syscall(intptr_t cmd, ...) {
	QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("QUAKE2_syscall({} {}) called\n", QUAKE2_eng_msg_names(cmd), cmd);
#endif

	// store copy of mod's export pointer. this is stored in g_gameinfo.api_info in s_mod_load_getgameapi(),
	// or set to nullptr in mod_unload()
	orig_export = (game_export_t*)(g_gameinfo.api_info.orig_export);

	intptr_t ret = 0;

	switch (cmd) {
		ROUTE_IMPORT(bprintf, G_BPRINTF);
		ROUTE_IMPORT(dprintf, G_DPRINTF);
		ROUTE_IMPORT(cprintf, G_CPRINTF);
		ROUTE_IMPORT(centerprintf, G_CENTERPRINTF);
		ROUTE_IMPORT(sound, G_SOUND);
		ROUTE_IMPORT(positioned_sound, G_POSITIONED_SOUND);
		ROUTE_IMPORT(configstring, G_CONFIGSTRING);
		ROUTE_IMPORT(error, G_ERROR);
		ROUTE_IMPORT(modelindex, G_MODELINDEX);
		ROUTE_IMPORT(soundindex, G_SOUNDINDEX);
		ROUTE_IMPORT(imageindex, G_IMAGEINDEX);
		ROUTE_IMPORT(setmodel, G_SETMODEL);
		ROUTE_IMPORT(trace, G_TRACE);
		ROUTE_IMPORT(pointcontents, G_POINT_CONTENTS);
		ROUTE_IMPORT(inPVS, G_IN_PVS);
		ROUTE_IMPORT(inPHS, G_IN_PHS);
		ROUTE_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE);
		ROUTE_IMPORT(AreasConnected, G_AREAS_CONNECTED);
		ROUTE_IMPORT(linkentity, G_LINKENTITY);
		ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
		ROUTE_IMPORT(BoxEdicts, G_BOXEDICTS);
		ROUTE_IMPORT(Pmove, G_PMOVE);
		ROUTE_IMPORT(multicast, G_MULTICAST);
		ROUTE_IMPORT(unicast, G_UNICAST);
		ROUTE_IMPORT(WriteChar, G_MSG_WRITECHAR);
		ROUTE_IMPORT(WriteByte, G_MSG_WRITEBYTE);
		ROUTE_IMPORT(WriteShort, G_MSG_WRITESHORT);
		ROUTE_IMPORT(WriteLong, G_MSG_WRITELONG);
		ROUTE_IMPORT(WriteFloat, G_MSG_WRITEFLOAT);
		ROUTE_IMPORT(WriteString, G_MSG_WRITESTRING);
		ROUTE_IMPORT(WritePosition, G_MSG_WRITEPOSITION);
		ROUTE_IMPORT(WriteDir, G_MSG_WRITEDIR);
		ROUTE_IMPORT(WriteAngle, G_MSG_WRITEANGLE);
		ROUTE_IMPORT(TagMalloc, G_TAGMALLOC);
		ROUTE_IMPORT(TagFree, G_TAGFREE);
		ROUTE_IMPORT(FreeTags, G_FREETAGS);
		ROUTE_IMPORT(cvar, G_CVAR);
		ROUTE_IMPORT(cvar_set, G_CVAR_SET);
		ROUTE_IMPORT(cvar_forceset, G_CVAR_FORCESET);
		ROUTE_IMPORT(argc, G_ARGC);
		ROUTE_IMPORT(argv, G_ARGV);
		ROUTE_IMPORT(args, G_ARGS);
		ROUTE_IMPORT(AddCommandString, G_ADDCOMMANDSTRING);
		ROUTE_IMPORT(DebugGraph, G_DEBUGGRAPH);

		// handle cmds for variables, this is how a plugin would get these values if needed

		// handle special cmds which QMM uses but QUAKE2 doesn't have an analogue for
		case G_CVAR_REGISTER: {
			// quake2: cvar_t *(*cvar) (char *var_name, char *value, int flags);
			// q3a: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
			// qmm always passes NULL for vmCvar so don't worry about it
			char* var_name = (char*)(args[1]);
			char* value = (char*)(args[2]);
			int flags = (int)args[3];
			(void)orig_import.cvar(var_name, value, flags);
			break;
		}
		case G_CVAR_VARIABLE_STRING_BUFFER: {
			// quake2: cvar_t *(*cvar) (char *var_name, char *value, int flags);
			// q3a: void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
			char* var_name = (char*)(args[0]);
			char* buffer = (char*)(args[1]);
			int bufsize = (int)args[2];
			*buffer = '\0';
			cvar_t* cvar = orig_import.cvar(var_name, (char*)"", 0);
			if (cvar)
				strncpyz(buffer, cvar->string, bufsize);
			break;
		}
		case G_CVAR_VARIABLE_INTEGER_VALUE: {
			// quake2: cvar_t *(*cvar) (char *var_name, char *value, int flags);
			// q3a: int trap_Cvar_VariableIntegerValue(const char* var_name)
			char* var_name = (char*)(args[0]);
			cvar_t* cvar = orig_import.cvar(var_name, (char*)"", 0);
			if (cvar)
				ret = (int)cvar->value;
			break;
		}
		case G_SEND_CONSOLE_COMMAND: {
			// quake2: void (*AddCommandString)(char *text);
			// qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
			char* text = (char*)(args[1]);
			orig_import.AddCommandString(text);
			break;
		}
		case G_PRINT: {
			// quake2: void	(*bprintf) (int printlevel, char *fmt, ...);
			// qmm: void trap_Printf( const char *fmt );
			char* text = (char*)args[0];
			orig_import.bprintf(PRINT_HIGH, text);
			break;
		}
		// provide these to plugins just so the most basic file functions all work. use FILE* for these
		case G_FS_FOPEN_FILE: {
			// int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
			const char* qpath = (const char*)args[0];
			fileHandle_t* f = (fileHandle_t*)args[1];
			intptr_t mode = args[2];

			const char* str_mode = "rb";
			if (mode == FS_WRITE)
				str_mode = "wb";
			else if (mode == FS_APPEND)
				str_mode = "ab";
			std::string path = fmt::format("{}/{}", g_gameinfo.qmm_dir, qpath);
			if (mode != FS_READ)
				path_mkdir(path_dirname(path));
			FILE* fp = fopen(path.c_str(), str_mode);
			if (!fp) {
				ret = -1;
				break;
			}
			if (mode == FS_WRITE)
				ret = 0;
			else if (mode == FS_APPEND)
				ret = ftell(fp);
			else {
				if (fseek(fp, 0, SEEK_END) != 0) {
					ret = -1;
					break;
				}
				ret = ftell(fp);
				fseek(fp, 0, SEEK_SET);
			}
			*f = (fileHandle_t)fp;
			break;
		}
		case G_FS_READ: {
			// void trap_FS_Read(void* buffer, int len, fileHandle_t f);
			char* buffer = (char*)args[0];
			size_t len = args[1];
			fileHandle_t f = (fileHandle_t)args[2];
			size_t total = 0;
			FILE* fp = (FILE*)f;
			for (int i = 0; i < 50; i++) {	// prevent infinite loops trying to read
				total += fread(buffer + total, 1, len - total, fp);
				if (total >= len || ferror(fp) || feof(fp))
					break;
			}
			break;
		}
		case G_FS_WRITE: {
			// void trap_FS_Write(const void* buffer, int len, fileHandle_t f);
			char* buffer = (char*)args[0];
			size_t len = args[1];
			fileHandle_t f = (fileHandle_t)args[2];
			size_t total = 0;
			FILE* fp = (FILE*)f;
			for (int i = 0; i < 50; i++) {	// prevent infinite loops trying to write
				total += fwrite(buffer + total, 1, len - total, fp);
				if (total >= len || ferror(fp))
					break;
			}
			break;
		}
		case G_FS_FCLOSE_FILE: {
			// void trap_FS_FCloseFile(fileHandle_t f);
			fileHandle_t f = (fileHandle_t)args[0];
			FILE* fp = (FILE*)f;
			fclose(fp);
			break;
		}
		// help plugins not need separate logic for entity/client pointers
		case G_LOCATE_GAME_DATA: {
			// void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient);
			// this is just to be hooked by plugins, so ignore everything
			break;
		}
		case G_DROP_CLIENT: {
			// void trap_DropClient(int clientNum, const char *reason);
			intptr_t clientnum = args[0];
			orig_import.AddCommandString((char*)fmt::format("kick {}\n", clientnum).c_str());
			break;
		}
		case G_GET_USERINFO: {
			// void trap_GetUserinfo(int num, char *buffer, int bufferSize);
			intptr_t num = args[0];
			char* buffer = (char*)args[1];
			intptr_t bufferSize = args[2];
			*buffer = '\0';
			if (s_userinfo.count(num))
				strncpyz(buffer, s_userinfo[num].c_str(), bufferSize);
			break;
		}
		case G_GET_ENTITY_TOKEN: {
			// bool trap_GetEntityToken(char *buffer, int bufferSize);
			if (s_tokencount >= s_entity_tokens.size()) {
				ret = false;
				break;
			}

			char* buffer = (char*)args[0];
			intptr_t bufferSize = args[1];

			strncpyz(buffer, s_entity_tokens[s_tokencount++].c_str(), bufferSize);
			ret = true;
			break;
		}
		case G_GET_CONFIGSTRING: {
			// const char* (*get_configstring)(int num);
			intptr_t num = args[0];

			if (s_configstrings.count(num))
				ret = (intptr_t)s_configstrings[num].c_str();

			break;
		}

		default:
			break;
	};

	// do anything that needs to be done after function call here

#ifdef _DEBUG
	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("QUAKE2_syscall({} {}) returning {}\n", QUAKE2_eng_msg_names(cmd), cmd, ret);
#endif

	return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t QUAKE2_vmMain(intptr_t cmd, ...) {
	QMM_GET_VMMAIN_ARGS();

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_vmMain({} {}) called\n", QUAKE2_mod_msg_names(cmd), cmd);

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
		ROUTE_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES);
		ROUTE_EXPORT(WriteGame, GAME_WRITE_GAME);
		ROUTE_EXPORT(ReadGame, GAME_READ_GAME);
		ROUTE_EXPORT(WriteLevel, GAME_WRITE_LEVEL);
		ROUTE_EXPORT(ReadLevel, GAME_READ_LEVEL);
		ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
		ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
		ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
		ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
		ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
		ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
		ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
		ROUTE_EXPORT(ServerCommand, GAME_SERVER_COMMAND);

		// handle cmds for variables, this is how a plugin would get these values if needed
		ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
		ROUTE_EXPORT_VAR(edicts, GAMEVP_EDICTS);
		ROUTE_EXPORT_VAR(edict_size, GAMEV_EDICT_SIZE);
		ROUTE_EXPORT_VAR(num_edicts, GAMEV_NUM_EDICTS);
		ROUTE_EXPORT_VAR(max_edicts, GAMEV_MAX_EDICTS);

		default:
			break;
	};

	// if entity data changed, send a G_LOCATE_GAME_DATA so plugins can hook it
	if (qmm_export.edicts != orig_export->edicts
		|| qmm_export.edict_size != orig_export->edict_size
		|| qmm_export.num_edicts != orig_export->num_edicts
		) {

		edict_t* edicts = orig_export->edicts;
		intptr_t edict_size = orig_export->edict_size;

		if (edicts) {
			gclient_t* clients = nullptr;
			intptr_t clientsize = 0;
			// only do clients if this isn't GAME_INIT
			if (cmd != GAME_INIT) {
				edict_t* edict1 = (edict_t*)((intptr_t)edicts + edict_size);
				edict_t* edict2 = (edict_t*)((intptr_t)edicts + edict_size * 2);
				clients = edict1->client;
				clientsize = (intptr_t)(edict2->client) - (intptr_t)clients;
			}
			// this will trigger this message to be fired to plugins, and then it will be handled
			// by the empty "case G_LOCATE_GAME_DATA" above in QUAKE2_syscall
			qmm_syscall(G_LOCATE_GAME_DATA, (intptr_t)edicts, orig_export->num_edicts, edict_size, (intptr_t)clients, clientsize);
		}
	}

	// after the mod is called into by the engine, some of the variables in the mod's exports may have changed (num_edicts in particular)
	// and these changes need to be available to the engine, so copy those values again now before returning from the mod
	qmm_export.edicts = orig_export->edicts;
	qmm_export.edict_size = orig_export->edict_size;
	qmm_export.num_edicts = orig_export->num_edicts;
	qmm_export.max_edicts = orig_export->max_edicts;

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_vmMain({} {}) returning {}\n", QUAKE2_mod_msg_names(cmd), cmd, ret);

	return ret;
}


void* QUAKE2_GetGameAPI(void* import) {
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_GetGameAPI({}) called\n", import);

	// original import struct from engine
	// the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
	game_import_t* gi = (game_import_t*)import;
	orig_import = *gi;

	// fill in variables of our hooked import struct to pass to the mod
	// qmm_import.x = orig_import.x;

	// this gets passed to the mod's GetGameAPI() function in mod.cpp:mod_load()
	g_gameinfo.api_info.qmm_import = &qmm_import;

	// this isn't used anywhere except returning from this function, but store it in g_gameinfo.api_info for consistency
	g_gameinfo.api_info.qmm_export = &qmm_export;

	// pointer to wrapper vmMain function that calls actual mod func from orig_export
	// this gets assigned to g_mod->pfnvmMain in mod.cpp:mod_load()
	g_gameinfo.api_info.orig_vmmain = QUAKE2_vmMain;

	// pointer to wrapper syscall function that calls actual engine func from orig_import
	g_gameinfo.pfnsyscall = QUAKE2_syscall;

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_GetGameAPI({}) returning {}\n", import, (void*)&qmm_export);

	// struct full of export lambdas to QMM's vmMain
	// this gets returned to the game engine, but we haven't loaded the mod yet.
	// the only thing in this struct the engine uses before calling Init is the apiversion
	return &qmm_export;
}


const char* QUAKE2_eng_msg_names(intptr_t cmd) {
	switch (cmd) {
		GEN_CASE(G_BPRINTF);
		GEN_CASE(G_DPRINTF);
		GEN_CASE(G_CPRINTF);
		GEN_CASE(G_CENTERPRINTF);
		GEN_CASE(G_SOUND);
		GEN_CASE(G_POSITIONED_SOUND);
		GEN_CASE(G_CONFIGSTRING);
		GEN_CASE(G_ERROR);
		GEN_CASE(G_MODELINDEX);
		GEN_CASE(G_SOUNDINDEX);
		GEN_CASE(G_IMAGEINDEX);
		GEN_CASE(G_SETMODEL);
		GEN_CASE(G_TRACE);
		GEN_CASE(G_POINT_CONTENTS);
		GEN_CASE(G_IN_PVS);
		GEN_CASE(G_IN_PHS);
		GEN_CASE(G_SETAREAPORTALSTATE);
		GEN_CASE(G_AREAS_CONNECTED);
		GEN_CASE(G_LINKENTITY);
		GEN_CASE(G_UNLINKENTITY);
		GEN_CASE(G_BOXEDICTS);
		GEN_CASE(G_PMOVE);
		GEN_CASE(G_MULTICAST);
		GEN_CASE(G_UNICAST);
		GEN_CASE(G_MSG_WRITECHAR);
		GEN_CASE(G_MSG_WRITEBYTE);
		GEN_CASE(G_MSG_WRITESHORT);
		GEN_CASE(G_MSG_WRITELONG);
		GEN_CASE(G_MSG_WRITEFLOAT);
		GEN_CASE(G_MSG_WRITESTRING);
		GEN_CASE(G_MSG_WRITEPOSITION);
		GEN_CASE(G_MSG_WRITEDIR);
		GEN_CASE(G_MSG_WRITEANGLE);
		GEN_CASE(G_TAGMALLOC);
		GEN_CASE(G_TAGFREE);
		GEN_CASE(G_FREETAGS);
		GEN_CASE(G_CVAR);
		GEN_CASE(G_CVAR_SET);
		GEN_CASE(G_CVAR_FORCESET);
		GEN_CASE(G_ARGC);
		GEN_CASE(G_ARGV);
		GEN_CASE(G_ARGS);
		GEN_CASE(G_ADDCOMMANDSTRING);
		GEN_CASE(G_DEBUGGRAPH);

		// polyfills
		GEN_CASE(G_CVAR_REGISTER);
		GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
		GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
		GEN_CASE(G_SEND_CONSOLE_COMMAND);
		GEN_CASE(G_PRINT);

		GEN_CASE(G_FS_FOPEN_FILE);
		GEN_CASE(G_FS_READ);
		GEN_CASE(G_FS_WRITE);
		GEN_CASE(G_FS_FCLOSE_FILE);

		GEN_CASE(G_LOCATE_GAME_DATA);
		GEN_CASE(G_DROP_CLIENT);
		GEN_CASE(G_GET_USERINFO);
		GEN_CASE(G_GET_ENTITY_TOKEN);
		GEN_CASE(G_GET_CONFIGSTRING);

	default:
		return "unknown";
	}
}


const char* QUAKE2_mod_msg_names(intptr_t cmd) {
	switch (cmd) {
		GEN_CASE(GAMEV_APIVERSION);
		GEN_CASE(GAME_INIT);
		GEN_CASE(GAME_SHUTDOWN);
		GEN_CASE(GAME_SPAWN_ENTITIES);
		GEN_CASE(GAME_WRITE_GAME);
		GEN_CASE(GAME_READ_GAME);
		GEN_CASE(GAME_WRITE_LEVEL);
		GEN_CASE(GAME_READ_LEVEL);
		GEN_CASE(GAME_CLIENT_CONNECT);
		GEN_CASE(GAME_CLIENT_BEGIN);
		GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
		GEN_CASE(GAME_CLIENT_DISCONNECT);
		GEN_CASE(GAME_CLIENT_COMMAND);
		GEN_CASE(GAME_CLIENT_THINK);
		GEN_CASE(GAME_RUN_FRAME);
		GEN_CASE(GAME_SERVER_COMMAND);
		GEN_CASE(GAMEVP_EDICTS);
		GEN_CASE(GAMEV_EDICT_SIZE);
		GEN_CASE(GAMEV_NUM_EDICTS);
		GEN_CASE(GAMEV_MAX_EDICTS);

	default:
		return "unknown";
	}
}
