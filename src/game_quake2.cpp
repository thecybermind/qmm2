/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1
#include <string.h>
#include <stdio.h>
#include <quake2/game/q_shared.h>
#define GAME_INCLUDE
#include <quake2/game/game.h>
#undef GAME_INCLUDE
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


// struct with lambdas that call QMM's syscall function. this is given to the mod
static game_import_t qmm_import = {
	GEN_IMPORT(bprintf, G_BPRINTF),
	GEN_IMPORT(dprintf, G_DPRINTF),
	GEN_IMPORT(cprintf, G_CPRINTF),
	GEN_IMPORT(centerprintf, G_CENTERPRINTF),
	GEN_IMPORT_6(sound, G_SOUND, void, edict_t*, int, int, float, float, float),
	GEN_IMPORT_7(positioned_sound, G_POSITIONED_SOUND, void, vec3_t, edict_t*, int, int, float, float, float),
	GEN_IMPORT(configstring, G_CONFIGSTRING),
	GEN_IMPORT(error, G_ERROR),
	GEN_IMPORT(modelindex, G_MODELINDEX),
	GEN_IMPORT(soundindex, G_SOUNDINDEX),
	GEN_IMPORT(imageindex, G_IMAGEINDEX),
	GEN_IMPORT(setmodel, G_SETMODEL),
	GEN_IMPORT(trace, G_TRACE),
	GEN_IMPORT(pointcontents, G_POINTCONTENTS),
	GEN_IMPORT(inPVS, G_INPVS),
	GEN_IMPORT(inPHS, G_INPHS),
	GEN_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE),
	GEN_IMPORT(AreasConnected, G_AREASCONNECTED),
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


// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
	GAME_API_VERSION,	// apiversion
	GEN_EXPORT(Init, GAME_INIT),
	GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
	GEN_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES),
	GEN_EXPORT(WriteGame, GAME_WRITE_GAME),
	GEN_EXPORT(ReadGame, GAME_READ_GAME),
	GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
	GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
	GEN_EXPORT(ClientConnect, GAME_CLIENT_CONNECT),
	GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
	GEN_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED),
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

	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("QUAKE2_syscall({}) called\n", QUAKE2_eng_msg_names(cmd));

	// store copy of mod's export pointer. this is stored in g_gameinfo.api_info in mod_load(), or set to nullptr in mod_unload()
	orig_export = (game_export_t*)(g_gameinfo.api_info.orig_export);

	// before the engine is called into by the mod, some of the variables in the mod's exports may have changed
	// and these changes need to be available to the engine, so copy those values before entering the engine
	if (orig_export) {
		qmm_export.edicts = orig_export->edicts;
		qmm_export.edict_size = orig_export->edict_size;
		qmm_export.num_edicts = orig_export->num_edicts;
		qmm_export.max_edicts = orig_export->max_edicts;
	}

	// store return value in case we do some stuff after the function call is over
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
		ROUTE_IMPORT(pointcontents, G_POINTCONTENTS);
		ROUTE_IMPORT(inPVS, G_INPVS);
		ROUTE_IMPORT(inPHS, G_INPHS);
		ROUTE_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE);
		ROUTE_IMPORT(AreasConnected, G_AREASCONNECTED);
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

		// handle special cmds which QMM uses but MOHAA doesn't have an analogue for
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
				strncpy(buffer, cvar->string, bufsize);
			buffer[bufsize - 1] = '\0';
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
			void* buffer = (void*)args[0];
			intptr_t len = args[1];
			fileHandle_t f = (fileHandle_t)args[2];
			(void)fread(buffer, len, 1, (FILE*)f);
			break;
		}
		case G_FS_WRITE: {
			// void trap_FS_Write(const void* buffer, int len, fileHandle_t f);
			void* buffer = (void*)args[0];
			intptr_t len = args[1];
			fileHandle_t f = (fileHandle_t)args[2];
			fwrite(buffer, len, 1, (FILE*)f);
			break;
		}
		case G_FS_FCLOSE_FILE: {
			// void trap_FS_FCloseFile(fileHandle_t f);
			fileHandle_t f = (fileHandle_t)args[0];
			fclose((FILE*)f);
			break;
		}

		default:
			break;
	};

	// do anything that needs to be done after function call here

	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("QUAKE2_syscall({}) returning {}\n", QUAKE2_eng_msg_names(cmd), ret);

	return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t QUAKE2_vmMain(intptr_t cmd, ...) {
	QMM_GET_VMMAIN_ARGS();

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("QUAKE2_vmMain({}) called\n", QUAKE2_mod_msg_names(cmd));

	// store copy of mod's export pointer. this is stored in g_gameinfo.api_info in mod_load(), or set to nullptr in mod_unload()
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

	// after the mod is called into by the engine, some of the variables in the mod's exports may have changed (num_entities in particular)
	// and these changes need to be available to the engine, so copy those values again now before returning from the mod
	qmm_export.edicts = orig_export->edicts;
	qmm_export.edict_size = orig_export->edict_size;
	qmm_export.num_edicts = orig_export->num_edicts;
	qmm_export.max_edicts = orig_export->max_edicts;

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("QUAKE2_vmMain({}) returning {}\n", QUAKE2_mod_msg_names(cmd), ret);

	return ret;
}


void* QUAKE2_GetGameAPI(void* import) {
	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("QUAKE2_GetGameAPI({}) called\n", import);

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

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("QUAKE2_GetGameAPI({}) returning {}\n", import, (void*)&qmm_export);

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
		GEN_CASE(G_POINTCONTENTS);
		GEN_CASE(G_INPVS);
		GEN_CASE(G_INPHS);
		GEN_CASE(G_SETAREAPORTALSTATE);
		GEN_CASE(G_AREASCONNECTED);
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

		// special cmds
		GEN_CASE(G_CVAR_REGISTER);
		GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
		GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
		GEN_CASE(G_SEND_CONSOLE_COMMAND);
		GEN_CASE(G_PRINT);
		GEN_CASE(G_FS_FOPEN_FILE);
		GEN_CASE(G_FS_READ);
		GEN_CASE(G_FS_WRITE);
		GEN_CASE(G_FS_FCLOSE_FILE);
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
