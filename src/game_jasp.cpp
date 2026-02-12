/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <jasp/game/q_shared.h>
#include <jasp/game/g_public.h>

#include "game_api.h"
#include "log.h"
// QMM-specific JASP header
#include "game_jasp.h"
#include "main.h"
#include "util.h"

GEN_QMM_MSGS(JASP);
GEN_EXTS(JASP);

// a copy of the original import struct that comes from the game engine
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod
static game_export_t* orig_export = nullptr;

// struct with lambdas that call QMM's syscall function. this is given to the mod
static game_import_t qmm_import = {
        GEN_IMPORT(Printf, G_PRINTF),
        GEN_IMPORT(WriteCam, G_WRITECAM),
        GEN_IMPORT(FlushCamFile, G_FLUSHCAMFILE),
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
        GEN_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND),
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
        GEN_IMPORT(totalMapContents, G_TOTALMAPCONTENTS),
        GEN_IMPORT(inPVS, G_IN_PVS),
        GEN_IMPORT(inPVSIgnorePortals, G_IN_PVS_IGNOREPORTALS),
        GEN_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE),
        GEN_IMPORT(AreasConnected, G_AREAS_CONNECTED),
        GEN_IMPORT(linkentity, G_LINKENTITY),
        GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
        GEN_IMPORT(EntitiesInBox, G_ENTITIES_IN_BOX),
        GEN_IMPORT(EntityContact, G_ENTITY_CONTACT),
        nullptr,	// int* VoiceVolume
        GEN_IMPORT(Malloc, G_MALLOC),
        GEN_IMPORT(Free, G_FREE),
        GEN_IMPORT(bIsFromZone, G_BISFROMZONE),
        GEN_IMPORT(G2API_PrecacheGhoul2Model, G_G2API_PRECACHEGHOUL2MODEL),
        GEN_IMPORT(G2API_InitGhoul2Model, G_G2API_INITGHOUL2MODEL),
        GEN_IMPORT(G2API_SetSkin, G_G2API_SETSKIN),
        GEN_IMPORT(G2API_SetBoneAnim, G_G2API_SETBONEANIM),
        GEN_IMPORT(G2API_SetBoneAngles, G_G2API_SETBONEANGLES),
        GEN_IMPORT(G2API_SetBoneAnglesIndex, G_G2API_SETBONEANGLESINDEX),
        GEN_IMPORT(G2API_SetBoneAnglesMatrix, G_G2API_SETBONEANGLESMATRIX),
        GEN_IMPORT(G2API_CopyGhoul2Instance, G_G2API_COPYGHOUL2INSTANCE),
        GEN_IMPORT(G2API_SetBoneAnimIndex, G_G2API_SETBONEANIMINDEX),
        GEN_IMPORT(G2API_SetLodBias, G_G2API_SETLODBIAS),
        GEN_IMPORT(G2API_SetShader, G_G2API_SETSHADER),
        GEN_IMPORT(G2API_RemoveGhoul2Model, G_G2API_REMOVEGHOUL2MODEL),
        GEN_IMPORT(G2API_SetSurfaceOnOff, G_G2API_SETSURFACEONOFF),
        GEN_IMPORT(G2API_SetRootSurface, G_G2API_SETROOTSURFACE),
        GEN_IMPORT(G2API_RemoveSurface, G_G2API_REMOVESURFACE),
        GEN_IMPORT_6(G2API_AddSurface, G_G2API_ADDSURFACE, int, CGhoul2Info*, int, int, float, float, int),
        GEN_IMPORT(G2API_GetBoneAnim, G_G2API_GETBONEANIM),
        GEN_IMPORT(G2API_GetBoneAnimIndex, G_G2API_GETBONEANIMINDEX),
        GEN_IMPORT(G2API_GetAnimRange, G_G2API_GETANIMRANGE),
        GEN_IMPORT(G2API_GetAnimRangeIndex, G_G2API_GETANIMRANGEINDEX),
        GEN_IMPORT(G2API_PauseBoneAnim, G_G2API_PAUSEBONEANIM),
        GEN_IMPORT(G2API_PauseBoneAnimIndex, G_G2API_PAUSEBONEANIMINDEX),
        GEN_IMPORT(G2API_IsPaused, G_G2API_ISPAUSED),
        GEN_IMPORT(G2API_StopBoneAnim, G_G2API_STOPBONEANIM),
        GEN_IMPORT(G2API_StopBoneAngles, G_G2API_STOPBONEANGLES),
        GEN_IMPORT(G2API_RemoveBone, G_G2API_REMOVEBONE),
        GEN_IMPORT(G2API_RemoveBolt, G_G2API_REMOVEBOLT),
        GEN_IMPORT(G2API_AddBolt, G_G2API_ADDBOLT),
        GEN_IMPORT(G2API_AddBoltSurfNum, G_G2API_ADDBOLTSURFNUM),
        GEN_IMPORT(G2API_AttachG2Model, G_G2API_ATTACHG2MODEL),
        GEN_IMPORT(G2API_DetachG2Model, G_G2API_DETACHG2MODEL),
        GEN_IMPORT(G2API_AttachEnt, G_G2API_ATTACHENT),
        GEN_IMPORT(G2API_DetachEnt, G_G2API_DETACHENT),
        GEN_IMPORT(G2API_GetBoltMatrix, G_G2API_GETBOLTMATRIX),
        GEN_IMPORT(G2API_ListSurfaces, G_G2API_LISTSURFACES),
        GEN_IMPORT(G2API_ListBones, G_G2API_LISTBONES),
        GEN_IMPORT(G2API_HaveWeGhoul2Models, G_G2API_HAVEWEGHOUL2MODELS),
        GEN_IMPORT(G2API_SetGhoul2ModelFlags, G_G2API_SETGHOUL2MODELFLAGS),
        GEN_IMPORT(G2API_GetGhoul2ModelFlags, G_G2API_GETGHOUL2MODELFLAGS),
        GEN_IMPORT(G2API_GetAnimFileName, G_G2API_GETANIMFILENAME),
        GEN_IMPORT(G2API_CollisionDetect, G_G2API_COLLISIONDETECT),
        GEN_IMPORT(G2API_GiveMeVectorFromMatrix, G_G2API_GIVEMEVECTORFROMMATRIX),
        GEN_IMPORT(G2API_CleanGhoul2Models, G_G2API_CLEANGHOUL2MODELS),
        GEN_IMPORT(TheGhoul2InfoArray, G_THEGHOUL2INFOARRAY),
        GEN_IMPORT(G2API_GetParentSurface, G_G2API_GETPARENTSURFACE),
        GEN_IMPORT(G2API_GetSurfaceIndex, G_G2API_GETSURFACEINDEX),
        GEN_IMPORT(G2API_GetSurfaceName, G_G2API_GETSURFACENAME),
        GEN_IMPORT(G2API_GetGLAName, G_G2API_GETGLANAME),
        GEN_IMPORT(G2API_SetNewOrigin, G_G2API_SETNEWORIGIN),
        GEN_IMPORT(G2API_GetBoneIndex, G_G2API_GETBONEINDEX),
        GEN_IMPORT(G2API_StopBoneAnglesIndex, G_G2API_STOPBONEANGLESINDEX),
        GEN_IMPORT(G2API_StopBoneAnimIndex, G_G2API_STOPBONEANIMINDEX),
        GEN_IMPORT(G2API_SetBoneAnglesMatrixIndex, G_G2API_SETBONEANGLESMATRIXINDEX),
        GEN_IMPORT(G2API_SetAnimIndex, G_G2API_SETANIMINDEX),
        GEN_IMPORT(G2API_GetAnimIndex, G_G2API_GETANIMINDEX),
        GEN_IMPORT(G2API_SaveGhoul2Models, G_G2API_SAVEGHOUL2MODELS),
        GEN_IMPORT(G2API_LoadGhoul2Models, G_G2API_LOADGHOUL2MODELS),
        GEN_IMPORT(G2API_LoadSaveCodeDestructGhoul2Info, G_G2API_LOADSAVECODEDESTRUCTGHOUL2INFO),
        GEN_IMPORT(G2API_GetAnimFileNameIndex, G_G2API_GETANIMFILENAMEINDEX),
        GEN_IMPORT(G2API_GetAnimFileInternalNameIndex, G_G2API_GETANIMFILEINTERNALNAMEINDEX),
        GEN_IMPORT(G2API_GetSurfaceRenderStatus, G_G2API_GETSURFACERENDERSTATUS),
        GEN_IMPORT(G2API_SetRagDoll, G_G2API_SETRAGDOLL),
        GEN_IMPORT(G2API_AnimateG2Models, G_G2API_ANIMATEG2MODELS),
        GEN_IMPORT(G2API_RagPCJConstraint, G_G2API_RAGPCJCONSTRAINT),
        GEN_IMPORT_3(G2API_RagPCJGradientSpeed, G_G2API_RAGPCJGRADIENTSPEED, qboolean, CGhoul2Info_v&, const char *, const float),
        GEN_IMPORT(G2API_RagEffectorGoal, G_G2API_RAGEFFECTORGOAL),
        GEN_IMPORT(G2API_GetRagBonePos, G_G2API_GETRAGBONEPOS),
        GEN_IMPORT(G2API_RagEffectorKick, G_G2API_RAGEFFECTORKICK),
        GEN_IMPORT(G2API_RagForceSolve, G_G2API_RAGFORCESOLVE),
        GEN_IMPORT(G2API_SetBoneIKState, G_G2API_SETBONEIKSTATE),
        GEN_IMPORT(G2API_IKMove, G_G2API_IKMOVE),
        GEN_IMPORT(G2API_AddSkinGore, G_G2API_ADDSKINGORE),
        GEN_IMPORT(G2API_ClearSkinGore, G_G2API_CLEARSKINGORE),
        GEN_IMPORT(RMG_Init, G_RMG_INIT),
        GEN_IMPORT(CM_RegisterTerrain, G_CM_REGISTERTERRAIN),
        GEN_IMPORT(SetActiveSubBSP, G_SET_ACTIVE_SUBBSP),
        GEN_IMPORT(RE_RegisterSkin, G_RE_REGISTERSKIN),
        GEN_IMPORT(RE_GetAnimationCFG, G_RE_GETANIMATIONCFG),
        GEN_IMPORT(WE_GetWindVector, G_WE_GETWINDVECTOR),
        GEN_IMPORT(WE_GetWindGusting, G_WE_GETWINDGUSTING),
        GEN_IMPORT(WE_IsOutside, G_WE_ISOUTSIDE),
        GEN_IMPORT(WE_IsOutsideCausingPain, G_WE_ISOUTSIDECAUSINGPAIN),	// todo: returns float
        GEN_IMPORT(WE_GetChanceOfSaberFizz, G_WE_GETCHANCEOFSABERFIZZ),	// todo: returns float
        GEN_IMPORT(WE_IsShaking, G_WE_ISSHAKING),
        GEN_IMPORT(WE_AddWeatherZone, G_WE_ADDWEATHERZONE),
        GEN_IMPORT(WE_SetTempGlobalFogColor, G_WE_SETTEMPGLOBALFOGCOLOR),
};


// these are "pre" hooks for storing some data for polyfills.
// we need these to be called BEFORE plugins' prehooks get called so they have to be done in the qmm_export table

// track entstrings for our G_GET_ENTITY_TOKEN syscall
static std::vector<std::string> s_entity_tokens;
static size_t s_tokencount = 0;
static void JASP_Init(const char* mapname, const char* spawntarget, int checkSum, const char* entstring, int levelTime, int randomSeed, int globalTime, SavedGameJustLoaded_e eSavedGameJustLoaded, qboolean qbLoadTransition) {
    if (entstring) {
        s_entity_tokens = util_parse_entstring(entstring);
        s_tokencount = 0;
    }
    cgame_is_QMM_vmMain_call = true;
    vmMain(GAME_INIT, mapname, spawntarget, checkSum, entstring, levelTime, randomSeed, globalTime, eSavedGameJustLoaded, qbLoadTransition);
}


// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
    GAME_API_VERSION,	// apiversion
    JASP_Init,
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
    GEN_EXPORT(ConnectNavs, GAME_CONNECTNAVS),
    GEN_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND),
    GEN_EXPORT(GameSpawnRMGEntity, GAME_SPAWN_RMG_ENTITY),

    // the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
    nullptr,	// gentities
    0,			// gentitySize
    0,			// num_entities
};


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t JASP_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JASP_syscall({} {}) called\n", JASP_eng_msg_names(cmd), cmd);
#endif

    intptr_t ret = 0;

    switch (cmd) {
        ROUTE_IMPORT(Printf, G_PRINTF);
        ROUTE_IMPORT(WriteCam, G_WRITECAM);
        ROUTE_IMPORT(FlushCamFile, G_FLUSHCAMFILE);
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
        // handled below since we do special handling to deal with the "when" argument
        // ROUTE_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND);
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
        ROUTE_IMPORT(totalMapContents, G_TOTALMAPCONTENTS);
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
        ROUTE_IMPORT(bIsFromZone, G_BISFROMZONE);
        ROUTE_IMPORT(G2API_PrecacheGhoul2Model, G_G2API_PRECACHEGHOUL2MODEL);
        ROUTE_IMPORT(G2API_InitGhoul2Model, G_G2API_INITGHOUL2MODEL);
        ROUTE_IMPORT(G2API_SetSkin, G_G2API_SETSKIN);
        ROUTE_IMPORT(G2API_SetBoneAnim, G_G2API_SETBONEANIM);
        ROUTE_IMPORT(G2API_SetBoneAngles, G_G2API_SETBONEANGLES);
        ROUTE_IMPORT(G2API_SetBoneAnglesIndex, G_G2API_SETBONEANGLESINDEX);
        ROUTE_IMPORT(G2API_SetBoneAnglesMatrix, G_G2API_SETBONEANGLESMATRIX);
        ROUTE_IMPORT(G2API_CopyGhoul2Instance, G_G2API_COPYGHOUL2INSTANCE);
        ROUTE_IMPORT(G2API_SetBoneAnimIndex, G_G2API_SETBONEANIMINDEX);
        ROUTE_IMPORT(G2API_SetLodBias, G_G2API_SETLODBIAS);
        ROUTE_IMPORT(G2API_SetShader, G_G2API_SETSHADER);
        ROUTE_IMPORT(G2API_RemoveGhoul2Model, G_G2API_REMOVEGHOUL2MODEL);
        ROUTE_IMPORT(G2API_SetSurfaceOnOff, G_G2API_SETSURFACEONOFF);
        ROUTE_IMPORT(G2API_SetRootSurface, G_G2API_SETROOTSURFACE);
        ROUTE_IMPORT(G2API_RemoveSurface, G_G2API_REMOVESURFACE);
        ROUTE_IMPORT(G2API_AddSurface, G_G2API_ADDSURFACE);
        ROUTE_IMPORT(G2API_GetBoneAnim, G_G2API_GETBONEANIM);
        ROUTE_IMPORT(G2API_GetBoneAnimIndex, G_G2API_GETBONEANIMINDEX);
        ROUTE_IMPORT(G2API_GetAnimRange, G_G2API_GETANIMRANGE);
        ROUTE_IMPORT(G2API_GetAnimRangeIndex, G_G2API_GETANIMRANGEINDEX);
        ROUTE_IMPORT(G2API_PauseBoneAnim, G_G2API_PAUSEBONEANIM);
        ROUTE_IMPORT(G2API_PauseBoneAnimIndex, G_G2API_PAUSEBONEANIMINDEX);
        ROUTE_IMPORT(G2API_IsPaused, G_G2API_ISPAUSED);
        ROUTE_IMPORT(G2API_StopBoneAnim, G_G2API_STOPBONEANIM);
        ROUTE_IMPORT(G2API_StopBoneAngles, G_G2API_STOPBONEANGLES);
        ROUTE_IMPORT(G2API_RemoveBone, G_G2API_REMOVEBONE);
        ROUTE_IMPORT(G2API_RemoveBolt, G_G2API_REMOVEBOLT);
        ROUTE_IMPORT(G2API_AddBolt, G_G2API_ADDBOLT);
        ROUTE_IMPORT(G2API_AddBoltSurfNum, G_G2API_ADDBOLTSURFNUM);
        ROUTE_IMPORT(G2API_AttachG2Model, G_G2API_ATTACHG2MODEL);
        ROUTE_IMPORT(G2API_DetachG2Model, G_G2API_DETACHG2MODEL);
        ROUTE_IMPORT(G2API_AttachEnt, G_G2API_ATTACHENT);
        ROUTE_IMPORT(G2API_DetachEnt, G_G2API_DETACHENT);
        ROUTE_IMPORT(G2API_GetBoltMatrix, G_G2API_GETBOLTMATRIX);
        ROUTE_IMPORT(G2API_ListSurfaces, G_G2API_LISTSURFACES);
        ROUTE_IMPORT(G2API_ListBones, G_G2API_LISTBONES);
        ROUTE_IMPORT(G2API_HaveWeGhoul2Models, G_G2API_HAVEWEGHOUL2MODELS);
        ROUTE_IMPORT(G2API_SetGhoul2ModelFlags, G_G2API_SETGHOUL2MODELFLAGS);
        ROUTE_IMPORT(G2API_GetGhoul2ModelFlags, G_G2API_GETGHOUL2MODELFLAGS);
        ROUTE_IMPORT(G2API_GetAnimFileName, G_G2API_GETANIMFILENAME);
        ROUTE_IMPORT(G2API_CollisionDetect, G_G2API_COLLISIONDETECT);
        ROUTE_IMPORT(G2API_GiveMeVectorFromMatrix, G_G2API_GIVEMEVECTORFROMMATRIX);
        ROUTE_IMPORT(G2API_CleanGhoul2Models, G_G2API_CLEANGHOUL2MODELS);
        ROUTE_IMPORT(TheGhoul2InfoArray, G_THEGHOUL2INFOARRAY);
        ROUTE_IMPORT(G2API_GetParentSurface, G_G2API_GETPARENTSURFACE);
        ROUTE_IMPORT(G2API_GetSurfaceIndex, G_G2API_GETSURFACEINDEX);
        ROUTE_IMPORT(G2API_GetSurfaceName, G_G2API_GETSURFACENAME);
        ROUTE_IMPORT(G2API_GetGLAName, G_G2API_GETGLANAME);
        ROUTE_IMPORT(G2API_SetNewOrigin, G_G2API_SETNEWORIGIN);
        ROUTE_IMPORT(G2API_GetBoneIndex, G_G2API_GETBONEINDEX);
        ROUTE_IMPORT(G2API_StopBoneAnglesIndex, G_G2API_STOPBONEANGLESINDEX);
        ROUTE_IMPORT(G2API_StopBoneAnimIndex, G_G2API_STOPBONEANIMINDEX);
        ROUTE_IMPORT(G2API_SetBoneAnglesMatrixIndex, G_G2API_SETBONEANGLESMATRIXINDEX);
        ROUTE_IMPORT(G2API_SetAnimIndex, G_G2API_SETANIMINDEX);
        ROUTE_IMPORT(G2API_GetAnimIndex, G_G2API_GETANIMINDEX);
        ROUTE_IMPORT(G2API_SaveGhoul2Models, G_G2API_SAVEGHOUL2MODELS);
        ROUTE_IMPORT(G2API_LoadGhoul2Models, G_G2API_LOADGHOUL2MODELS);
        ROUTE_IMPORT(G2API_LoadSaveCodeDestructGhoul2Info, G_G2API_LOADSAVECODEDESTRUCTGHOUL2INFO);
        ROUTE_IMPORT(G2API_GetAnimFileNameIndex, G_G2API_GETANIMFILENAMEINDEX);
        ROUTE_IMPORT(G2API_GetAnimFileInternalNameIndex, G_G2API_GETANIMFILEINTERNALNAMEINDEX);
        ROUTE_IMPORT(G2API_GetSurfaceRenderStatus, G_G2API_GETSURFACERENDERSTATUS);
        ROUTE_IMPORT(G2API_SetRagDoll, G_G2API_SETRAGDOLL);
        ROUTE_IMPORT(G2API_AnimateG2Models, G_G2API_ANIMATEG2MODELS);
        ROUTE_IMPORT(G2API_RagPCJConstraint, G_G2API_RAGPCJCONSTRAINT);
        ROUTE_IMPORT(G2API_RagPCJGradientSpeed, G_G2API_RAGPCJGRADIENTSPEED);
        ROUTE_IMPORT(G2API_RagEffectorGoal, G_G2API_RAGEFFECTORGOAL);
        ROUTE_IMPORT(G2API_GetRagBonePos, G_G2API_GETRAGBONEPOS);
        ROUTE_IMPORT(G2API_RagEffectorKick, G_G2API_RAGEFFECTORKICK);
        ROUTE_IMPORT(G2API_RagForceSolve, G_G2API_RAGFORCESOLVE);
        ROUTE_IMPORT(G2API_SetBoneIKState, G_G2API_SETBONEIKSTATE);
        ROUTE_IMPORT(G2API_IKMove, G_G2API_IKMOVE);
        ROUTE_IMPORT(G2API_AddSkinGore, G_G2API_ADDSKINGORE);
        ROUTE_IMPORT(G2API_ClearSkinGore, G_G2API_CLEARSKINGORE);
        ROUTE_IMPORT(RMG_Init, G_RMG_INIT);
        ROUTE_IMPORT(CM_RegisterTerrain, G_CM_REGISTERTERRAIN);
        ROUTE_IMPORT(SetActiveSubBSP, G_SET_ACTIVE_SUBBSP);
        ROUTE_IMPORT(RE_RegisterSkin, G_RE_REGISTERSKIN);
        ROUTE_IMPORT(RE_GetAnimationCFG, G_RE_GETANIMATIONCFG);
        ROUTE_IMPORT(WE_GetWindVector, G_WE_GETWINDVECTOR);
        ROUTE_IMPORT(WE_GetWindGusting, G_WE_GETWINDGUSTING);
        ROUTE_IMPORT(WE_IsOutside, G_WE_ISOUTSIDE);
        ROUTE_IMPORT(WE_IsOutsideCausingPain, G_WE_ISOUTSIDECAUSINGPAIN);
        ROUTE_IMPORT(WE_GetChanceOfSaberFizz, G_WE_GETCHANCEOFSABERFIZZ);
        ROUTE_IMPORT(WE_IsShaking, G_WE_ISSHAKING);
        ROUTE_IMPORT(WE_AddWeatherZone, G_WE_ADDWEATHERZONE);
        ROUTE_IMPORT(WE_SetTempGlobalFogColor, G_WE_SETTEMPGLOBALFOGCOLOR);

        // handle cmds for variables, this is how a plugin would get these values if needed
        ROUTE_IMPORT_VAR(VoiceVolume, GVP_VOICEVOLUME);

    // handle special cmds which QMM uses but JASP doesn't have an analogue for
    case G_CVAR_REGISTER: {
        // jasp: cvar_t* (*cvar)(const char* varName, const char* varValue, int varFlags)
        // q3a: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
        // qmm always passes NULL for vmCvar so don't worry about it
        const char* varName = (const char*)(args[1]);
        const char* defaultValue = (const char*)(args[2]);
        int flags = args[3];
        (void)orig_import.cvar(varName, defaultValue, flags);
        break;
    }
    case G_SEND_CONSOLE_COMMAND_QMM:
    case G_SEND_CONSOLE_COMMAND: {
        // JASP: void (*SendConsoleCommand)(const char *text);
        // qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
        // first arg may be exec_when, like EXEC_APPEND
        intptr_t when = args[0];
        const char* text = (const char*)(args[1]);
        // EXEC_APPEND is the highest flag in all known games at 2, but go with 100 to be safe
        if (when > 100)
            text = (const char*)when;
        orig_import.SendConsoleCommand(text);
        break;
    }
    case G_LOCATE_GAME_DATA: {
        // help plugins not need separate logic for entity/client pointers
        // void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient);
        // this is just to be hooked by plugins, so ignore everything
        break;
    }
    case G_GET_ENTITY_TOKEN: {
        // qboolean trap_GetEntityToken(char *buffer, int bufferSize);
        if (s_tokencount >= s_entity_tokens.size()) {
            ret = qfalse;
            break;
        }

        char* buffer = (char*)args[0];
        intptr_t bufferSize = args[1];

        strncpyz(buffer, s_entity_tokens[s_tokencount++].c_str(), (size_t)bufferSize);
        ret = qtrue;
        break;
    }
    case G_ARGS: {
        // quake2: char* (*args)(void);
        static std::string s;
        s = "";
        int i = 1;
        while (i < orig_import.argc()) {
            if (i != 1)
                s += " ";
            s += orig_import.argv(i);
        }
        ret = (intptr_t)s.c_str();
        break;
    }

    default:
        break;
    };

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JASP_syscall({} {}) returning {}\n", JASP_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t JASP_vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JASP_vmMain({} {}) called\n", JASP_mod_msg_names(cmd), cmd);
#endif

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
        ROUTE_EXPORT(ConnectNavs, GAME_CONNECTNAVS);
        ROUTE_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND);
        ROUTE_EXPORT(GameSpawnRMGEntity, GAME_SPAWN_RMG_ENTITY);

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
            // single player only makes 1 client
            playerState_s* clients = gentities->client;
            intptr_t clientsize = 1;
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

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JASP_vmMain({} {}) returning {}\n", JASP_mod_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


void* JASP_GetGameAPI(void* import, void*) {
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JASP_GetGameAPI({}) called\n", import);

    // original import struct from engine
    // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
    game_import_t* gi = (game_import_t*)import;
    orig_import = *gi;

    // fill in variables of our hooked import struct to pass to the mod
    qmm_import.VoiceVolume = orig_import.VoiceVolume;

    // pointer to wrapper vmMain function that calls actual mod func from orig_export
    g_gameinfo.pfnvmMain = JASP_vmMain;

    // pointer to wrapper syscall function that calls actual engine func from orig_import
    g_gameinfo.pfnsyscall = JASP_syscall;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JASP_GetGameAPI({}) returning {}\n", import, (void*)&qmm_export);

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // the only thing in this struct the engine uses before calling Init is the apiversion
    return &qmm_export;
}


bool JASP_mod_load(void* entry) {
    mod_GetGameAPI_t mod_GetGameAPI = (mod_GetGameAPI_t)entry;
    orig_export = (game_export_t*)mod_GetGameAPI(&qmm_import, nullptr);


    return !!orig_export;
}


void JASP_mod_unload() {
    orig_export = nullptr;
}


const char* JASP_eng_msg_names(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_PRINTF);
        GEN_CASE(G_WRITECAM);
        GEN_CASE(G_FLUSHCAMFILE);
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
        GEN_CASE(G_SEND_CONSOLE_COMMAND);
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
        GEN_CASE(G_TOTALMAPCONTENTS);
        GEN_CASE(G_IN_PVS);
        GEN_CASE(G_IN_PVS_IGNOREPORTALS);
        GEN_CASE(G_ADJUSTAREAPORTALSTATE);
        GEN_CASE(G_AREAS_CONNECTED);
        GEN_CASE(G_LINKENTITY);
        GEN_CASE(G_UNLINKENTITY);
        GEN_CASE(G_ENTITIES_IN_BOX);
        GEN_CASE(G_ENTITY_CONTACT);
        GEN_CASE(GVP_VOICEVOLUME);
        GEN_CASE(G_MALLOC);
        GEN_CASE(G_FREE);
        GEN_CASE(G_BISFROMZONE);
        GEN_CASE(G_G2API_PRECACHEGHOUL2MODEL);
        GEN_CASE(G_G2API_INITGHOUL2MODEL);
        GEN_CASE(G_G2API_SETSKIN);
        GEN_CASE(G_G2API_SETBONEANIM);
        GEN_CASE(G_G2API_SETBONEANGLES);
        GEN_CASE(G_G2API_SETBONEANGLESINDEX);
        GEN_CASE(G_G2API_SETBONEANGLESMATRIX);
        GEN_CASE(G_G2API_COPYGHOUL2INSTANCE);
        GEN_CASE(G_G2API_SETBONEANIMINDEX);
        GEN_CASE(G_G2API_SETLODBIAS);
        GEN_CASE(G_G2API_SETSHADER);
        GEN_CASE(G_G2API_REMOVEGHOUL2MODEL);
        GEN_CASE(G_G2API_SETSURFACEONOFF);
        GEN_CASE(G_G2API_SETROOTSURFACE);
        GEN_CASE(G_G2API_REMOVESURFACE);
        GEN_CASE(G_G2API_ADDSURFACE);
        GEN_CASE(G_G2API_GETBONEANIM);
        GEN_CASE(G_G2API_GETBONEANIMINDEX);
        GEN_CASE(G_G2API_GETANIMRANGE);
        GEN_CASE(G_G2API_GETANIMRANGEINDEX);
        GEN_CASE(G_G2API_PAUSEBONEANIM);
        GEN_CASE(G_G2API_PAUSEBONEANIMINDEX);
        GEN_CASE(G_G2API_ISPAUSED);
        GEN_CASE(G_G2API_STOPBONEANIM);
        GEN_CASE(G_G2API_STOPBONEANGLES);
        GEN_CASE(G_G2API_REMOVEBONE);
        GEN_CASE(G_G2API_REMOVEBOLT);
        GEN_CASE(G_G2API_ADDBOLT);
        GEN_CASE(G_G2API_ADDBOLTSURFNUM);
        GEN_CASE(G_G2API_ATTACHG2MODEL);
        GEN_CASE(G_G2API_DETACHG2MODEL);
        GEN_CASE(G_G2API_ATTACHENT);
        GEN_CASE(G_G2API_DETACHENT);
        GEN_CASE(G_G2API_GETBOLTMATRIX);
        GEN_CASE(G_G2API_LISTSURFACES);
        GEN_CASE(G_G2API_LISTBONES);
        GEN_CASE(G_G2API_HAVEWEGHOUL2MODELS);
        GEN_CASE(G_G2API_SETGHOUL2MODELFLAGS);
        GEN_CASE(G_G2API_GETGHOUL2MODELFLAGS);
        GEN_CASE(G_G2API_GETANIMFILENAME);
        GEN_CASE(G_G2API_COLLISIONDETECT);
        GEN_CASE(G_G2API_GIVEMEVECTORFROMMATRIX);
        GEN_CASE(G_G2API_CLEANGHOUL2MODELS);
        GEN_CASE(G_THEGHOUL2INFOARRAY);
        GEN_CASE(G_G2API_GETPARENTSURFACE);
        GEN_CASE(G_G2API_GETSURFACEINDEX);
        GEN_CASE(G_G2API_GETSURFACENAME);
        GEN_CASE(G_G2API_GETGLANAME);
        GEN_CASE(G_G2API_SETNEWORIGIN);
        GEN_CASE(G_G2API_GETBONEINDEX);
        GEN_CASE(G_G2API_STOPBONEANGLESINDEX);
        GEN_CASE(G_G2API_STOPBONEANIMINDEX);
        GEN_CASE(G_G2API_SETBONEANGLESMATRIXINDEX);
        GEN_CASE(G_G2API_SETANIMINDEX);
        GEN_CASE(G_G2API_GETANIMINDEX);
        GEN_CASE(G_G2API_SAVEGHOUL2MODELS);
        GEN_CASE(G_G2API_LOADGHOUL2MODELS);
        GEN_CASE(G_G2API_LOADSAVECODEDESTRUCTGHOUL2INFO);
        GEN_CASE(G_G2API_GETANIMFILENAMEINDEX);
        GEN_CASE(G_G2API_GETANIMFILEINTERNALNAMEINDEX);
        GEN_CASE(G_G2API_GETSURFACERENDERSTATUS);
        GEN_CASE(G_G2API_SETRAGDOLL);
        GEN_CASE(G_G2API_ANIMATEG2MODELS);
        GEN_CASE(G_G2API_RAGPCJCONSTRAINT);
        GEN_CASE(G_G2API_RAGPCJGRADIENTSPEED);
        GEN_CASE(G_G2API_RAGEFFECTORGOAL);
        GEN_CASE(G_G2API_GETRAGBONEPOS);
        GEN_CASE(G_G2API_RAGEFFECTORKICK);
        GEN_CASE(G_G2API_RAGFORCESOLVE);
        GEN_CASE(G_G2API_SETBONEIKSTATE);
        GEN_CASE(G_G2API_IKMOVE);
        GEN_CASE(G_G2API_ADDSKINGORE);
        GEN_CASE(G_G2API_CLEARSKINGORE);
        GEN_CASE(G_RMG_INIT);
        GEN_CASE(G_CM_REGISTERTERRAIN);
        GEN_CASE(G_SET_ACTIVE_SUBBSP);
        GEN_CASE(G_RE_REGISTERSKIN);
        GEN_CASE(G_RE_GETANIMATIONCFG);
        GEN_CASE(G_WE_GETWINDVECTOR);
        GEN_CASE(G_WE_GETWINDGUSTING);
        GEN_CASE(G_WE_ISOUTSIDE);
        GEN_CASE(G_WE_ISOUTSIDECAUSINGPAIN);
        GEN_CASE(G_WE_GETCHANCEOFSABERFIZZ);
        GEN_CASE(G_WE_ISSHAKING);
        GEN_CASE(G_WE_ADDWEATHERZONE);
        GEN_CASE(G_WE_SETTEMPGLOBALFOGCOLOR);

        // polyfills
        GEN_CASE(G_CVAR_REGISTER);

        GEN_CASE(G_LOCATE_GAME_DATA);
        GEN_CASE(G_GET_ENTITY_TOKEN);

    default:
        return "unknown";
    }
}


const char* JASP_mod_msg_names(intptr_t cmd) {
    switch (cmd) {
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
        GEN_CASE(GAME_CONNECTNAVS);
        GEN_CASE(GAME_CONSOLE_COMMAND);
        GEN_CASE(GAME_SPAWN_RMG_ENTITY);
        GEN_CASE(GAMEVP_GENTITIES);
        GEN_CASE(GAMEV_GENTITYSIZE);
        GEN_CASE(GAMEV_NUM_ENTITIES);
    default:
        return "unknown";
    }
}
