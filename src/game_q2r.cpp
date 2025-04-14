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
#define GAME_INCLUDE
#include <q2r/rerelease/game.h>
#undef GAME_INCLUDE
#include "game_api.h"
#include "log.h"
// QMM-specific Q2R header
#include "game_q2r.h"
#include "main.h"

GEN_QMM_MSGS(Q2R);
GEN_EXTS(Q2R);

// a copy of the original import struct that comes from the game engine. this is given to plugins
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod. this is given to plugins
static game_export_t* orig_export = nullptr;

// sound gets messed up (due to the float args in varargs?), so this will call the original first then QMM
// so plugins can still get called when G_SOUND fires
static void s_syscall_sound(edict_t* arg0, soundchan_t arg1, int arg2, float arg3, float arg4, float arg5) {
	orig_import.sound(arg0, arg1, arg2, arg3, arg4, arg5);
	syscall(G_SOUND, arg0, arg1, arg2, arg3, arg4, arg5);
}

// struct with lambdas that call QMM's syscall function. this is given to the mod
static game_import_t qmm_import = {
	0, // tick_rate
	0, // frame_time_s
	0, // frame_time_ms
	GEN_IMPORT(Broadcast_Print, G_BROADCAST_PRINT),
	GEN_IMPORT(Com_Print, G_COM_PRINT),
	GEN_IMPORT(Client_Print, G_CLIENT_PRINT),
	GEN_IMPORT(Center_Print, G_CENTERPRINT),
	s_syscall_sound, // GEN_IMPORT(sound, G_SOUND),
	GEN_IMPORT(positioned_sound, G_POSITIONED_SOUND),
	GEN_IMPORT(local_sound, G_LOCAL_SOUND),
	GEN_IMPORT(configstring, G_CONFIGSTRING),
	GEN_IMPORT(get_configstring, G_GET_CONFIGSTRING),
	GEN_IMPORT(Com_Error, G_COM_ERROR),
	GEN_IMPORT(modelindex, G_MODELINDEX),
	GEN_IMPORT(soundindex, G_SOUNDINDEX),
	GEN_IMPORT(imageindex, G_IMAGEINDEX),
	GEN_IMPORT(setmodel, G_SETMODEL),
	GEN_IMPORT(trace, G_TRACE),
	GEN_IMPORT(clip, G_CLIP),
	GEN_IMPORT(pointcontents, G_POINTCONTENTS),
	GEN_IMPORT(inPVS, G_INPVS),
	GEN_IMPORT(inPHS, G_INPHS),
	GEN_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE),
	GEN_IMPORT(AreasConnected, G_AREASCONNECTED),
	GEN_IMPORT(linkentity, G_LINKENTITY),
	GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
	GEN_IMPORT(BoxEdicts, G_BOXEDICTS),
	GEN_IMPORT(multicast, G_MULTICAST),
	GEN_IMPORT(unicast, G_UNICAST),
	GEN_IMPORT(WriteChar, G_MSG_WRITECHAR),
	GEN_IMPORT(WriteByte, G_MSG_WRITEBYTE),
	GEN_IMPORT(WriteShort, G_MSG_WRITESHORT),
	GEN_IMPORT(WriteLong, G_MSG_WRITELONG),
	GEN_IMPORT(WriteFloat, G_MSG_WRITEFLOAT),
	GEN_IMPORT(WriteString, G_MSG_WRITESTRING),
	GEN_IMPORT(WritePosition, G_MSG_WRITEPOSITION),
	GEN_IMPORT(WriteDir, G_MSG_WRITEDIR),
	GEN_IMPORT(WriteAngle, G_MSG_WRITEANGLE),
	GEN_IMPORT(WriteEntity, G_MSG_WRITEENTITY),
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
	GEN_IMPORT(DebugGraph, G_DEBUGGRAPH),
	GEN_IMPORT(GetExtension, G_GET_EXTENSION),
	GEN_IMPORT(Bot_RegisterEdict, G_BOT_REGISTEREDICT),
	GEN_IMPORT(Bot_UnRegisterEdict, G_BOT_UNREGISTEREDICT),
	GEN_IMPORT(Bot_MoveToPoint, G_BOT_MOVETOPOINT),
	GEN_IMPORT(Bot_FollowActor, G_BOT_FOLLOWACTOR),
	GEN_IMPORT(GetPathToGoal, G_GETPATHTOGOAL),
	GEN_IMPORT(Loc_Print, G_LOC_PRINT),
	GEN_IMPORT(Draw_Line, G_DRAW_LINE),
	GEN_IMPORT(Draw_Point, G_DRAW_POINT),
	GEN_IMPORT(Draw_Circle, G_DRAW_CIRCLE),
	GEN_IMPORT(Draw_Bounds, G_DRAW_BOUNDS),
	GEN_IMPORT(Draw_Sphere, G_DRAW_SPHERE),
	GEN_IMPORT(Draw_OrientedWorldText, G_DRAW_ORIENTEDWORLDTEXT),
	GEN_IMPORT(Draw_StaticWorldText, G_DRAW_STATICWORLDTEXT),
	GEN_IMPORT(Draw_Cylinder, G_DRAW_CYLINDER),
	GEN_IMPORT(Draw_Ray, G_DRAW_RAY),
	GEN_IMPORT(Draw_Arrow, G_DRAW_ARROW),
	GEN_IMPORT(ReportMatchDetails_Multicast, G_REPORTMATCHDETAILS_MULTICAST),
	GEN_IMPORT(ServerFrame, G_SERVER_FRAME),
	GEN_IMPORT(SendToClipBoard, G_SENDTOCLIPBOARD),
	GEN_IMPORT(Info_ValueForKey, G_INFO_VALUEFORKEY),
	GEN_IMPORT(Info_RemoveKey, G_INFO_REMOVEKEY),
	GEN_IMPORT(Info_SetValueForKey, G_INFO_SETVALUEFORKEY),
};

// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
	GAME_API_VERSION,	// apiversion
	GEN_EXPORT(PreInit, GAME_PREINIT),
	GEN_EXPORT(Init, GAME_INIT_EX),
	GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
	GEN_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES),
	GEN_EXPORT(WriteGameJson, GAME_WRITE_GAME_JSON),
	GEN_EXPORT(ReadGameJson, GAME_READ_GAME_JSON),
	GEN_EXPORT(WriteLevelJson, GAME_WRITE_LEVEL_JSON),
	GEN_EXPORT(ReadLevelJson, GAME_READ_LEVEL_JSON),
	GEN_EXPORT(CanSave, GAME_CAN_SAVE),
	GEN_EXPORT(ClientChooseSlot, GAME_CLIENT_CHOOSESLOT),
	GEN_EXPORT(ClientConnect, GAME_CLIENT_CONNECT),
	GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
	GEN_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED),
	GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
	GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
	GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
	GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
	GEN_EXPORT(PrepFrame, GAME_PREP_FRAME),
	GEN_EXPORT(ServerCommand, GAME_SERVER_COMMAND),
	// the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
	nullptr,			// edicts
	0,					// edict_size
	0,					// num_edicts
	0,					// max_edicts
	SERVER_FLAGS_NONE,	// server_flags (0)
	GEN_EXPORT(Pmove, GAME_PMOVE),
	GEN_EXPORT(GetExtension, GAME_GET_EXTENSION),
	GEN_EXPORT(Bot_SetWeapon, GAME_BOT_SETWEAPON),
	GEN_EXPORT(Bot_TriggerEdict, GAME_BOT_TRIGGEREDICT),
	GEN_EXPORT(Bot_UseItem, GAME_BOT_USEITEM),
	GEN_EXPORT(Bot_GetItemID, GAME_BOT_GETITEMID),
	GEN_EXPORT(Edict_ForceLookAtPoint, GAME_EDICT_FORCELOOKATPOINT),
	GEN_EXPORT(Bot_PickedUpItem, GAME_BOT_PICKEDUPITEM),
	GEN_EXPORT(Entity_IsVisibleToPlayer, GAME_ENTITY_ISVISIBLETOPLAYER),
	GEN_EXPORT(GetShadowLightData, GAME_GETSHADOWLIGHTDATA),
};

// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t Q2R_syscall(intptr_t cmd, ...) {
	QMM_GET_SYSCALL_ARGS();

	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q2R_syscall({}) called\n", Q2R_eng_msg_names(cmd));

	// before the engine is called into by the mod, some of the variables in the mod's exports may have changed
	// and these changes need to be available to the engine, so copy those values before entering the engine
	qmm_export.edicts = orig_export->edicts;
	qmm_export.edict_size = orig_export->edict_size;
	qmm_export.num_edicts = orig_export->num_edicts;
	qmm_export.max_edicts = orig_export->max_edicts;
	qmm_export.server_flags = orig_export->server_flags;

	// store return value in case we do some stuff after the function call is over
	intptr_t ret = 0;

	switch (cmd) {
		ROUTE_IMPORT(Broadcast_Print, G_BROADCAST_PRINT);
		ROUTE_IMPORT(Com_Print, G_COM_PRINT);
		ROUTE_IMPORT(Client_Print, G_CLIENT_PRINT);
		ROUTE_IMPORT(Center_Print, G_CENTERPRINT);
		ROUTE_IMPORT(sound, G_SOUND);
		ROUTE_IMPORT(positioned_sound, G_POSITIONED_SOUND);
		ROUTE_IMPORT(local_sound, G_LOCAL_SOUND);
		ROUTE_IMPORT(configstring, G_CONFIGSTRING);
		ROUTE_IMPORT(get_configstring, G_GET_CONFIGSTRING);
		ROUTE_IMPORT(Com_Error, G_COM_ERROR);
		ROUTE_IMPORT(modelindex, G_MODELINDEX);
		ROUTE_IMPORT(soundindex, G_SOUNDINDEX);
		ROUTE_IMPORT(imageindex, G_IMAGEINDEX);
		ROUTE_IMPORT(setmodel, G_SETMODEL);
		ROUTE_IMPORT(trace, G_TRACE);
		ROUTE_IMPORT(clip, G_CLIP);
		ROUTE_IMPORT(pointcontents, G_POINTCONTENTS);
		ROUTE_IMPORT(inPVS, G_INPVS);
		ROUTE_IMPORT(inPHS, G_INPHS);
		ROUTE_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE);
		ROUTE_IMPORT(AreasConnected, G_AREASCONNECTED);
		ROUTE_IMPORT(linkentity, G_LINKENTITY);
		ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
		ROUTE_IMPORT(BoxEdicts, G_BOXEDICTS);
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
		ROUTE_IMPORT(WriteEntity, G_MSG_WRITEENTITY);
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
		ROUTE_IMPORT(GetExtension, G_GET_EXTENSION);
		ROUTE_IMPORT(Bot_RegisterEdict, G_BOT_REGISTEREDICT);
		ROUTE_IMPORT(Bot_UnRegisterEdict, G_BOT_UNREGISTEREDICT);
		ROUTE_IMPORT(Bot_MoveToPoint, G_BOT_MOVETOPOINT);
		ROUTE_IMPORT(Bot_FollowActor, G_BOT_FOLLOWACTOR);
		ROUTE_IMPORT(GetPathToGoal, G_GETPATHTOGOAL);
		ROUTE_IMPORT(Loc_Print, G_LOC_PRINT);
		ROUTE_IMPORT(Draw_Line, G_DRAW_LINE);
		ROUTE_IMPORT(Draw_Point, G_DRAW_POINT);
		ROUTE_IMPORT(Draw_Circle, G_DRAW_CIRCLE);
		ROUTE_IMPORT(Draw_Bounds, G_DRAW_BOUNDS);
		ROUTE_IMPORT(Draw_Sphere, G_DRAW_SPHERE);
		ROUTE_IMPORT(Draw_OrientedWorldText, G_DRAW_ORIENTEDWORLDTEXT);
		ROUTE_IMPORT(Draw_StaticWorldText, G_DRAW_STATICWORLDTEXT);
		ROUTE_IMPORT(Draw_Cylinder, G_DRAW_CYLINDER);
		ROUTE_IMPORT(Draw_Ray, G_DRAW_RAY);
		ROUTE_IMPORT(Draw_Arrow, G_DRAW_ARROW);
		ROUTE_IMPORT(ReportMatchDetails_Multicast, G_REPORTMATCHDETAILS_MULTICAST);
		ROUTE_IMPORT(ServerFrame, G_SERVER_FRAME);
		ROUTE_IMPORT(SendToClipBoard, G_SENDTOCLIPBOARD);
		ROUTE_IMPORT(Info_ValueForKey, G_INFO_VALUEFORKEY);
		ROUTE_IMPORT(Info_RemoveKey, G_INFO_REMOVEKEY);
		ROUTE_IMPORT(Info_SetValueForKey, G_INFO_SETVALUEFORKEY);

		// handle cmds for variables, this is how a plugin would get these values if needed
		ROUTE_IMPORT_VAR(tick_rate, GV_TICK_RATE);
		ROUTE_IMPORT_VAR(frame_time_s, GV_FRAME_TIME_S);
		ROUTE_IMPORT_VAR(frame_time_ms, GV_FRAME_TIME_MS);

		// handle special cmds which QMM uses but MOHAA doesn't have an analogue for
	case G_CVAR_REGISTER: {
		// q2r: cvar_t *(*cvar) (char *var_name, char *value, int flags);
		// qmm: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
		// qmm always passes NULL for vmCvar so don't worry about it
		char* var_name = (char*)(args[1]);
		char* value = (char*)(args[2]);
		cvar_flags_t flags = (cvar_flags_t)args[3];
		(void)orig_import.cvar(var_name, value, flags);
		break;
	}
	case G_CVAR_VARIABLE_STRING_BUFFER: {
		// q2r: cvar_t *(*cvar) (char *var_name, char *value, int flags);
		// qmm: void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
		char* var_name = (char*)(args[0]);
		char* buffer = (char*)(args[1]);
		int bufsize = (int)args[2];
		*buffer = '\0';
		cvar_t* cvar = orig_import.cvar(var_name, (char*)"", CVAR_NOFLAGS);
		if (cvar)
			strncpy(buffer, cvar->string, bufsize);
		buffer[bufsize - 1] = '\0';
		break;
	}
	case G_CVAR_VARIABLE_INTEGER_VALUE: {
		// q2r: cvar_t *(*cvar) (char *var_name, char *value, int flags);
		// qmm: int trap_Cvar_VariableIntegerValue(const char* var_name)
		char* var_name = (char*)(args[0]);
		cvar_t* cvar = orig_import.cvar(var_name, (char*)"", CVAR_NOFLAGS);
		if (cvar)
			ret = cvar->integer;
		break;
	}
	case G_SEND_CONSOLE_COMMAND: {
		// Q2R: void (*AddCommandString)(const char *text);
		// qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
		const char* text = (const char*)(args[1]);
		orig_import.AddCommandString(text);
		break;
	}
	case G_FS_FOPEN_FILE:
	case G_FS_READ:
	case G_FS_WRITE:
	case G_FS_FCLOSE_FILE:
		// these don't get called by QMM in Q2R (only in engines with QVM mods)
		// these are included here only for completeness, really
		break;

	default:
		break;
	};

	// do anything that needs to be done after function call here

	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q2R_syscall({}) returning {}\n", Q2R_eng_msg_names(cmd), ret);

	return ret;
}

// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t Q2R_vmMain(intptr_t cmd, ...) {
	QMM_GET_VMMAIN_ARGS();

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q2R_vmMain({}) called\n", Q2R_mod_msg_names(cmd));

	// store copy of mod's export pointer (this is stored in g_gameinfo.api_info in mod_load)
	if (!orig_export)
		orig_export = (game_export_t*)(g_gameinfo.api_info.orig_export);

	// store return value since we do some stuff after the function call is over
	intptr_t ret = 0;

	switch (cmd) {
		ROUTE_EXPORT(PreInit, GAME_PREINIT);
		ROUTE_EXPORT(Init, GAME_INIT_EX);
		ROUTE_EXPORT(Shutdown, GAME_SHUTDOWN);
		ROUTE_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES);
		ROUTE_EXPORT(WriteGameJson, GAME_WRITE_GAME_JSON);
		ROUTE_EXPORT(ReadGameJson, GAME_READ_GAME_JSON);
		ROUTE_EXPORT(WriteLevelJson, GAME_WRITE_LEVEL_JSON);
		ROUTE_EXPORT(ReadLevelJson, GAME_READ_LEVEL_JSON);
		ROUTE_EXPORT(CanSave, GAME_CAN_SAVE);
		ROUTE_EXPORT(ClientChooseSlot, GAME_CLIENT_CHOOSESLOT);
		ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
		ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
		ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
		ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
		ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
		ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
		ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
		ROUTE_EXPORT(PrepFrame, GAME_PREP_FRAME);
		ROUTE_EXPORT(ServerCommand, GAME_SERVER_COMMAND);
		ROUTE_EXPORT(Pmove, GAME_PMOVE);
		ROUTE_EXPORT(GetExtension, GAME_GET_EXTENSION);
		ROUTE_EXPORT(Bot_SetWeapon, GAME_BOT_SETWEAPON);
		ROUTE_EXPORT(Bot_TriggerEdict, GAME_BOT_TRIGGEREDICT);
		ROUTE_EXPORT(Bot_UseItem, GAME_BOT_USEITEM);
		ROUTE_EXPORT(Bot_GetItemID, GAME_BOT_GETITEMID);
		ROUTE_EXPORT(Edict_ForceLookAtPoint, GAME_EDICT_FORCELOOKATPOINT);
		ROUTE_EXPORT(Bot_PickedUpItem, GAME_BOT_PICKEDUPITEM);
		ROUTE_EXPORT(Entity_IsVisibleToPlayer, GAME_ENTITY_ISVISIBLETOPLAYER);
		ROUTE_EXPORT(GetShadowLightData, GAME_GETSHADOWLIGHTDATA);

		// handle cmds for variables, this is how a plugin would get these values if needed
		ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
		ROUTE_EXPORT_VAR(edicts, GAMEVP_EDICTS);
		ROUTE_EXPORT_VAR(edict_size, GAMEV_EDICT_SIZE);
		ROUTE_EXPORT_VAR(num_edicts, GAMEV_NUM_EDICTS);
		ROUTE_EXPORT_VAR(max_edicts, GAMEV_MAX_EDICTS);
		ROUTE_EXPORT_VAR(server_flags, GAMEV_SERVER_FLAGS);

	default:
		break;
	};

	// after the mod is called into by the engine, some of the variables in the mod's exports may have changed (num_entities and errorMessage in particular)
	// and these changes need to be available to the engine, so copy those values again now before returning from the mod
	qmm_export.edicts = orig_export->edicts;
	qmm_export.edict_size = orig_export->edict_size;
	qmm_export.num_edicts = orig_export->num_edicts;
	qmm_export.max_edicts = orig_export->max_edicts;
	qmm_export.server_flags = orig_export->server_flags;

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q2R_vmMain({}) returning {}\n", Q2R_mod_msg_names(cmd), ret);

	return ret;
}

void* Q2R_GetGameAPI(void* import) {
	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q2R_GetGameAPI({}) called\n", import);

	// original import struct from engine
	// the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
	game_import_t* gi = (game_import_t*)import;
	orig_import = *gi;

	// fill in variables of our hooked import struct to pass to the mod
	qmm_import.tick_rate = orig_import.tick_rate;
	qmm_import.frame_time_s = orig_import.frame_time_s;
	qmm_import.frame_time_ms = orig_import.frame_time_ms;
	// qmm_import.sound = orig_import.sound;

	// this gets passed to the mod's GetGameAPI() function in mod.cpp:mod_load()
	g_gameinfo.api_info.qmm_import = &qmm_import;

	// this isn't used anywhere except returning from this function, but store it in g_gameinfo.api_info for consistency
	g_gameinfo.api_info.qmm_export = &qmm_export;

	// pointer to wrapper vmMain function that calls actual mod func from orig_export
	// this gets assigned to g_mod->pfnvmMain in mod.cpp:mod_load()
	g_gameinfo.api_info.orig_vmmain = Q2R_vmMain;

	// pointer to wrapper syscall function that calls actual engine func from orig_import
	g_gameinfo.pfnsyscall = Q2R_syscall;

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q2R_GetGameAPI({}) returning {}\n", import, (void*)&qmm_export);

	// struct full of export lambdas to QMM's vmMain
	// this gets returned to the game engine, but we haven't loaded the mod yet.
	// the only thing in this struct the engine uses before calling Init is the apiversion
	return &qmm_export;
}

const char* Q2R_eng_msg_names(intptr_t cmd) {
	switch (cmd) {
		GEN_CASE(G_BROADCAST_PRINT);
		GEN_CASE(G_COM_PRINT);
		GEN_CASE(G_CLIENT_PRINT);
		GEN_CASE(G_CENTERPRINT);
		GEN_CASE(G_SOUND);
		GEN_CASE(G_POSITIONED_SOUND);
		GEN_CASE(G_LOCAL_SOUND);
		GEN_CASE(G_CONFIGSTRING);
		GEN_CASE(G_GET_CONFIGSTRING);
		GEN_CASE(G_COM_ERROR);
		GEN_CASE(G_MODELINDEX);
		GEN_CASE(G_SOUNDINDEX);
		GEN_CASE(G_IMAGEINDEX);
		GEN_CASE(G_SETMODEL);
		GEN_CASE(G_TRACE);
		GEN_CASE(G_CLIP);
		GEN_CASE(G_POINTCONTENTS);
		GEN_CASE(G_INPVS);
		GEN_CASE(G_INPHS);
		GEN_CASE(G_SETAREAPORTALSTATE);
		GEN_CASE(G_AREASCONNECTED);
		GEN_CASE(G_LINKENTITY);
		GEN_CASE(G_UNLINKENTITY);
		GEN_CASE(G_BOXEDICTS);
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
		GEN_CASE(G_MSG_WRITEENTITY);
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
		GEN_CASE(G_GET_EXTENSION);
		GEN_CASE(G_BOT_REGISTEREDICT);
		GEN_CASE(G_BOT_UNREGISTEREDICT);
		GEN_CASE(G_BOT_MOVETOPOINT);
		GEN_CASE(G_BOT_FOLLOWACTOR);
		GEN_CASE(G_GETPATHTOGOAL);
		GEN_CASE(G_LOC_PRINT);
		GEN_CASE(G_DRAW_LINE);
		GEN_CASE(G_DRAW_POINT);
		GEN_CASE(G_DRAW_CIRCLE);
		GEN_CASE(G_DRAW_BOUNDS);
		GEN_CASE(G_DRAW_SPHERE);
		GEN_CASE(G_DRAW_ORIENTEDWORLDTEXT);
		GEN_CASE(G_DRAW_STATICWORLDTEXT);
		GEN_CASE(G_DRAW_CYLINDER);
		GEN_CASE(G_DRAW_RAY);
		GEN_CASE(G_DRAW_ARROW);
		GEN_CASE(G_REPORTMATCHDETAILS_MULTICAST);
		GEN_CASE(G_SERVER_FRAME);
		GEN_CASE(G_SENDTOCLIPBOARD);
		GEN_CASE(G_INFO_VALUEFORKEY);
		GEN_CASE(G_INFO_REMOVEKEY);
		GEN_CASE(G_INFO_SETVALUEFORKEY);

		// special cmds
		GEN_CASE(G_CVAR_REGISTER);
		GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
		GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
		GEN_CASE(G_SEND_CONSOLE_COMMAND);
		GEN_CASE(G_FS_FOPEN_FILE);
		GEN_CASE(G_FS_READ);
		GEN_CASE(G_FS_WRITE);
		GEN_CASE(G_FS_FCLOSE_FILE);

	default:
		return "unknown";
	}
}

const char* Q2R_mod_msg_names(intptr_t cmd) {
	switch (cmd) {
		GEN_CASE(GAMEV_APIVERSION);
		GEN_CASE(GAME_PREINIT);
		GEN_CASE(GAME_INIT_EX);
		GEN_CASE(GAME_SHUTDOWN);
		GEN_CASE(GAME_SPAWN_ENTITIES);
		GEN_CASE(GAME_WRITE_GAME_JSON);
		GEN_CASE(GAME_READ_GAME_JSON);
		GEN_CASE(GAME_WRITE_LEVEL_JSON);
		GEN_CASE(GAME_READ_LEVEL_JSON);
		GEN_CASE(GAME_CAN_SAVE);
		GEN_CASE(GAME_CLIENT_CHOOSESLOT);
		GEN_CASE(GAME_CLIENT_CONNECT);
		GEN_CASE(GAME_CLIENT_BEGIN);
		GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
		GEN_CASE(GAME_CLIENT_DISCONNECT);
		GEN_CASE(GAME_CLIENT_COMMAND);
		GEN_CASE(GAME_CLIENT_THINK);
		GEN_CASE(GAME_RUN_FRAME);
		GEN_CASE(GAME_PREP_FRAME);
		GEN_CASE(GAME_SERVER_COMMAND);
		GEN_CASE(GAMEVP_EDICTS);
		GEN_CASE(GAMEV_EDICT_SIZE);
		GEN_CASE(GAMEV_NUM_EDICTS);
		GEN_CASE(GAMEV_MAX_EDICTS);
		GEN_CASE(GAMEV_SERVER_FLAGS);
		GEN_CASE(GAME_PMOVE);
		GEN_CASE(GAME_GET_EXTENSION);
		GEN_CASE(GAME_BOT_SETWEAPON);
		GEN_CASE(GAME_BOT_TRIGGEREDICT);
		GEN_CASE(GAME_BOT_USEITEM);
		GEN_CASE(GAME_BOT_GETITEMID);
		GEN_CASE(GAME_EDICT_FORCELOOKATPOINT);
		GEN_CASE(GAME_BOT_PICKEDUPITEM);
		GEN_CASE(GAME_ENTITY_ISVISIBLETOPLAYER);
		GEN_CASE(GAME_GETSHADOWLIGHTDATA);

	default:
		return "unknown";
	}
}
