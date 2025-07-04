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
#include <mohaa/qcommon/q_shared.h>
#define GAME_DLL
#include <mohaa/fgame/g_public.h>
#undef GAME_DLL
#include "game_api.h"
#include "log.h"
// QMM-specific MOHAA header
#include "game_mohaa.h"
#include "main.h"
#include "util.h"

GEN_QMM_MSGS(MOHAA);
GEN_EXTS(MOHAA);

// a copy of the original import struct that comes from the game engine. this is given to plugins
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod. this is given to plugins
static game_export_t* orig_export = nullptr;


// struct with lambdas that call QMM's syscall function. this is given to the mod
static game_import_t qmm_import = {
	GEN_IMPORT(Printf, G_PRINTF),
	GEN_IMPORT(DPrintf, G_DPRINTF),
	GEN_IMPORT(DPrintf2, G_DPRINTF2),
	GEN_IMPORT(DebugPrintf, G_DEBUGPRINTF),
	GEN_IMPORT(Error, G_ERROR),
	GEN_IMPORT(Milliseconds, G_MILLISECONDS),
	GEN_IMPORT(LV_ConvertString, G_LV_CONVERTSTRING),
	GEN_IMPORT(Malloc, G_MALLOC),
	GEN_IMPORT(Free, G_FREE),
	GEN_IMPORT(Cvar_Get, G_CVAR_GET),
	GEN_IMPORT(cvar_set, G_CVAR_SET),
	GEN_IMPORT(cvar_set2, G_CVAR_SET2),
	GEN_IMPORT(NextCvar, G_NEXTCVAR),
	GEN_IMPORT(Argc, G_ARGC),
	GEN_IMPORT(Argv, G_ARGV),
	GEN_IMPORT(Args, G_ARGS),
	GEN_IMPORT(AddCommand, G_ADDCOMMAND),
	GEN_IMPORT(FS_ReadFile, G_FS_READFILE),
	GEN_IMPORT(FS_FreeFile, G_FS_FREEFILE),
	GEN_IMPORT(FS_WriteFile, G_FS_WRITEFILE),
	GEN_IMPORT(FS_FOpenFileWrite, G_FS_FOPEN_FILE_WRITE),
	GEN_IMPORT(FS_FOpenFileAppend, G_FS_FOPEN_FILE_APPEND),
	GEN_IMPORT(FS_PrepFileWrite, G_FS_PREPFILEWRITE),
	GEN_IMPORT(FS_Write, G_FS_WRITE),
	GEN_IMPORT(FS_Read, G_FS_READ),
	GEN_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE),
	GEN_IMPORT(FS_Tell, G_FS_TELL),
	GEN_IMPORT(FS_Seek, G_FS_SEEK),
	GEN_IMPORT(FS_Flush, G_FS_FLUSH),
	GEN_IMPORT(FS_FileNewer, G_FS_FILENEWER),
	GEN_IMPORT(FS_CanonicalFilename, G_FS_CANONICALFILENAME),
	GEN_IMPORT(FS_ListFiles, G_FS_LISTFILES),
	GEN_IMPORT(FS_FreeFileList, G_FS_FREEFILELIST),
	GEN_IMPORT(GetArchiveFileName, G_GETARCHIVEFILENAME),
	GEN_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND_EX),
	GEN_IMPORT_1(DebugGraph, G_DEBUGGRAPH, void, float),
	GEN_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND),
	GEN_IMPORT(DropClient, G_DROPCLIENT),
	GEN_IMPORT(MSG_WriteBits, G_MSG_WRITEBITS),
	GEN_IMPORT(MSG_WriteChar, G_MSG_WRITECHAR),
	GEN_IMPORT(MSG_WriteByte, G_MSG_WRITEBYTE),
	GEN_IMPORT(MSG_WriteSVC, G_MSG_WRITESVC),
	GEN_IMPORT(MSG_WriteShort, G_MSG_WRITESHORT),
	GEN_IMPORT(MSG_WriteLong, G_MSG_WRITELONG),
	GEN_IMPORT_1(MSG_WriteFloat, G_MSG_WRITEFLOAT, void, float),
	GEN_IMPORT(MSG_WriteString, G_MSG_WRITESTRING),
	GEN_IMPORT_1(MSG_WriteAngle8, G_MSG_WRITEANGLE8, void, float),
	GEN_IMPORT_1(MSG_WriteAngle16, G_MSG_WRITEANGLE16, void, float),
	GEN_IMPORT_1(MSG_WriteCoord, G_MSG_WRITECOORD, void, float),
	GEN_IMPORT(MSG_WriteDir, G_MSG_WRITEDIR),
	GEN_IMPORT(MSG_StartCGM, G_MSG_STARTCGM),
	GEN_IMPORT(MSG_EndCGM, G_MSG_ENDCGM),
	GEN_IMPORT(MSG_SetClient, G_MSG_SETCLIENT),
	GEN_IMPORT(SetBroadcastVisible, G_SETBROADCASTVISIBLE),
	GEN_IMPORT(SetBroadcastHearable, G_SETBROADCASTHEARABLE),
	GEN_IMPORT(SetBroadcastAll, G_SETBROADCASTALL),
	GEN_IMPORT(setConfigstring, G_SET_CONFIGSTRING),
	GEN_IMPORT(getConfigstring, G_GET_CONFIGSTRING),
	GEN_IMPORT(SetUserinfo, G_SET_USERINFO),
	GEN_IMPORT(GetUserinfo, G_GET_USERINFO),
	GEN_IMPORT(SetBrushModel, G_SETBRUSHMODEL),
	GEN_IMPORT(ModelBoundsFromName, G_MODELBOUNDSFROMNAME),
	GEN_IMPORT(SightTraceEntity, G_SIGHTTRACEENTITY),
	GEN_IMPORT(SightTrace, G_SIGHTTRACE),
	GEN_IMPORT(trace, G_TRACE),
	GEN_IMPORT(GetShader, G_GETSHADER),
	GEN_IMPORT(pointcontents, G_POINTCONTENTS),
	GEN_IMPORT(PointBrushnum, G_POINTBRUSHNUM),
	GEN_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE),
	GEN_IMPORT(AreaForPoint, G_AREAFORPOINT),
	GEN_IMPORT(AreasConnected, G_AREASCONNECTED),
	GEN_IMPORT(InPVS, G_INPVS),
	GEN_IMPORT(linkentity, G_LINKENTITY),
	GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
	GEN_IMPORT(AreaEntities, G_AREAENTITIES),
	GEN_IMPORT(ClipToEntity, G_CLIPTOENTITY),
	GEN_IMPORT(imageindex, G_IMAGEINDEX),
	GEN_IMPORT(itemindex, G_ITEMINDEX),
	GEN_IMPORT(soundindex, G_SOUNDINDEX),
	GEN_IMPORT(TIKI_RegisterModel, G_TIKI_REGISTERMODEL),
	GEN_IMPORT(modeltiki, G_MODELTIKI),
	GEN_IMPORT(modeltikianim, G_MODELTIKIANIM),
	GEN_IMPORT(SetLightStyle, G_SETLIGHTSTYLE),
	GEN_IMPORT(GameDir, G_GAMEDIR),
	GEN_IMPORT(setmodel, G_SETMODEL),
	GEN_IMPORT(clearmodel, G_CLEARMODEL),
	GEN_IMPORT(TIKI_NumAnims, G_TIKI_NUMANIMS),
	GEN_IMPORT(TIKI_NumSurfaces, G_TIKI_NUMSURFACES),
	GEN_IMPORT(TIKI_NumTags, G_TIKI_NUMTAGS),
	GEN_IMPORT_4(TIKI_CalculateBounds, G_TIKI_CALCULATEBOUNDS, void, dtiki_t*, float, vec3_t, vec3_t),
	GEN_IMPORT(TIKI_GetSkeletor, G_TIKI_GETSKELETOR),
	GEN_IMPORT(Anim_NameForNum, G_ANIM_NAMEFORNUM),
	GEN_IMPORT(Anim_NumForName, G_ANIM_NUMFORNAME),
	GEN_IMPORT(Anim_Random, G_ANIM_RANDOM),
	GEN_IMPORT(Anim_NumFrames, G_ANIM_NUMFRAMES),
	GEN_IMPORT(Anim_Time, G_ANIM_TIME),
	GEN_IMPORT(Anim_Frametime, G_ANIM_FRAMETIME),
	GEN_IMPORT(Anim_CrossTime, G_ANIM_CROSSTIME),
	GEN_IMPORT(Anim_Delta, G_ANIM_DELTA),
	GEN_IMPORT(Anim_HasDelta, G_ANIM_HASDELTA),
	GEN_IMPORT_5(Anim_DeltaOverTime, G_ANIM_DELTAOVERTIME, void, dtiki_t*, int, float, float, float*),
	GEN_IMPORT(Anim_Flags, G_ANIM_FLAGS),
	GEN_IMPORT(Anim_FlagsSkel, G_ANIM_FLAGSSKEL),
	GEN_IMPORT(Anim_HasCommands, G_ANIM_HASCOMMANDS),
	GEN_IMPORT(NumHeadModels, G_NUMHEADMODELS),
	GEN_IMPORT(GetHeadModel, G_GETHEADMODEL),
	GEN_IMPORT(NumHeadSkins, G_NUMHEADSKINS),
	GEN_IMPORT(GetHeadSkin, G_GETHEADSKIN),
	GEN_IMPORT(Frame_Commands, G_FRAME_COMMANDS),
	GEN_IMPORT(Surface_NameToNum, G_SURFACE_NAMETONUM),
	GEN_IMPORT(Surface_NumToName, G_SURFACE_NUMTONAME),
	GEN_IMPORT(Tag_NumForName, G_TAG_NUMFORNAME),
	GEN_IMPORT(Tag_NameForNum, G_TAG_NAMEFORNUM),
	GEN_IMPORT(TIKI_OrientationInternal, G_TIKI_ORIENTATIONINTERNAL),
	// todo, change types to actually match float, but also need to return an intptr_t instead of orientation_t
	GEN_IMPORT(TIKI_TransformInternal, G_TIKI_TRANSFORMINTERNAL),
	GEN_IMPORT_4(TIKI_IsOnGroundInternal, G_TIKI_ISONGROUNDINTERNAL, qboolean, dtiki_t*, int, int, float),
	GEN_IMPORT_6(TIKI_SetPoseInternal, G_TIKI_SETPOSEINTERNAL, void, dtiki_t*, int, const frameInfo_t*, int*, vec4_t*, float),
	GEN_IMPORT(CM_GetHitLocationInfo, G_CM_GETHITLOCATIONINFO),
	GEN_IMPORT(CM_GetHitLocationInfoSecondary, G_CM_GETHITLOCATIONINFOSECONDARY),
	GEN_IMPORT(Alias_Add, G_ALIAS_ADD),
	GEN_IMPORT(Alias_FindRandom, G_ALIAS_FINDRANDOM),
	GEN_IMPORT(Alias_Dump, G_ALIAS_DUMP),
	GEN_IMPORT(Alias_Clear, G_ALIAS_CLEAR),
	GEN_IMPORT(Alias_UpdateDialog, G_ALIAS_UPDATEDIALOG),
	GEN_IMPORT(TIKI_NameForNum, G_TIKI_NAMEFORNUM),
	GEN_IMPORT(GlobalAlias_Add, G_GLOBALALIAS_ADD),
	GEN_IMPORT(GlobalAlias_FindRandom, G_GLOBALALIAS_FINDRANDOM),
	GEN_IMPORT(GlobalAlias_Dump, G_GLOBALALIAS_DUMP),
	GEN_IMPORT(GlobalAlias_Clear, G_GLOBALALIAS_CLEAR),
	GEN_IMPORT(centerprintf, G_CENTERPRINTF),
	GEN_IMPORT(locationprintf, G_LOCATIONPRINTF),
	GEN_IMPORT_9(Sound, G_SOUND, void, vec3_t*, int, int, const char*, float, float, float, float, int),
	GEN_IMPORT(StopSound, G_STOPSOUND),
	GEN_IMPORT(SoundLength, G_SOUNDLENGTH),
	GEN_IMPORT(SoundAmplitudes, G_SOUNDAMPLITUDES),
	GEN_IMPORT(S_IsSoundPlaying, G_S_ISSOUNDPLAYING),
	GEN_IMPORT(CalcCRC, G_CALCCRC),
	nullptr,	// DebugLines
	nullptr,	// numDebugLines
	nullptr,	// DebugStrings
	nullptr,	// numDebugStrings
	GEN_IMPORT(LocateGameData, G_LOCATE_GAME_DATA),
	GEN_IMPORT(SetFarPlane, G_SETFARPLANE),
	GEN_IMPORT(SetSkyPortal, G_SETSKYPORTAL),
	GEN_IMPORT(Popmenu, G_POPMENU),
	GEN_IMPORT(Showmenu, G_SHOWMENU),
	GEN_IMPORT(Hidemenu, G_HIDEMENU),
	GEN_IMPORT(Pushmenu, G_PUSHMENU),
	GEN_IMPORT(HideMouseCursor, G_HIDEMOUSECURSOR),
	GEN_IMPORT(ShowMouseCursor, G_SHOWMOUSECURSOR),
	GEN_IMPORT(MapTime, G_MAPTIME),
	GEN_IMPORT(LoadResource, G_LOADRESOURCE),
	GEN_IMPORT(ClearResource, G_CLEARRESOURCE),
	GEN_IMPORT(Key_StringToKeynum, G_KEY_STRINGTOKEYNUM),
	GEN_IMPORT(Key_KeynumToBindString, G_KEY_KEYNUMTOBINDSTRING),
	GEN_IMPORT(Key_GetKeysForCommand, G_KEY_GETKEYSFORCOMMAND),
	GEN_IMPORT(ArchiveLevel, G_ARCHIVELEVEL),
	GEN_IMPORT(AddSvsTimeFixup, G_ADDSVSTIMEFIXUP),
	GEN_IMPORT(HudDrawShader, G_HUDDRAWSHADER),
	GEN_IMPORT(HudDrawAlign, G_HUDDRAWALIGN),
	GEN_IMPORT(HudDrawRect, G_HUDDRAWRECT),
	GEN_IMPORT(HudDrawVirtualSize, G_HUDDRAWVIRTUALSIZE),
	GEN_IMPORT(HudDrawColor, G_HUDDRAWCOLOR),
	GEN_IMPORT_2(HudDrawAlpha, G_HUDDRAWALPHA, void, int, float),
	GEN_IMPORT(HudDrawString, G_HUDDRAWSTRING),
	GEN_IMPORT(HudDrawFont, G_HUDDRAWFONT),
	GEN_IMPORT(SanitizeName, G_SANITIZENAME),
	nullptr,	//fsDebug
};


// these are "pre" hooks for storing some data for polyfills.
// we need these to be called BEFORE plugins' prehooks get called so they have to be done in the qmm_export table

// track userinfo for our G_GET_ENTITY_TOKEN syscall
static std::vector<std::string> s_entity_tokens;
static void MOHAA_SpawnEntities(char* entstring, int levelTime) {
	s_entity_tokens = util_parse_tokens(entstring);
	vmMain(GAME_SPAWN_ENTITIES, entstring, levelTime);
}


// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
	GAME_API_VERSION,	// apiversion
	GEN_EXPORT(Init, GAME_INIT),
	GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
	GEN_EXPORT(Cleanup, GAME_CLEANUP),
	GEN_EXPORT(Precache, GAME_PRECACHE),
	GEN_EXPORT(SetMap, GAME_SETMAP),
	GEN_EXPORT(Restart, GAME_RESTART),
	GEN_EXPORT(SetTime, GAME_SETTIME),
	MOHAA_SpawnEntities, // GEN_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES),
	GEN_EXPORT(ClientConnect, GAME_CLIENT_CONNECT),
	GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
	GEN_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED),
	GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
	GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
	GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
	GEN_EXPORT(BotBegin, GAME_BOTBEGIN),
	GEN_EXPORT(BotThink, GAME_BOTTHINK),
	GEN_EXPORT(PrepFrame, GAME_PREP_FRAME),
	GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
	GEN_EXPORT(ServerSpawned, GAME_SERVER_SPAWNED),
	GEN_EXPORT(RegisterSounds, GAME_REGISTER_SOUNDS),
	GEN_EXPORT(AllowPaused, GAME_ALLOW_PAUSED),
	GEN_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND),
	GEN_EXPORT(ArchivePersistant, GAME_ARCHIVE_PERSISTANT),
	GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
	GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
	GEN_EXPORT(LevelArchiveValid, GAME_LEVEL_ARCHIVE_VALID),
	GEN_EXPORT(ArchiveInteger, GAME_ARCHIVE_INTEGER),
	GEN_EXPORT(ArchiveFloat, GAME_ARCHIVE_FLOAT),
	GEN_EXPORT(ArchiveString, GAME_ARCHIVE_STRING),
	GEN_EXPORT(ArchiveSvsTime, GAME_ARCHIVE_SVSTIME),
	GEN_EXPORT(TIKI_Orientation, GAME_TIKI_ORIENTATION), // todo, change types to actually match float, but also need to return an intptr_t instead of orientation_t
	/* DebugCircle */ +[](float* arg0, float arg1, float arg2, float arg3, float arg4, float arg5, qboolean arg6) -> void { vmMain(GAME_DEBUG_CIRCLE, arg0, arg1, arg2, arg3, arg4, arg5, arg6); },
	GEN_EXPORT(SetFrameNumber, GAME_SET_FRAME_NUMBER),
	GEN_EXPORT(SoundCallback, GAME_SOUND_CALLBACK),

	// the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
	nullptr,			// profStruct
	nullptr,			// gentities
	sizeof(gentity_t),	// gentitySize
	0,					// num_entities
	0,					// max_entities
	nullptr,			// errorMessage
};


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t MOHAA_syscall(intptr_t cmd, ...) {
	QMM_GET_SYSCALL_ARGS();

	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("MOHAA_syscall({} {}) called\n", MOHAA_eng_msg_names(cmd), cmd);

	// store copy of mod's export pointer. this is stored in g_gameinfo.api_info in mod_load(), or set to nullptr in mod_unload()
	orig_export = (game_export_t*)(g_gameinfo.api_info.orig_export);

	// before the engine is called into by the mod, some of the variables in the mod's exports may have changed
	// and these changes need to be available to the engine, so copy those values before entering the engine
	if (orig_export) {
		qmm_export.profStruct = orig_export->profStruct;
		qmm_export.gentities = orig_export->gentities;
		qmm_export.gentitySize = orig_export->gentitySize;
		qmm_export.num_entities = orig_export->num_entities;
		qmm_export.max_entities = orig_export->max_entities;
		qmm_export.errorMessage = orig_export->errorMessage;
	}

	// store return value in case we do some stuff after the function call is over
	intptr_t ret = 0;

	switch (cmd) {
		ROUTE_IMPORT(Printf, G_PRINTF);
		ROUTE_IMPORT(DPrintf, G_DPRINTF);
		ROUTE_IMPORT(DPrintf2, G_DPRINTF2);
		ROUTE_IMPORT(DebugPrintf, G_DEBUGPRINTF);
		ROUTE_IMPORT(Error, G_ERROR);
		ROUTE_IMPORT(Milliseconds, G_MILLISECONDS);
		ROUTE_IMPORT(LV_ConvertString, G_LV_CONVERTSTRING);
		ROUTE_IMPORT(Malloc, G_MALLOC);
		ROUTE_IMPORT(Free, G_FREE);
		ROUTE_IMPORT(Cvar_Get, G_CVAR_GET);
		ROUTE_IMPORT(cvar_set, G_CVAR_SET);
		ROUTE_IMPORT(cvar_set2, G_CVAR_SET2);
		ROUTE_IMPORT(NextCvar, G_NEXTCVAR);
		ROUTE_IMPORT(Argc, G_ARGC);
		ROUTE_IMPORT(Argv, G_ARGV);
		ROUTE_IMPORT(Args, G_ARGS);
		ROUTE_IMPORT(AddCommand, G_ADDCOMMAND);
		ROUTE_IMPORT(FS_ReadFile, G_FS_READFILE);
		ROUTE_IMPORT(FS_FreeFile, G_FS_FREEFILE);
		ROUTE_IMPORT(FS_WriteFile, G_FS_WRITEFILE);
		ROUTE_IMPORT(FS_FOpenFileWrite, G_FS_FOPEN_FILE_WRITE);
		ROUTE_IMPORT(FS_FOpenFileAppend, G_FS_FOPEN_FILE_APPEND);
		ROUTE_IMPORT(FS_PrepFileWrite, G_FS_PREPFILEWRITE);
		ROUTE_IMPORT(FS_Write, G_FS_WRITE);
		ROUTE_IMPORT(FS_Read, G_FS_READ);
		ROUTE_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE);
		ROUTE_IMPORT(FS_Tell, G_FS_TELL);
		ROUTE_IMPORT(FS_Seek, G_FS_SEEK);
		ROUTE_IMPORT(FS_Flush, G_FS_FLUSH);
		ROUTE_IMPORT(FS_FileNewer, G_FS_FILENEWER);
		ROUTE_IMPORT(FS_CanonicalFilename, G_FS_CANONICALFILENAME);
		ROUTE_IMPORT(FS_ListFiles, G_FS_LISTFILES);
		ROUTE_IMPORT(FS_FreeFileList, G_FS_FREEFILELIST);
		ROUTE_IMPORT(GetArchiveFileName, G_GETARCHIVEFILENAME);
		ROUTE_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND_EX);
		ROUTE_IMPORT(DebugGraph, G_DEBUGGRAPH);
		ROUTE_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND);
		ROUTE_IMPORT(DropClient, G_DROPCLIENT);
		ROUTE_IMPORT(MSG_WriteBits, G_MSG_WRITEBITS);
		ROUTE_IMPORT(MSG_WriteChar, G_MSG_WRITECHAR);
		ROUTE_IMPORT(MSG_WriteByte, G_MSG_WRITEBYTE);
		ROUTE_IMPORT(MSG_WriteSVC, G_MSG_WRITESVC);
		ROUTE_IMPORT(MSG_WriteShort, G_MSG_WRITESHORT);
		ROUTE_IMPORT(MSG_WriteLong, G_MSG_WRITELONG);
		ROUTE_IMPORT(MSG_WriteFloat, G_MSG_WRITEFLOAT);
		ROUTE_IMPORT(MSG_WriteString, G_MSG_WRITESTRING);
		ROUTE_IMPORT(MSG_WriteAngle8, G_MSG_WRITEANGLE8);
		ROUTE_IMPORT(MSG_WriteAngle16, G_MSG_WRITEANGLE16);
		ROUTE_IMPORT(MSG_WriteCoord, G_MSG_WRITECOORD);
		ROUTE_IMPORT(MSG_WriteDir, G_MSG_WRITEDIR);
		ROUTE_IMPORT(MSG_StartCGM, G_MSG_STARTCGM);
		ROUTE_IMPORT(MSG_EndCGM, G_MSG_ENDCGM);
		ROUTE_IMPORT(MSG_SetClient, G_MSG_SETCLIENT);
		ROUTE_IMPORT(SetBroadcastVisible, G_SETBROADCASTVISIBLE);
		ROUTE_IMPORT(SetBroadcastHearable, G_SETBROADCASTHEARABLE);
		ROUTE_IMPORT(SetBroadcastAll, G_SETBROADCASTALL);
		ROUTE_IMPORT(setConfigstring, G_SET_CONFIGSTRING);
		ROUTE_IMPORT(getConfigstring, G_GET_CONFIGSTRING);
		ROUTE_IMPORT(SetUserinfo, G_SET_USERINFO);
		ROUTE_IMPORT(GetUserinfo, G_GET_USERINFO);
		ROUTE_IMPORT(SetBrushModel, G_SETBRUSHMODEL);
		ROUTE_IMPORT(ModelBoundsFromName, G_MODELBOUNDSFROMNAME);
		ROUTE_IMPORT(SightTraceEntity, G_SIGHTTRACEENTITY);
		ROUTE_IMPORT(SightTrace, G_SIGHTTRACE);
		ROUTE_IMPORT(trace, G_TRACE);
		ROUTE_IMPORT(GetShader, G_GETSHADER);
		ROUTE_IMPORT(pointcontents, G_POINTCONTENTS);
		ROUTE_IMPORT(PointBrushnum, G_POINTBRUSHNUM);
		ROUTE_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE);
		ROUTE_IMPORT(AreaForPoint, G_AREAFORPOINT);
		ROUTE_IMPORT(AreasConnected, G_AREASCONNECTED);
		ROUTE_IMPORT(InPVS, G_INPVS);
		ROUTE_IMPORT(linkentity, G_LINKENTITY);
		ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
		ROUTE_IMPORT(AreaEntities, G_AREAENTITIES);
		ROUTE_IMPORT(ClipToEntity, G_CLIPTOENTITY);
		ROUTE_IMPORT(imageindex, G_IMAGEINDEX);
		ROUTE_IMPORT(itemindex, G_ITEMINDEX);
		ROUTE_IMPORT(soundindex, G_SOUNDINDEX);
		ROUTE_IMPORT(TIKI_RegisterModel, G_TIKI_REGISTERMODEL);
		ROUTE_IMPORT(modeltiki, G_MODELTIKI);
		ROUTE_IMPORT(modeltikianim, G_MODELTIKIANIM);
		ROUTE_IMPORT(SetLightStyle, G_SETLIGHTSTYLE);
		ROUTE_IMPORT(GameDir, G_GAMEDIR);
		ROUTE_IMPORT(setmodel, G_SETMODEL);
		ROUTE_IMPORT(clearmodel, G_CLEARMODEL);
		ROUTE_IMPORT(TIKI_NumAnims, G_TIKI_NUMANIMS);
		ROUTE_IMPORT(TIKI_NumSurfaces, G_TIKI_NUMSURFACES);
		ROUTE_IMPORT(TIKI_NumTags, G_TIKI_NUMTAGS);
		ROUTE_IMPORT(TIKI_CalculateBounds, G_TIKI_CALCULATEBOUNDS);
		ROUTE_IMPORT(TIKI_GetSkeletor, G_TIKI_GETSKELETOR);
		ROUTE_IMPORT(Anim_NameForNum, G_ANIM_NAMEFORNUM);
		ROUTE_IMPORT(Anim_NumForName, G_ANIM_NUMFORNAME);
		ROUTE_IMPORT(Anim_Random, G_ANIM_RANDOM);
		ROUTE_IMPORT(Anim_NumFrames, G_ANIM_NUMFRAMES);
		ROUTE_IMPORT(Anim_Time, G_ANIM_TIME);
		ROUTE_IMPORT(Anim_Frametime, G_ANIM_FRAMETIME);
		ROUTE_IMPORT(Anim_CrossTime, G_ANIM_CROSSTIME);
		ROUTE_IMPORT(Anim_Delta, G_ANIM_DELTA);
		ROUTE_IMPORT(Anim_HasDelta, G_ANIM_HASDELTA);
		ROUTE_IMPORT(Anim_DeltaOverTime, G_ANIM_DELTAOVERTIME);
		ROUTE_IMPORT(Anim_Flags, G_ANIM_FLAGS);
		ROUTE_IMPORT(Anim_FlagsSkel, G_ANIM_FLAGSSKEL);
		ROUTE_IMPORT(Anim_HasCommands, G_ANIM_HASCOMMANDS);
		ROUTE_IMPORT(NumHeadModels, G_NUMHEADMODELS);
		ROUTE_IMPORT(GetHeadModel, G_GETHEADMODEL);
		ROUTE_IMPORT(NumHeadSkins, G_NUMHEADSKINS);
		ROUTE_IMPORT(GetHeadSkin, G_GETHEADSKIN);
		ROUTE_IMPORT(Frame_Commands, G_FRAME_COMMANDS);
		ROUTE_IMPORT(Surface_NameToNum, G_SURFACE_NAMETONUM);
		ROUTE_IMPORT(Surface_NumToName, G_SURFACE_NUMTONAME);
		ROUTE_IMPORT(Tag_NumForName, G_TAG_NUMFORNAME);
		ROUTE_IMPORT(Tag_NameForNum, G_TAG_NAMEFORNUM);
		ROUTE_IMPORT(TIKI_OrientationInternal, G_TIKI_ORIENTATIONINTERNAL);
		ROUTE_IMPORT(TIKI_TransformInternal, G_TIKI_TRANSFORMINTERNAL);
		ROUTE_IMPORT(TIKI_IsOnGroundInternal, G_TIKI_ISONGROUNDINTERNAL);
		ROUTE_IMPORT(TIKI_SetPoseInternal, G_TIKI_SETPOSEINTERNAL);
		ROUTE_IMPORT(CM_GetHitLocationInfo, G_CM_GETHITLOCATIONINFO);
		ROUTE_IMPORT(CM_GetHitLocationInfoSecondary, G_CM_GETHITLOCATIONINFOSECONDARY);
		ROUTE_IMPORT(Alias_Add, G_ALIAS_ADD);
		ROUTE_IMPORT(Alias_FindRandom, G_ALIAS_FINDRANDOM);
		ROUTE_IMPORT(Alias_Dump, G_ALIAS_DUMP);
		ROUTE_IMPORT(Alias_Clear, G_ALIAS_CLEAR);
		ROUTE_IMPORT(Alias_UpdateDialog, G_ALIAS_UPDATEDIALOG);
		ROUTE_IMPORT(TIKI_NameForNum, G_TIKI_NAMEFORNUM);
		ROUTE_IMPORT(GlobalAlias_Add, G_GLOBALALIAS_ADD);
		ROUTE_IMPORT(GlobalAlias_FindRandom, G_GLOBALALIAS_FINDRANDOM);
		ROUTE_IMPORT(GlobalAlias_Dump, G_GLOBALALIAS_DUMP);
		ROUTE_IMPORT(GlobalAlias_Clear, G_GLOBALALIAS_CLEAR);
		ROUTE_IMPORT(centerprintf, G_CENTERPRINTF);
		ROUTE_IMPORT(locationprintf, G_LOCATIONPRINTF);
		ROUTE_IMPORT(Sound, G_SOUND);
		ROUTE_IMPORT(StopSound, G_STOPSOUND);
		ROUTE_IMPORT(SoundLength, G_SOUNDLENGTH);
		ROUTE_IMPORT(SoundAmplitudes, G_SOUNDAMPLITUDES);
		ROUTE_IMPORT(S_IsSoundPlaying, G_S_ISSOUNDPLAYING);
		ROUTE_IMPORT(CalcCRC, G_CALCCRC);
		ROUTE_IMPORT(LocateGameData, G_LOCATE_GAME_DATA);
		ROUTE_IMPORT(SetFarPlane, G_SETFARPLANE);
		ROUTE_IMPORT(SetSkyPortal, G_SETSKYPORTAL);
		ROUTE_IMPORT(Popmenu, G_POPMENU);
		ROUTE_IMPORT(Showmenu, G_SHOWMENU);
		ROUTE_IMPORT(Hidemenu, G_HIDEMENU);
		ROUTE_IMPORT(Pushmenu, G_PUSHMENU);
		ROUTE_IMPORT(HideMouseCursor, G_HIDEMOUSECURSOR);
		ROUTE_IMPORT(ShowMouseCursor, G_SHOWMOUSECURSOR);
		ROUTE_IMPORT(MapTime, G_MAPTIME);
		ROUTE_IMPORT(LoadResource, G_LOADRESOURCE);
		ROUTE_IMPORT(ClearResource, G_CLEARRESOURCE);
		ROUTE_IMPORT(Key_StringToKeynum, G_KEY_STRINGTOKEYNUM);
		ROUTE_IMPORT(Key_KeynumToBindString, G_KEY_KEYNUMTOBINDSTRING);
		ROUTE_IMPORT(Key_GetKeysForCommand, G_KEY_GETKEYSFORCOMMAND);
		ROUTE_IMPORT(ArchiveLevel, G_ARCHIVELEVEL);
		ROUTE_IMPORT(AddSvsTimeFixup, G_ADDSVSTIMEFIXUP);
		ROUTE_IMPORT(HudDrawShader, G_HUDDRAWSHADER);
		ROUTE_IMPORT(HudDrawAlign, G_HUDDRAWALIGN);
		ROUTE_IMPORT(HudDrawRect, G_HUDDRAWRECT);
		ROUTE_IMPORT(HudDrawVirtualSize, G_HUDDRAWVIRTUALSIZE);
		ROUTE_IMPORT(HudDrawColor, G_HUDDRAWCOLOR);
		ROUTE_IMPORT(HudDrawAlpha, G_HUDDRAWALPHA);
		ROUTE_IMPORT(HudDrawString, G_HUDDRAWSTRING);
		ROUTE_IMPORT(HudDrawFont, G_HUDDRAWFONT);
		ROUTE_IMPORT(SanitizeName, G_SANITIZENAME);

		// handle cmds for variables, this is how a plugin would get these values if needed
		ROUTE_IMPORT_VAR(DebugLines, GVP_DEBUGLINES);
		ROUTE_IMPORT_VAR(numDebugLines, GVP_NUMDEBUGLINES);
		ROUTE_IMPORT_VAR(DebugStrings, GVP_DEBUGSTRINGS);
		ROUTE_IMPORT_VAR(numDebugStrings, GVP_NUMDEBUGSTRINGS);

		// handle special cmds which QMM uses but MOHAA doesn't have an analogue for
		case G_CVAR_REGISTER: {
			// mohaa: cvar_t* (*Cvar_Get)(const char* varName, const char* varValue, int varFlags)
			// q3a: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
			// qmm always passes NULL for vmCvar so don't worry about it
			const char* varName = (const char*)(args[1]);
			const char* defaultValue = (const char*)(args[2]);
			int flags = args[3];
			(void)orig_import.Cvar_Get(varName, defaultValue, flags);
			break;
		}
		case G_CVAR_VARIABLE_STRING_BUFFER: {
			// mohaa: cvar_t *(*Cvar_Get)(const char *varName, const char *varValue, int varFlags)
			// q3a: void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
			const char* varName = (const char*)(args[0]);
			char* buffer = (char*)(args[1]);
			int bufsize = args[2];
			*buffer = '\0';
			cvar_t* cvar = orig_import.Cvar_Get(varName, "", 0);
			if (cvar)
				strncpy(buffer, cvar->string, bufsize);
			buffer[bufsize - 1] = '\0';
			break;
		}
		case G_CVAR_VARIABLE_INTEGER_VALUE: {
			// mohaa: cvar_t *(*Cvar_Get)(const char *varName, const char *varValue, int varFlags)
			// q3a: int trap_Cvar_VariableIntegerValue(const char* var_name)
			const char* varName = (const char*)(args[0]);
			cvar_t* cvar = orig_import.Cvar_Get(varName, "", 0);
			if (cvar)
				ret = cvar->integer;
			break;
		}
		case G_SEND_CONSOLE_COMMAND: {
			// MOHAA: void (*SendConsoleCommand)(const char *text);
			// qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
			const char* text = (const char*)(args[1]);
			orig_import.SendConsoleCommand(text);
			break;
		}
		// MOHAA is only missing FS_FOPEN_FILE for reading
		// if mode == FS_WRITE, then just pass to FS_FOpenFileWrite.
		// if mode == FS_APPEND(_SYNC), then just pass to FS_FOpenFileAppend.
		// if mode == FS_READ, then return -1 since we can't do anything :(
		case G_FS_FOPEN_FILE: {
			// MOHAA: fileHandle_t (*FS_FOpenFileAppend)(const char *fileName);
			// MOHAA: fileHandle_t (*FS_FOpenFileWrite)(const char *fileName);
			// q3a: int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
			const char* qpath = (const char*)args[0];
			fileHandle_t* f = (fileHandle_t*)args[1];
			fsMode_t mode = (fsMode_t)args[2];
			if (mode == FS_READ) {
				ret = -1;
				break;
			}
			else if (mode == FS_WRITE) {
				*f = orig_import.FS_FOpenFileWrite(qpath);
				if (!*f) {
					ret = -1;
					break;
				}
				ret = 0;
			}
			// mode == FS_APPEND(_SYNC)
			else {
				*f = orig_import.FS_FOpenFileAppend(qpath);
				if (!*f) {
					ret = -1;
					break;
				}
				orig_import.FS_Seek(*f, 0, FS_SEEK_END);
				ret = orig_import.FS_Tell(*f);
			}
			break;
		}
		// since fileHandle_t-based functions don't exist in MOHAA when trying to READ a file, we can provide some help
		// to plugins by offering the FILE* functions available to Q2R and QUAKE2, although the cmd name ends with _QMM
		// (since MOHAA provides G_FS_READ, G_FS_WRITE, and G_FS_FCLOSE_FILE)
		case G_FS_FOPEN_FILE_QMM: {
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
		case G_FS_READ_QMM: {
			// void trap_FS_Read(void* buffer, int len, fileHandle_t f);
			void* buffer = (void*)args[0];
			intptr_t len = args[1];
			fileHandle_t f = (fileHandle_t)args[2];
			(void)fread(buffer, len, 1, (FILE*)f);
			break;
		}
		case G_FS_WRITE_QMM: {
			// void trap_FS_Write(const void* buffer, int len, fileHandle_t f);
			void* buffer = (void*)args[0];
			intptr_t len = args[1];
			fileHandle_t f = (fileHandle_t)args[2];
			fwrite(buffer, len, 1, (FILE*)f);
			break;
		}
		case G_FS_FCLOSE_FILE_QMM: {
			// void trap_FS_FCloseFile(fileHandle_t f);
			fileHandle_t f = (fileHandle_t)args[0];
			fclose((FILE*)f);
			break;
		}
		case G_DROP_CLIENT: {
			// void trap_DropClient(int clientNum, const char *reason);
			intptr_t clientnum = args[0];
			orig_import.SendConsoleCommand(fmt::format("kick {}\n", clientnum).c_str());
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

			strncpy(buffer, s_entity_tokens[token++].c_str(), bufferSize);
			buffer[bufferSize - 1] = '\0';
			ret = qtrue;
			break;
		}

		default:
			break;
	};

	// do anything that needs to be done after function call here

	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("MOHAA_syscall({} {}) returning {}\n", MOHAA_eng_msg_names(cmd), cmd, ret);

	return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t MOHAA_vmMain(intptr_t cmd, ...) {
	QMM_GET_VMMAIN_ARGS();

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("MOHAA_vmMain({} {}) called\n", MOHAA_mod_msg_names(cmd), cmd);

	// store copy of mod's export pointer. this is stored in g_gameinfo.api_info in mod_load(), or set to nullptr in mod_unload()
	orig_export = (game_export_t*)(g_gameinfo.api_info.orig_export);
	if (!orig_export)
		return 0;

	// store return value since we do some stuff after the function call is over
	intptr_t ret = 0;

	switch (cmd) {
		ROUTE_EXPORT(Init, GAME_INIT);
		ROUTE_EXPORT(Shutdown, GAME_SHUTDOWN);
		ROUTE_EXPORT(Cleanup, GAME_CLEANUP);
		ROUTE_EXPORT(Precache, GAME_PRECACHE);
		ROUTE_EXPORT(SetMap, GAME_SETMAP);
		ROUTE_EXPORT(Restart, GAME_RESTART);
		ROUTE_EXPORT(SetTime, GAME_SETTIME);
		ROUTE_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES);
		ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
		ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
		ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
		ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
		ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
		ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
		ROUTE_EXPORT(BotBegin, GAME_BOTBEGIN);
		ROUTE_EXPORT(BotThink, GAME_BOTTHINK);
		ROUTE_EXPORT(PrepFrame, GAME_PREP_FRAME);
		ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
		ROUTE_EXPORT(ServerSpawned, GAME_SERVER_SPAWNED);
		ROUTE_EXPORT(RegisterSounds, GAME_REGISTER_SOUNDS);
		ROUTE_EXPORT(AllowPaused, GAME_ALLOW_PAUSED);
		ROUTE_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND);
		ROUTE_EXPORT(ArchivePersistant, GAME_ARCHIVE_PERSISTANT);
		ROUTE_EXPORT(WriteLevel, GAME_WRITE_LEVEL);
		ROUTE_EXPORT(ReadLevel, GAME_READ_LEVEL);
		ROUTE_EXPORT(LevelArchiveValid, GAME_LEVEL_ARCHIVE_VALID);
		ROUTE_EXPORT(ArchiveInteger, GAME_ARCHIVE_INTEGER);
		ROUTE_EXPORT(ArchiveFloat, GAME_ARCHIVE_FLOAT);
		ROUTE_EXPORT(ArchiveString, GAME_ARCHIVE_STRING);
		ROUTE_EXPORT(ArchiveSvsTime, GAME_ARCHIVE_SVSTIME);
		ROUTE_EXPORT(TIKI_Orientation, GAME_TIKI_ORIENTATION);
		ROUTE_EXPORT(DebugCircle, GAME_DEBUG_CIRCLE);
		ROUTE_EXPORT(SetFrameNumber, GAME_SET_FRAME_NUMBER);
		ROUTE_EXPORT(SoundCallback, GAME_SOUND_CALLBACK);

		// handle cmds for variables, this is how a plugin would get these values if needed
		ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
		ROUTE_EXPORT_VAR(profStruct, GAMEVP_PROFSTRUCT);
		ROUTE_EXPORT_VAR(gentities, GAMEVP_GENTITIES);
		ROUTE_EXPORT_VAR(gentitySize, GAMEV_GENTITYSIZE);
		ROUTE_EXPORT_VAR(num_entities, GAMEV_NUM_ENTITIES);
		ROUTE_EXPORT_VAR(max_entities, GAMEV_MAX_ENTITIES);
		ROUTE_EXPORT_VAR(errorMessage, GAMEVP_ERRORMESSAGE);

		default:
			break;
	};

	// after the mod is called into by the engine, some of the variables in the mod's exports may have changed (num_entities and errorMessage in particular)
	// and these changes need to be available to the engine, so copy those values again now before returning from the mod
	qmm_export.profStruct = orig_export->profStruct;
	qmm_export.gentities = orig_export->gentities;
	qmm_export.gentitySize = orig_export->gentitySize;
	qmm_export.num_entities = orig_export->num_entities;
	qmm_export.max_entities = orig_export->max_entities;
	qmm_export.errorMessage = orig_export->errorMessage;

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("MOHAA_vmMain({} {}) returning {}\n", MOHAA_mod_msg_names(cmd), cmd, ret);

	return ret;
}


void* MOHAA_GetGameAPI(void* import) {
	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("MOHAA_GetGameAPI({}) called\n", import);

	// original import struct from engine
	// the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
	game_import_t* gi = (game_import_t*)import;
	orig_import = *gi;

	// fill in variables of our hooked import struct to pass to the mod
	qmm_import.DebugLines = orig_import.DebugLines;
	qmm_import.numDebugLines = orig_import.numDebugLines;
	qmm_import.DebugStrings = orig_import.DebugStrings;
	qmm_import.numDebugStrings = orig_import.numDebugStrings;
	qmm_import.fsDebug = orig_import.fsDebug;

	// this gets passed to the mod's GetGameAPI() function in mod.cpp:mod_load()
	g_gameinfo.api_info.qmm_import = &qmm_import;

	// this isn't used anywhere except returning from this function, but store it in g_gameinfo.api_info for consistency
	g_gameinfo.api_info.qmm_export = &qmm_export;

	// pointer to wrapper vmMain function that calls actual mod func from orig_export
	// this gets assigned to g_mod->pfnvmMain in mod.cpp:mod_load()
	g_gameinfo.api_info.orig_vmmain = MOHAA_vmMain;

	// pointer to wrapper syscall function that calls actual engine func from orig_import
	g_gameinfo.pfnsyscall = MOHAA_syscall;

	LOG(QMM_LOG_TRACE, "QMM") << fmt::format("MOHAA_GetGameAPI({}) returning {}\n", import, (void*)&qmm_export);

	// struct full of export lambdas to QMM's vmMain
	// this gets returned to the game engine, but we haven't loaded the mod yet.
	// the only thing in this struct the engine uses before calling Init is the apiversion
	return &qmm_export;
}


const char* MOHAA_eng_msg_names(intptr_t cmd) {
	switch (cmd) {
		GEN_CASE(G_PRINTF);
		GEN_CASE(G_DPRINTF);
		GEN_CASE(G_DPRINTF2);
		GEN_CASE(G_DEBUGPRINTF);
		GEN_CASE(G_ERROR);
		GEN_CASE(G_MILLISECONDS);
		GEN_CASE(G_LV_CONVERTSTRING);
		GEN_CASE(G_MALLOC);
		GEN_CASE(G_FREE);
		GEN_CASE(G_CVAR_GET);
		GEN_CASE(G_CVAR_SET);
		GEN_CASE(G_CVAR_SET2);
		GEN_CASE(G_NEXTCVAR);
		GEN_CASE(G_ARGC);
		GEN_CASE(G_ARGV);
		GEN_CASE(G_ARGS);
		GEN_CASE(G_ADDCOMMAND);
		GEN_CASE(G_FS_READFILE);
		GEN_CASE(G_FS_FREEFILE);
		GEN_CASE(G_FS_WRITEFILE);
		GEN_CASE(G_FS_FOPEN_FILE_WRITE);
		GEN_CASE(G_FS_FOPEN_FILE_APPEND);
		GEN_CASE(G_FS_PREPFILEWRITE);
		GEN_CASE(G_FS_WRITE);
		GEN_CASE(G_FS_READ);
		GEN_CASE(G_FS_FCLOSE_FILE);
		GEN_CASE(G_FS_TELL);
		GEN_CASE(G_FS_SEEK);
		GEN_CASE(G_FS_FLUSH);
		GEN_CASE(G_FS_FILENEWER);
		GEN_CASE(G_FS_CANONICALFILENAME);
		GEN_CASE(G_FS_LISTFILES);
		GEN_CASE(G_FS_FREEFILELIST);
		GEN_CASE(G_GETARCHIVEFILENAME);
		GEN_CASE(G_SEND_CONSOLE_COMMAND_EX);
		GEN_CASE(G_DEBUGGRAPH);
		GEN_CASE(G_SEND_SERVER_COMMAND);
		GEN_CASE(G_DROPCLIENT);
		GEN_CASE(G_MSG_WRITEBITS);
		GEN_CASE(G_MSG_WRITECHAR);
		GEN_CASE(G_MSG_WRITEBYTE);
		GEN_CASE(G_MSG_WRITESVC);
		GEN_CASE(G_MSG_WRITESHORT);
		GEN_CASE(G_MSG_WRITELONG);
		GEN_CASE(G_MSG_WRITEFLOAT);
		GEN_CASE(G_MSG_WRITESTRING);
		GEN_CASE(G_MSG_WRITEANGLE8);
		GEN_CASE(G_MSG_WRITEANGLE16);
		GEN_CASE(G_MSG_WRITECOORD);
		GEN_CASE(G_MSG_WRITEDIR);
		GEN_CASE(G_MSG_STARTCGM);
		GEN_CASE(G_MSG_ENDCGM);
		GEN_CASE(G_MSG_SETCLIENT);
		GEN_CASE(G_SETBROADCASTVISIBLE);
		GEN_CASE(G_SETBROADCASTHEARABLE);
		GEN_CASE(G_SETBROADCASTALL);
		GEN_CASE(G_SET_CONFIGSTRING);
		GEN_CASE(G_GET_CONFIGSTRING);
		GEN_CASE(G_SET_USERINFO);
		GEN_CASE(G_GET_USERINFO);
		GEN_CASE(G_SETBRUSHMODEL);
		GEN_CASE(G_TRACE);
		GEN_CASE(G_GETSHADER);
		GEN_CASE(G_POINTCONTENTS);
		GEN_CASE(G_POINTBRUSHNUM);
		GEN_CASE(G_ADJUSTAREAPORTALSTATE);
		GEN_CASE(G_AREAFORPOINT);
		GEN_CASE(G_AREASCONNECTED);
		GEN_CASE(G_INPVS);
		GEN_CASE(G_LINKENTITY);
		GEN_CASE(G_UNLINKENTITY);
		GEN_CASE(G_AREAENTITIES);
		GEN_CASE(G_CLIPTOENTITY);
		GEN_CASE(G_IMAGEINDEX);
		GEN_CASE(G_ITEMINDEX);
		GEN_CASE(G_SOUNDINDEX);
		GEN_CASE(G_TIKI_REGISTERMODEL);
		GEN_CASE(G_MODELTIKI);
		GEN_CASE(G_MODELTIKIANIM);
		GEN_CASE(G_SETLIGHTSTYLE);
		GEN_CASE(G_GAMEDIR);
		GEN_CASE(G_SETMODEL);
		GEN_CASE(G_CLEARMODEL);
		GEN_CASE(G_TIKI_NUMANIMS);
		GEN_CASE(G_TIKI_NUMSURFACES);
		GEN_CASE(G_TIKI_NUMTAGS);
		GEN_CASE(G_TIKI_CALCULATEBOUNDS);
		GEN_CASE(G_TIKI_GETSKELETOR);
		GEN_CASE(G_ANIM_NAMEFORNUM);
		GEN_CASE(G_ANIM_NUMFORNAME);
		GEN_CASE(G_ANIM_RANDOM);
		GEN_CASE(G_ANIM_NUMFRAMES);
		GEN_CASE(G_ANIM_TIME);
		GEN_CASE(G_ANIM_FRAMETIME);
		GEN_CASE(G_ANIM_CROSSTIME);
		GEN_CASE(G_ANIM_DELTA);
		GEN_CASE(G_ANIM_HASDELTA);
		GEN_CASE(G_ANIM_DELTAOVERTIME);
		GEN_CASE(G_ANIM_FLAGS);
		GEN_CASE(G_ANIM_FLAGSSKEL);
		GEN_CASE(G_ANIM_HASCOMMANDS);
		GEN_CASE(G_NUMHEADMODELS);
		GEN_CASE(G_GETHEADMODEL);
		GEN_CASE(G_NUMHEADSKINS);
		GEN_CASE(G_GETHEADSKIN);
		GEN_CASE(G_FRAME_COMMANDS);
		GEN_CASE(G_SURFACE_NAMETONUM);
		GEN_CASE(G_SURFACE_NUMTONAME);
		GEN_CASE(G_TAG_NUMFORNAME);
		GEN_CASE(G_TAG_NAMEFORNUM);
		GEN_CASE(G_TIKI_ORIENTATIONINTERNAL);
		GEN_CASE(G_TIKI_TRANSFORMINTERNAL);
		GEN_CASE(G_TIKI_ISONGROUNDINTERNAL);
		GEN_CASE(G_TIKI_SETPOSEINTERNAL);
		GEN_CASE(G_CM_GETHITLOCATIONINFO);
		GEN_CASE(G_CM_GETHITLOCATIONINFOSECONDARY);
		GEN_CASE(G_ALIAS_ADD);
		GEN_CASE(G_ALIAS_FINDRANDOM);
		GEN_CASE(G_ALIAS_DUMP);
		GEN_CASE(G_ALIAS_CLEAR);
		GEN_CASE(G_ALIAS_UPDATEDIALOG);
		GEN_CASE(G_TIKI_NAMEFORNUM);
		GEN_CASE(G_GLOBALALIAS_ADD);
		GEN_CASE(G_GLOBALALIAS_FINDRANDOM);
		GEN_CASE(G_GLOBALALIAS_DUMP);
		GEN_CASE(G_GLOBALALIAS_CLEAR);
		GEN_CASE(G_CENTERPRINTF);
		GEN_CASE(G_LOCATIONPRINTF);
		GEN_CASE(G_SOUND);
		GEN_CASE(G_STOPSOUND);
		GEN_CASE(G_SOUNDLENGTH);
		GEN_CASE(G_SOUNDAMPLITUDES);
		GEN_CASE(G_CALCCRC);
		GEN_CASE(GVP_DEBUGLINES);
		GEN_CASE(GVP_NUMDEBUGLINES);
		GEN_CASE(GVP_DEBUGSTRINGS);
		GEN_CASE(GVP_NUMDEBUGSTRINGS);
		GEN_CASE(G_LOCATE_GAME_DATA);
		GEN_CASE(G_SETFARPLANE);
		GEN_CASE(G_SETSKYPORTAL);
		GEN_CASE(G_POPMENU);
		GEN_CASE(G_SHOWMENU);
		GEN_CASE(G_HIDEMENU);
		GEN_CASE(G_PUSHMENU);
		GEN_CASE(G_HIDEMOUSECURSOR);
		GEN_CASE(G_SHOWMOUSECURSOR);
		GEN_CASE(G_MAPTIME);
		GEN_CASE(G_LOADRESOURCE);
		GEN_CASE(G_CLEARRESOURCE);
		GEN_CASE(G_KEY_STRINGTOKEYNUM);
		GEN_CASE(G_KEY_KEYNUMTOBINDSTRING);
		GEN_CASE(G_KEY_GETKEYSFORCOMMAND);
		GEN_CASE(G_ARCHIVELEVEL);
		GEN_CASE(G_ADDSVSTIMEFIXUP);
		GEN_CASE(G_HUDDRAWSHADER);
		GEN_CASE(G_HUDDRAWALIGN);
		GEN_CASE(G_HUDDRAWRECT);
		GEN_CASE(G_HUDDRAWVIRTUALSIZE);
		GEN_CASE(G_HUDDRAWCOLOR);
		GEN_CASE(G_HUDDRAWALPHA);
		GEN_CASE(G_HUDDRAWSTRING);
		GEN_CASE(G_HUDDRAWFONT);
		GEN_CASE(G_SANITIZENAME);
		GEN_CASE(GVP_FSDEBUG);

		// polyfills
		GEN_CASE(G_CVAR_REGISTER);
		GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
		GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
		GEN_CASE(G_SEND_CONSOLE_COMMAND);

		GEN_CASE(G_FS_FOPEN_FILE);

		GEN_CASE(G_FS_FOPEN_FILE_QMM);
		GEN_CASE(G_FS_READ_QMM);
		GEN_CASE(G_FS_WRITE_QMM);
		GEN_CASE(G_FS_FCLOSE_FILE_QMM);

		GEN_CASE(G_DROP_CLIENT);
		GEN_CASE(G_GET_ENTITY_TOKEN);

		default:
			return "unknown";
	}
}


const char* MOHAA_mod_msg_names(intptr_t cmd) {
	switch (cmd) {
		GEN_CASE(GAMEV_APIVERSION);
		GEN_CASE(GAME_INIT);
		GEN_CASE(GAME_SHUTDOWN);
		GEN_CASE(GAME_CLEANUP);
		GEN_CASE(GAME_PRECACHE);
		GEN_CASE(GAME_SETMAP);
		GEN_CASE(GAME_RESTART);
		GEN_CASE(GAME_SETTIME);
		GEN_CASE(GAME_SPAWN_ENTITIES);
		GEN_CASE(GAME_CLIENT_CONNECT);
		GEN_CASE(GAME_CLIENT_BEGIN);
		GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
		GEN_CASE(GAME_CLIENT_DISCONNECT);
		GEN_CASE(GAME_CLIENT_COMMAND);
		GEN_CASE(GAME_CLIENT_THINK);
		GEN_CASE(GAME_BOTBEGIN);
		GEN_CASE(GAME_BOTTHINK);
		GEN_CASE(GAME_PREP_FRAME);
		GEN_CASE(GAME_RUN_FRAME);
		GEN_CASE(GAME_SERVER_SPAWNED);
		GEN_CASE(GAME_REGISTER_SOUNDS);
		GEN_CASE(GAME_ALLOW_PAUSED);
		GEN_CASE(GAME_CONSOLE_COMMAND);
		GEN_CASE(GAME_ARCHIVE_PERSISTANT);
		GEN_CASE(GAME_WRITE_LEVEL);
		GEN_CASE(GAME_READ_LEVEL);
		GEN_CASE(GAME_LEVEL_ARCHIVE_VALID);
		GEN_CASE(GAME_ARCHIVE_INTEGER);
		GEN_CASE(GAME_ARCHIVE_FLOAT);
		GEN_CASE(GAME_ARCHIVE_STRING);
		GEN_CASE(GAME_ARCHIVE_SVSTIME);
		GEN_CASE(GAME_TIKI_ORIENTATION);
		GEN_CASE(GAME_DEBUG_CIRCLE);
		GEN_CASE(GAME_SET_FRAME_NUMBER);
		GEN_CASE(GAME_SOUND_CALLBACK);
		GEN_CASE(GAMEVP_PROFSTRUCT);
		GEN_CASE(GAMEVP_GENTITIES);
		GEN_CASE(GAMEV_GENTITYSIZE);
		GEN_CASE(GAMEV_NUM_ENTITIES);
		GEN_CASE(GAMEV_MAX_ENTITIES);
		GEN_CASE(GAMEVP_ERRORMESSAGE);
		default:
			return "unknown";
	}
}
