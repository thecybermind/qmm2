/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "version.h"

#if defined(QMM_ARCH_32)

#include <stef2/game/q_shared.h>
#define GAME_DLL
#include <stef2/game/g_public.h>
#undef GAME_DLL

#include "game_api.hpp"
#include "log.hpp"
#include <vector>
#include <string>
// QMM-specific STEF2 header
#include "game_stef2.h"
#include "gameinfo.hpp"
#include "main.hpp"
#include "util.hpp"

struct STEF2_GameSupport : public GameSupport {
    virtual const char* EngMsgName(intptr_t msg);
    virtual const char* ModMsgName(intptr_t msg);
    virtual bool AutoDetect(APIType engine_api);
    virtual void* Entry(void* syscall, void*, APIType engine_api);
    virtual bool ModLoad(void* entry, APIType mod_api);
    virtual void ModUnload();
    virtual int QMMEngMsg(int msg) { return qmm_eng_msgs[msg]; }
    virtual int QMMModMsg(int msg) { return qmm_mod_msgs[msg]; }

    virtual intptr_t syscall(intptr_t, ...);
    virtual intptr_t vmMain(intptr_t, ...);

    virtual const char* DefaultDLLName() { return "game" MOD_DLL; }
    virtual const char* DefaultModDir() { return "BASE"; }
    virtual const char* GameName() { return "Star Trek: Elite Force II"; }
    virtual const char* GameCode() { return "STEF2"; }

private:
    // update the export variables from orig_export
    static void update_exports();

    // track entstrings for our G_GET_ENTITY_TOKEN syscall
    static std::vector<std::string> entity_tokens;
    static size_t token_counter;
    static void SpawnEntities(const char* mapname, const char* entstring, int levelTime);

    // a copy of the original import struct that comes from the game engine
    static game_import_t orig_import;

    // a copy of the original export struct pointer that comes from the mod
    static game_export_t* orig_export;

    // struct with lambdas that call QMM's syscall function. this is given to the mod
    static game_import_t qmm_import;

    // struct with lambdas that call QMM's vmMain function. this is given to the game engine
    static game_export_t qmm_export;

    const int qmm_eng_msgs[QMM_ENGINE_MSG_COUNT] = GEN_GAME_QMM_ENG_MSGS();
    const int qmm_mod_msgs[QMM_MOD_MSG_COUNT] = GEN_GAME_QMM_MOD_MSGS();
};

GEN_GAME_OBJ(STEF2);


// auto-detection logic for STEF2
bool STEF2_GameSupport::AutoDetect(APIType engineapi) {
    if (engineapi != QMM_API_GETGAMEAPI)
        return false;

    if (!str_striequal(gameinfo.qmm_file, DefaultDLLName()))
        return false;

    if (!str_stristr(gameinfo.exe_file, "ef"))
        return false;

    return true;
}


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t STEF2_GameSupport::syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

    if (cmd != G_PRINT)
        QMMLOG(QMM_LOG_TRACE, "QMM") << "STEF2_GameSupport::syscall(" << EngMsgName(cmd) << "(" << cmd << ")) called\n";

    // update export vars before calling into the engine
    update_exports();

    intptr_t ret = 0;

    float fret; // used to get float return values

    switch (cmd) {
        ROUTE_IMPORT(Printf, G_PRINTF);
        ROUTE_IMPORT(DPrintf, G_DPRINTF);
        ROUTE_IMPORT(WPrintf, G_WPRINTF);
        ROUTE_IMPORT(WDPrintf, G_WDPRINTF);
        ROUTE_IMPORT(DebugPrintf, G_DEBUGPRINTF);
        ROUTE_IMPORT(LocalizeFilePath, G_LOCALIZEFILEPATH);
        ROUTE_IMPORT(Error, G_ERROR);
        ROUTE_IMPORT(Milliseconds, G_MILLISECONDS);
        ROUTE_IMPORT(Malloc, G_MALLOC);
        ROUTE_IMPORT(Free, G_FREE);
        ROUTE_IMPORT(cvar, G_CVAR);
        ROUTE_IMPORT(cvar_get, G_CVAR_GET);
        ROUTE_IMPORT(cvar_set, G_CVAR_SET);
        ROUTE_IMPORT(Cvar_VariableStringBuffer, G_CVAR_VARIABLE_STRING_BUFFER);
        ROUTE_IMPORT(Cvar_Register, G_CVAR_REGISTER);
        ROUTE_IMPORT_1_F(Cvar_VariableValue, G_CVAR_VARIABLEVALUE, (const char*));
        ROUTE_IMPORT(Cvar_Update, G_CVAR_UPDATE);
        ROUTE_IMPORT(Cvar_VariableIntegerValue, G_CVAR_VARIABLE_INTEGER_VALUE);
        ROUTE_IMPORT(argc, G_ARGC);
        ROUTE_IMPORT(argv, G_ARGV);
        ROUTE_IMPORT(args, G_ARGS);
        ROUTE_IMPORT(AddCommand, G_ADDCOMMAND);
        ROUTE_IMPORT(FS_ReadFile, G_FS_READFILE);
        ROUTE_IMPORT(FS_Exists, G_FS_EXISTS);
        ROUTE_IMPORT(FS_FreeFile, G_FS_FREEFILE);
        ROUTE_IMPORT(FS_WriteFile, G_FS_WRITEFILE);
        ROUTE_IMPORT(FS_FOpenFileWrite, G_FS_FOPEN_FILE_WRITE);
        ROUTE_IMPORT(FS_FOpenFileAppend, G_FS_FOPEN_FILE_APPEND);
        ROUTE_IMPORT(FS_ListFiles, G_FS_LISTFILES);
        ROUTE_IMPORT(FS_PrepFileWrite, G_FS_PREPFILEWRITE);
        ROUTE_IMPORT(FS_Write, G_FS_WRITE);
        ROUTE_IMPORT(FS_Read, G_FS_READ);
        ROUTE_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE);
        ROUTE_IMPORT(FS_FTell, G_FS_FTELL);
        ROUTE_IMPORT(FS_FSeek, G_FS_FSEEK);
        ROUTE_IMPORT(FS_Flush, G_FS_FLUSH);
        ROUTE_IMPORT(FS_DeleteFile, G_FS_DELETEFILE);
        ROUTE_IMPORT(FS_GetFileList, G_FS_GETFILELIST);
        ROUTE_IMPORT(GetArchiveFileName, G_GETARCHIVEFILENAME);
        // handled below since we do special handling to deal with the "when" argument
        // ROUTE_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND);
        ROUTE_IMPORT_2_V(DebugGraph, G_DEBUGGRAPH, *(float*)&, (int));
        ROUTE_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND);
        ROUTE_IMPORT(GetNumFreeReliableServerCommands, G_GETNUMFREERELIABLESERVERCOMMANDS);
        ROUTE_IMPORT(setConfigstring, G_SET_CONFIGSTRING);
        ROUTE_IMPORT(getConfigstring, G_GET_CONFIGSTRING);
        ROUTE_IMPORT(setUserinfo, G_SET_USERINFO);
        ROUTE_IMPORT(getUserinfo, G_GET_USERINFO);
        ROUTE_IMPORT(SetBrushModel, G_SET_BRUSH_MODEL);
        ROUTE_IMPORT(trace, G_TRACE);
        ROUTE_IMPORT(fulltrace, G_FULLTRACE);
        ROUTE_IMPORT(pointcontents, G_POINT_CONTENTS);
        ROUTE_IMPORT(pointbrushnum, G_POINTBRUSHNUM);
        ROUTE_IMPORT(inPVS, G_IN_PVS);
        ROUTE_IMPORT(inPVSIgnorePortals, G_IN_PVS_IGNOREPORTALS);
        ROUTE_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE);
        ROUTE_IMPORT(AreasConnected, G_AREAS_CONNECTED);
        ROUTE_IMPORT(GetLightingGroup, G_GETLIGHTINGGROUP);
        ROUTE_IMPORT_2_V(SetDynamicLight, G_SETDYNAMICLIGHT, (int), *(float*)&);
        ROUTE_IMPORT_2_V(SetDynamicLightDefault, G_SETDYNAMICLIGHTDEFAULT, (int), *(float*)&);
        ROUTE_IMPORT(SetWindDirection, G_SETWINDDIRECTION);
        ROUTE_IMPORT_1_V(SetWindIntensity, G_SETWINDINTENSITY, *(float*)&);
        ROUTE_IMPORT(SetWeatherInfo, G_SETWEATHERINFO);
        ROUTE_IMPORT_1_V(SetTimeScale, G_SETTIMESCALE, *(float*)&);
        ROUTE_IMPORT(linkentity, G_LINKENTITY);
        ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
        ROUTE_IMPORT(AreaEntities, G_AREAENTITIES);
        ROUTE_IMPORT(ClipToEntity, G_CLIPTOENTITY);
        ROUTE_IMPORT(objectivenameindex, G_OBJECTIVENAMEINDEX);
        ROUTE_IMPORT(archetypeindex, G_ARCHETYPEINDEX);
        ROUTE_IMPORT(imageindex, G_IMAGEINDEX);
        ROUTE_IMPORT(failedcondition, G_FAILEDCONDITION);
        ROUTE_IMPORT(itemindex, G_ITEMINDEX);
        ROUTE_IMPORT(soundindex, G_SOUNDINDEX);
        ROUTE_IMPORT(modelindex, G_MODELINDEX);
        ROUTE_IMPORT(SetLightStyle, G_SETLIGHTSTYLE);
        ROUTE_IMPORT(GameDir, G_GAMEDIR);
        ROUTE_IMPORT(IsModel, G_ISMODEL);
        ROUTE_IMPORT(setmodel, G_SETMODEL);
        ROUTE_IMPORT(setviewmodel, G_SETVIEWMODEL);
        ROUTE_IMPORT(NumAnims, G_NUMANIMS);
        ROUTE_IMPORT(NumSkins, G_NUMSKINS);
        ROUTE_IMPORT(NumSurfaces, G_NUMSURFACES);
        ROUTE_IMPORT(NumTags, G_NUMTAGS);
        ROUTE_IMPORT(NumMorphs, G_NUMMORPHS);
        ROUTE_IMPORT(InitCommands, G_INITCOMMANDS);
        ROUTE_IMPORT_4_V(CalculateBounds, G_CALCULATEBOUNDS, (int), *(float*)&, (float*), (float*));
        ROUTE_IMPORT(TIKI_CacheAnim, G_TIKI_CACHEANIM);
        ROUTE_IMPORT(Anim_NameForNum, G_ANIM_NAMEFORNUM);
        ROUTE_IMPORT(Anim_NumForName, G_ANIM_NUMFORNAME);
        ROUTE_IMPORT(Anim_Random, G_ANIM_RANDOM);
        ROUTE_IMPORT(Anim_NumFrames, G_ANIM_NUMFRAMES);
        ROUTE_IMPORT_2_F(Anim_Time, G_ANIM_TIME, (int), (int));
        ROUTE_IMPORT(Anim_Delta, G_ANIM_DELTA);
        ROUTE_IMPORT(Anim_AbsoluteDelta, G_ANIM_ABSOLUTEDELTA);
        ROUTE_IMPORT(Anim_Flags, G_ANIM_FLAGS);
        ROUTE_IMPORT(Anim_HasCommands, G_ANIM_HASCOMMANDS);
        ROUTE_IMPORT(Frame_Commands, G_FRAME_COMMANDS);
        ROUTE_IMPORT(Frame_Delta, G_FRAME_DELTA);
        ROUTE_IMPORT_3_F(Frame_Time, G_FRAME_TIME, (int), (int), (int));
        ROUTE_IMPORT_6_V(Frame_Bounds, G_FRAME_BOUNDS, (int), (int), (int), *(float*)&, (float*), (float*));
        ROUTE_IMPORT(Surface_NameToNum, G_SURFACE_NAMETONUM);
        ROUTE_IMPORT(Surface_NumToName, G_SURFACE_NUMTONAME);
        ROUTE_IMPORT(Surface_Flags, G_SURFACE_FLAGS);
        ROUTE_IMPORT(Surface_NumSkins, G_SURFACE_NUMSKINS);
        ROUTE_IMPORT(Morph_NumForName, G_MORPH_NUMFORNAME);
        ROUTE_IMPORT(Morph_NameForNum, G_MORPH_NAMEFORNUM);
        ROUTE_IMPORT(GetExpression, G_GETEXPRESSION);
        ROUTE_IMPORT(Tag_NumForName, G_TAG_NUMFORNAME);
        ROUTE_IMPORT(Tag_NameForNum, G_TAG_NAMEFORNUM);
        ROUTE_IMPORT_8(Tag_Orientation, G_TAG_ORIENTATION, (orientation_t*), (int), (int), (int), (int), *(float*)&, (int*), (vec4_t*));
        ROUTE_IMPORT_18(Tag_OrientationEx, G_TAG_ORIENTATIONEX, (orientation_t*), (int), (int), (int), (int), *(float*)&, (int*), (vec4_t*), (int), (int), *(float*)&, (qboolean), (qboolean), (int), (int), (int), (int), *(float*)&);
        ROUTE_IMPORT(Bone_GetParentNum, G_BONE_GETPARENTNUM);
        ROUTE_IMPORT(Alias_Add, G_ALIAS_ADD);
        ROUTE_IMPORT(Alias_FindRandom, G_ALIAS_FINDRANDOM);
        ROUTE_IMPORT(Alias_Find, G_ALIAS_FIND);
        ROUTE_IMPORT(Alias_Dump, G_ALIAS_DUMP);
        ROUTE_IMPORT(Alias_Clear, G_ALIAS_CLEAR);
        ROUTE_IMPORT(Alias_FindDialog, G_ALIAS_FINDDIALOG);
        ROUTE_IMPORT(Alias_FindSpecificAnim, G_ALIAS_FINDSPECIFICANIM);
        ROUTE_IMPORT(Alias_CheckLoopAnim, G_ALIAS_CHECKLOOPANIM);
        ROUTE_IMPORT(Alias_GetList, G_ALIAS_GETLIST);
        ROUTE_IMPORT(Alias_UpdateDialog, G_ALIAS_UPDATEDIALOG);
        ROUTE_IMPORT(Alias_AddActorDialog, G_ALIAS_ADDACTORDIALOG);
        ROUTE_IMPORT(NameForNum, G_NAMEFORNUM);
        ROUTE_IMPORT(GlobalAlias_Add, G_GLOBALALIAS_ADD);
        ROUTE_IMPORT(GlobalAlias_FindRandom, G_GLOBALALIAS_FINDRANDOM);
        ROUTE_IMPORT(GlobalAlias_Find, G_GLOBALALIAS_FIND);
        ROUTE_IMPORT(GlobalAlias_Dump, G_GLOBALALIAS_DUMP);
        ROUTE_IMPORT(GlobalAlias_Clear, G_GLOBALALIAS_CLEAR);
        ROUTE_IMPORT(isClientActive, G_ISCLIENTACTIVE);
        ROUTE_IMPORT(centerprintf, G_CENTERPRINTF);
        ROUTE_IMPORT(locationprintf, G_LOCATIONPRINTF);
        ROUTE_IMPORT_8_V(Sound, G_SOUND, (vec3_t*), (int), (int), (const char*), *(float*)&, *(float*)&, *(float*)&, (qboolean));
        ROUTE_IMPORT(StopSound, G_STOPSOUND);
        ROUTE_IMPORT_1_F(SoundLength, G_SOUNDLENGTH, (const char*));
        ROUTE_IMPORT(GetNextMorphTarget, G_GETNEXTMORPHTARGET);
        ROUTE_IMPORT(CalcCRC, G_CALCCRC);
        ROUTE_IMPORT(LocateGameData, G_LOCATE_GAME_DATA);
        ROUTE_IMPORT(SetFarPlane, G_SETFARPLANE);
        ROUTE_IMPORT(TikiReload, G_TIKIRELOAD);
        ROUTE_IMPORT(TikiLoadFromTS, G_TIKILOADFROMTS);
        ROUTE_IMPORT(ToolServerGetData, G_TOOLSERVERGETDATA);
        ROUTE_IMPORT(SetSkyPortal, G_SETSKYPORTAL);
        ROUTE_IMPORT(WidgetPrintf, G_WIDGETPRINTF);
        ROUTE_IMPORT(ProcessLoadingScreen, G_PROCESSLOADINGSCREEN);
        ROUTE_IMPORT(MObjective_GetDescription, G_MOBJECTIVE_GETDESCRIPTION);
        ROUTE_IMPORT(MObjective_SetDescription, G_MOBJECTIVE_SETDESCRIPTION);
        ROUTE_IMPORT(MObjective_GetShowObjective, G_MOBJECTIVE_GETSHOWOBJECTIVE);
        ROUTE_IMPORT(MObjective_SetShowObjective, G_MOBJECTIVE_SETSHOWOBJECTIVE);
        ROUTE_IMPORT(MObjective_GetObjectiveComplete, G_MOBJECTIVE_GETOBJECTIVECOMPLETE);
        ROUTE_IMPORT(MObjective_SetObjectiveComplete, G_MOBJECTIVE_SETOBJECTIVECOMPLETE);
        ROUTE_IMPORT(MObjective_GetObjectiveFailed, G_MOBJECTIVE_GETOBJECTIVEFAILED);
        ROUTE_IMPORT(MObjective_SetObjectiveFailed, G_MOBJECTIVE_SETOBJECTIVEFAILED);
        ROUTE_IMPORT(MObjective_GetNameFromIndex, G_MOBJECTIVE_GETNAMEFROMINDEX);
        ROUTE_IMPORT(MObjective_GetIndexFromName, G_MOBJECTIVE_GETINDEXFROMNAME);
        ROUTE_IMPORT(MObjective_NewObjective, G_MOBJECTIVE_NEWOBJECTIVE);
        ROUTE_IMPORT(MObjective_ClearObjectiveList, G_MOBJECTIVE_CLEAROBJECTIVELIST);
        ROUTE_IMPORT(MObjective_ParseObjectiveFile, G_MOBJECTIVE_PARSEOBJECTIVEFILE);
        ROUTE_IMPORT(MObjective_Update, G_MOBJECTIVE_UPDATE);
        ROUTE_IMPORT(MObjective_GetNumObjectives, G_MOBJECTIVE_GETNUMOBJECTIVES);
        ROUTE_IMPORT(MObjective_GetNumActiveObjectives, G_MOBJECTIVE_GETNUMACTIVEOBJECTIVES);
        ROUTE_IMPORT(MObjective_GetNumCompleteObjectives, G_MOBJECTIVE_GETNUMCOMPLETEOBJECTIVES);
        ROUTE_IMPORT(MObjective_GetNumFailedObjectives, G_MOBJECTIVE_GETNUMFAILEDOBJECTIVES);
        ROUTE_IMPORT(MObjective_GetNumIncompleteObjectives, G_MOBJECTIVE_GETNUMINCOMPLETEOBJECTIVES);
        ROUTE_IMPORT(MI_GetShader, G_MI_GETSHADER);
        ROUTE_IMPORT(MI_SetShader, G_MI_SETSHADER);
        ROUTE_IMPORT(MI_GetInformationData, G_MI_GETINFORMATIONDATA);
        ROUTE_IMPORT(MI_SetInformationData, G_MI_SETINFORMATIONDATA);
        ROUTE_IMPORT(MI_GetNameFromIndex, G_MI_GETNAMEFROMINDEX);
        ROUTE_IMPORT(MI_GetIndexFromName, G_MI_GETINDEXFROMNAME);
        ROUTE_IMPORT(MI_NewInformation, G_MI_NEWINFORMATION);
        ROUTE_IMPORT(MI_ClearInformationList, G_MI_CLEARINFORMATIONLIST);
        ROUTE_IMPORT(MI_SetShowInformation, G_MI_SETSHOWINFORMATION);
        ROUTE_IMPORT(MI_GetShowInformation, G_MI_GETSHOWINFORMATION);
        ROUTE_IMPORT(SR_InitializeStringResource, G_SR_INITIALIZESTRINGRESOURCE);
        ROUTE_IMPORT(SR_UninitializeStringResource, G_SR_UNINITIALIZESTRINGRESOURCE);
        ROUTE_IMPORT(SR_LoadLevelStrings, G_SR_LOADLEVELSTRINGS);
        ROUTE_IMPORT(GetViewModeMask, G_GETVIEWMODEMASK);
        ROUTE_IMPORT(GetViewModeClassMask, G_GETVIEWMODECLASSMASK);
        ROUTE_IMPORT(GetViewModeSendInMode, G_GETVIEWMODESENDINMODE);
        ROUTE_IMPORT(GetViewModeSendNotInMode, G_GETVIEWMODESENDNOTINMODE);
        ROUTE_IMPORT(GetViewModeScreenBlend, G_GETVIEWMODESCREENBLEND);
        ROUTE_IMPORT(GetLevelDefs, G_GETLEVELDEFS);
        ROUTE_IMPORT(areSublevels, G_ARESUBLEVELS);
        ROUTE_IMPORT(SurfaceTypeToName, G_SURFACETYPETONAME);
        ROUTE_IMPORT(AAS_EntityInfo, G_AAS_ENTITYINFO);
        ROUTE_IMPORT(AAS_Initialized, G_AAS_INITIALIZED);
        ROUTE_IMPORT(AAS_PresenceTypeBoundingBox, G_AAS_PRESENCETYPEBOUNDINGBOX);
        ROUTE_IMPORT_0_F(AAS_Time, G_AAS_TIME);
        ROUTE_IMPORT(AAS_PointAreaNum, G_AAS_POINTAREANUM);
        ROUTE_IMPORT(AAS_PointReachabilityAreaIndex, G_AAS_POINTREACHABILITYAREAINDEX);
        ROUTE_IMPORT(AAS_TraceAreas, G_AAS_TRACEAREAS);
        ROUTE_IMPORT(AAS_BBoxAreas, G_AAS_BBOXAREAS);
        ROUTE_IMPORT(AAS_AreaInfo, G_AAS_AREAINFO);
        ROUTE_IMPORT(AAS_PointContents, G_AAS_POINTCONTENTS);
        ROUTE_IMPORT(AAS_NextBSPEntity, G_AAS_NEXTBSPENTITY);
        ROUTE_IMPORT(AAS_ValueForBSPEpairKey, G_AAS_VALUEFORBSPEPAIRKEY);
        ROUTE_IMPORT(AAS_VectorForBSPEpairKey, G_AAS_VECTORFORBSPEPAIRKEY);
        ROUTE_IMPORT(AAS_FloatForBSPEpairKey, G_AAS_FLOATFORBSPEPAIRKEY);
        ROUTE_IMPORT(AAS_IntForBSPEpairKey, G_AAS_INTFORBSPEPAIRKEY);
        ROUTE_IMPORT(AAS_AreaReachability, G_AAS_AREAREACHABILITY);
        ROUTE_IMPORT(AAS_AreaTravelTimeToGoalArea, G_AAS_AREATRAVELTIMETOGOALAREA);
        ROUTE_IMPORT(AAS_EnableRoutingArea, G_AAS_ENABLEROUTINGAREA);
        ROUTE_IMPORT(AAS_PredictRoute, G_AAS_PREDICTROUTE);
        ROUTE_IMPORT(AAS_AlternativeRouteGoals, G_AAS_ALTERNATIVEROUTEGOALS);
        ROUTE_IMPORT(AAS_Swimming, G_AAS_SWIMMING);
        ROUTE_IMPORT(AAS_PredictClientMovement, G_AAS_PREDICTCLIENTMOVEMENT);
        ROUTE_IMPORT(EA_Command, G_EA_COMMAND);
        ROUTE_IMPORT(EA_Say, G_EA_SAY);
        ROUTE_IMPORT(EA_SayTeam, G_EA_SAYTEAM);
        ROUTE_IMPORT(EA_Action, G_EA_ACTION);
        ROUTE_IMPORT(EA_Gesture, G_EA_GESTURE);
        ROUTE_IMPORT(EA_Talk, G_EA_TALK);
        ROUTE_IMPORT(EA_ToggleFireState, G_EA_TOGGLEFIRESTATE);
        ROUTE_IMPORT(EA_Attack, G_EA_ATTACK);
        ROUTE_IMPORT(EA_Use, G_EA_USE);
        ROUTE_IMPORT(EA_Respawn, G_EA_RESPAWN);
        ROUTE_IMPORT(EA_MoveUp, G_EA_MOVEUP);
        ROUTE_IMPORT(EA_MoveDown, G_EA_MOVEDOWN);
        ROUTE_IMPORT(EA_MoveForward, G_EA_MOVEFORWARD);
        ROUTE_IMPORT(EA_MoveBack, G_EA_MOVEBACK);
        ROUTE_IMPORT(EA_MoveLeft, G_EA_MOVELEFT);
        ROUTE_IMPORT(EA_MoveRight, G_EA_MOVERIGHT);
        ROUTE_IMPORT(EA_Crouch, G_EA_CROUCH);
        ROUTE_IMPORT(EA_SelectWeapon, G_EA_SELECTWEAPON);
        ROUTE_IMPORT(EA_Jump, G_EA_JUMP);
        ROUTE_IMPORT(EA_DelayedJump, G_EA_DELAYEDJUMP);
        ROUTE_IMPORT_3_V(EA_Move, G_EA_MOVE, (int), (float*), *(float*)&);
        ROUTE_IMPORT(EA_View, G_EA_VIEW);
        ROUTE_IMPORT_2_V(EA_EndRegular, G_EA_ENDREGULAR, (int), *(float*)&);
        ROUTE_IMPORT_3_V(EA_GetInput, G_EA_GETINPUT, (int), *(float*)&, (bot_input_t*));
        ROUTE_IMPORT(EA_ResetInput, G_EA_RESETINPUT);
        ROUTE_IMPORT_2(BotLoadCharacter, G_BOTLOADCHARACTER, (char*), *(float*)&);
        ROUTE_IMPORT(BotFreeCharacter, G_BOTFREECHARACTER);
        ROUTE_IMPORT(Characteristic_Float, G_CHARACTERISTIC_FLOAT);
        ROUTE_IMPORT_4_F(Characteristic_BFloat, G_CHARACTERISTIC_BFLOAT, (int), (int), *(float*)&, *(float*)&);
        ROUTE_IMPORT(Characteristic_Integer, G_CHARACTERISTIC_INTEGER);
        ROUTE_IMPORT(Characteristic_BInteger, G_CHARACTERISTIC_BINTEGER);
        ROUTE_IMPORT(Characteristic_String, G_CHARACTERISTIC_STRING);
        ROUTE_IMPORT(BotAllocChatState, G_BOTALLOCCHATSTATE);
        ROUTE_IMPORT(BotFreeChatState, G_BOTFREECHATSTATE);
        ROUTE_IMPORT(BotQueueConsoleMessage, G_BOTQUEUECONSOLEMESSAGE);
        ROUTE_IMPORT(BotRemoveConsoleMessage, G_BOTREMOVECONSOLEMESSAGE);
        ROUTE_IMPORT(BotNextConsoleMessage, G_BOTNEXTCONSOLEMESSAGE);
        ROUTE_IMPORT(BotNumConsoleMessages, G_BOTNUMCONSOLEMESSAGES);
        ROUTE_IMPORT(BotInitialChat, G_BOTINITIALCHAT);
        ROUTE_IMPORT(BotNumInitialChats, G_BOTNUMINITIALCHATS);
        ROUTE_IMPORT(BotReplyChat, G_BOTREPLYCHAT);
        ROUTE_IMPORT(BotChatLength, G_BOTCHATLENGTH);
        ROUTE_IMPORT(BotEnterChat, G_BOTENTERCHAT);
        ROUTE_IMPORT(BotGetChatMessage, G_BOTGETCHATMESSAGE);
        ROUTE_IMPORT(StringContains, G_STRINGCONTAINS);
        ROUTE_IMPORT(BotFindMatch, G_BOTFINDMATCH);
        ROUTE_IMPORT(BotMatchVariable, G_BOTMATCHVARIABLE);
        ROUTE_IMPORT(UnifyWhiteSpaces, G_UNIFYWHITESPACES);
        ROUTE_IMPORT(BotReplaceSynonyms, G_BOTREPLACESYNONYMS);
        ROUTE_IMPORT(BotLoadChatFile, G_BOTLOADCHATFILE);
        ROUTE_IMPORT(BotSetChatGender, G_BOTSETCHATGENDER);
        ROUTE_IMPORT(BotSetChatName, G_BOTSETCHATNAME);
        ROUTE_IMPORT(BotResetGoalState, G_BOTRESETGOALSTATE);
        ROUTE_IMPORT(BotResetAvoidGoals, G_BOTRESETAVOIDGOALS);
        ROUTE_IMPORT(BotRemoveFromAvoidGoals, G_BOTREMOVEFROMAVOIDGOALS);
        ROUTE_IMPORT(BotPushGoal, G_BOTPUSHGOAL);
        ROUTE_IMPORT(BotPopGoal, G_BOTPOPGOAL);
        ROUTE_IMPORT(BotEmptyGoalStack, G_BOTEMPTYGOALSTACK);
        ROUTE_IMPORT(BotDumpAvoidGoals, G_BOTDUMPAVOIDGOALS);
        ROUTE_IMPORT(BotDumpGoalStack, G_BOTDUMPGOALSTACK);
        ROUTE_IMPORT(BotGoalName, G_BOTGOALNAME);
        ROUTE_IMPORT(BotGetTopGoal, G_BOTGETTOPGOAL);
        ROUTE_IMPORT(BotGetSecondGoal, G_BOTGETSECONDGOAL);
        ROUTE_IMPORT(BotChooseLTGItem, G_BOTCHOOSELTGITEM);
        ROUTE_IMPORT_6(BotChooseNBGItem, G_BOTCHOOSENBGITEM, (int), (float*), (int*), (int), (struct bot_goal_s*), *(float*)&);
        ROUTE_IMPORT(BotTouchingGoal, G_BOTTOUCHINGGOAL);
        ROUTE_IMPORT(BotItemGoalInVisButNotVisible, G_BOTITEMGOALINVISBUTNOTVISIBLE);
        ROUTE_IMPORT(BotGetLevelItemGoal, G_BOTGETLEVELITEMGOAL);
        ROUTE_IMPORT(BotGetNextCampSpotGoal, G_BOTGETNEXTCAMPSPOTGOAL);
        ROUTE_IMPORT(BotGetMapLocationGoal, G_BOTGETMAPLOCATIONGOAL);
        ROUTE_IMPORT_2_F(BotAvoidGoalTime, G_BOTAVOIDGOALTIME, (int), (int));
        ROUTE_IMPORT_3_V(BotSetAvoidGoalTime, G_BOTSETAVOIDGOALTIME, (int), (int), *(float*)&);
        ROUTE_IMPORT(BotInitLevelItems, G_BOTINITLEVELITEMS);
        ROUTE_IMPORT(BotUpdateEntityItems, G_BOTUPDATEENTITYITEMS);
        ROUTE_IMPORT(BotLoadItemWeights, G_BOTLOADITEMWEIGHTS);
        ROUTE_IMPORT(BotFreeItemWeights, G_BOTFREEITEMWEIGHTS);
        ROUTE_IMPORT(BotInterbreedGoalFuzzyLogic, G_BOTINTERBREEDGOALFUZZYLOGIC);
        ROUTE_IMPORT(BotSaveGoalFuzzyLogic, G_BOTSAVEGOALFUZZYLOGIC);
        ROUTE_IMPORT_2_V(BotMutateGoalFuzzyLogic, G_BOTMUTATEGOALFUZZYLOGIC, (int), *(float*)&);
        ROUTE_IMPORT(BotAllocGoalState, G_BOTALLOCGOALSTATE);
        ROUTE_IMPORT(BotFreeGoalState, G_BOTFREEGOALSTATE);
        ROUTE_IMPORT(BotResetMoveState, G_BOTRESETMOVESTATE);
        ROUTE_IMPORT(BotMoveToGoal, G_BOTMOVETOGOAL);
        ROUTE_IMPORT_4(BotMoveInDirection, G_BOTMOVEINDIRECTION, (int), (float*), *(float*)&, (int));
        ROUTE_IMPORT(BotResetAvoidReach, G_BOTRESETAVOIDREACH);
        ROUTE_IMPORT(BotResetLastAvoidReach, G_BOTRESETLASTAVOIDREACH);
        ROUTE_IMPORT(BotReachabilityArea, G_BOTREACHABILITYAREA);
        ROUTE_IMPORT_5(BotMovementViewTarget, G_BOTMOVEMENTVIEWTARGET, (int), (struct bot_goal_s*), (int), *(float*)&, (float*));
        ROUTE_IMPORT(BotPredictVisiblePosition, G_BOTPREDICTVISIBLEPOSITION);
        ROUTE_IMPORT(BotAllocMoveState, G_BOTALLOCMOVESTATE);
        ROUTE_IMPORT(BotFreeMoveState, G_BOTFREEMOVESTATE);
        ROUTE_IMPORT(BotInitMoveState, G_BOTINITMOVESTATE);
        ROUTE_IMPORT_4_V(BotAddAvoidSpot, G_BOTADDAVOIDSPOT, (int), (float*), *(float*)&, (int));
        ROUTE_IMPORT(BotChooseBestFightWeapon, G_BOTCHOOSEBESTFIGHTWEAPON);
        ROUTE_IMPORT(BotGetWeaponInfo, G_BOTGETWEAPONINFO);
        ROUTE_IMPORT(BotLoadWeaponWeights, G_BOTLOADWEAPONWEIGHTS);
        ROUTE_IMPORT(BotAllocWeaponState, G_BOTALLOCWEAPONSTATE);
        ROUTE_IMPORT(BotFreeWeaponState, G_BOTFREEWEAPONSTATE);
        ROUTE_IMPORT(BotResetWeaponState, G_BOTRESETWEAPONSTATE);
        ROUTE_IMPORT(GeneticParentsAndChildSelection, G_GENETICPARENTSANDCHILDSELECTION);
        ROUTE_IMPORT(Print, G_BOTPRINT);
        ROUTE_IMPORT(PointContents, G_BOTPOINTCONTENTS);
        ROUTE_IMPORT(BSPEntityData, G_BSPENTITYDATA);
        ROUTE_IMPORT(BSPModelMinsMaxsOrigin, G_BSPMODELMINSMAXSORIGIN);
        ROUTE_IMPORT(BotClientCommand, G_BOTCLIENTCOMMAND);
        ROUTE_IMPORT(AvailableMemory, G_AVAILABLEMEMORY);
        ROUTE_IMPORT(HunkAlloc, G_HUNKALLOC);
        ROUTE_IMPORT(FS_FOpenFile, G_FS_FOPEN_FILE);
        ROUTE_IMPORT(FS_Seek, G_FS_SEEK);
        ROUTE_IMPORT(DebugLineCreate, G_DEBUGLINECREATE);
        ROUTE_IMPORT(DebugLineDelete, G_DEBUGLINEDELETE);
        ROUTE_IMPORT(DebugLineShow, G_DEBUGLINESHOW);
        ROUTE_IMPORT(DebugPolygonCreate, G_DEBUGPOLYGONCREATE);
        ROUTE_IMPORT(DebugPolygonDelete, G_DEBUGPOLYGONDELETE);
        ROUTE_IMPORT(DropClient, G_DROP_CLIENT);
        ROUTE_IMPORT(SV_GetServerinfo, G_SV_GETSERVERINFO);
        ROUTE_IMPORT(BotAllocateClient, G_BOTALLOCATECLIENT);
        ROUTE_IMPORT(BotGetSnapshotEntity, G_BOTGETSNAPSHOTENTITY);
        ROUTE_IMPORT(BotGetConsoleMessage, G_BOTGETCONSOLEMESSAGE);
        ROUTE_IMPORT(BotLibSetup, G_BOTLIBSETUP);
        ROUTE_IMPORT(BotLibShutdown, G_BOTLIBSHUTDOWN);
        ROUTE_IMPORT(BotLibVarSet, G_BOTLIBVARSET);
        ROUTE_IMPORT(BotLibVarGet, G_BOTLIBVARGET);
        ROUTE_IMPORT(PC_AddGlobalDefine, G_PC_ADDGLOBALDEFINE);
        ROUTE_IMPORT(PC_LoadSourceHandle, G_PC_LOADSOURCEHANDLE);
        ROUTE_IMPORT(PC_FreeSourceHandle, G_PC_FREESOURCEHANDLE);
        ROUTE_IMPORT(PC_SourceFileAndLine, G_PC_SOURCEFILEANDLINE);
        ROUTE_IMPORT_1(BotLibStartFrame, G_BOTLIBSTARTFRAME, *(float*)&);
        ROUTE_IMPORT(BotLibLoadMap, G_BOTLIBLOADMAP);
        ROUTE_IMPORT(BotLibUpdateEntity, G_BOTLIBUPDATEENTITY);
        ROUTE_IMPORT(Test, G_TEST);
        ROUTE_IMPORT(BotUserCommand, G_BOTUSERCOMMAND);

        // handle cmds for variables, this is how a plugin would get these values if needed
        ROUTE_IMPORT_VAR(DebugLines, GVP_DEBUGLINES);
        ROUTE_IMPORT_VAR(numDebugLines, GVP_NUMDEBUGLINES);

    // handle special cmds which QMM uses but STEF2 doesn't have an analogue for
    case G_SEND_CONSOLE_COMMAND_QMM:
    case G_SEND_CONSOLE_COMMAND: {
        // STEF2: void (*SendConsoleCommand)(const char *text);
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
    case G_GET_ENTITY_TOKEN: {
        // qboolean trap_GetEntityToken(char *buffer, int bufferSize);
        if (token_counter >= entity_tokens.size()) {
            ret = qfalse;
            break;
        }

        char* buffer = (char*)args[0];
        intptr_t bufferSize = args[1];

        strncpyz(buffer, entity_tokens[token_counter++].c_str(), (size_t)bufferSize);
        ret = qtrue;
        break;
    }

    default:
        break;
    };

    // do anything that needs to be done after function call here

    if (cmd != G_PRINT)
        QMMLOG(QMM_LOG_TRACE, "QMM") << "STEF2_GameSupport::syscall(" << EngMsgName(cmd) << "(" << cmd << ")) reutrning " << ret << "\n";


    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t STEF2_GameSupport::vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

    QMMLOG(QMM_LOG_TRACE, "QMM") << "STEF2_GameSupport::vmMain(" << ModMsgName(cmd) << "(" << cmd << ")) called\n";

    if (!orig_export)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    switch (cmd) {
        ROUTE_EXPORT(Init, GAME_INIT);
        ROUTE_EXPORT(Shutdown, GAME_SHUTDOWN);
        ROUTE_EXPORT(Cleanup, GAME_CLEANUP);
        ROUTE_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES);
        ROUTE_EXPORT(PostLoad, GAME_POSTLOAD);
        ROUTE_EXPORT(PostSublevelLoad, GAME_POSTSUBLEVELLOAD);
        ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
        ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
        ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
        ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
        ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
        ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
        ROUTE_EXPORT(PrepFrame, GAME_PREP_FRAME);
        ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
        ROUTE_EXPORT(SendEntity, GAME_SEND_ENTITY);
        ROUTE_EXPORT(UpdateEntityStateForClient, GAME_UPDATE_ENTITYSTATE_FOR_CLIENT);
        ROUTE_EXPORT(UpdatePlayerStateForClient, GAME_UPDATE_PLAYERSTATE_FOR_CLIENT);
        ROUTE_EXPORT(ExtraEntitiesToSend, GAME_EXTRA_ENTITIES_TO_SEND);
        ROUTE_EXPORT(GetEntityCurrentAnimFrame, GAME_GETENTITY_CURRENT_ANIMFRAME);
        ROUTE_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND);
        ROUTE_EXPORT(WritePersistant, GAME_WRITE_PERSISTANT);
        ROUTE_EXPORT(ReadPersistant, GAME_READ_PERSISTANT);
        ROUTE_EXPORT(WriteLevel, GAME_WRITE_LEVEL);
        ROUTE_EXPORT(ReadLevel, GAME_READ_LEVEL);
        ROUTE_EXPORT(LevelArchiveValid, GAME_LEVEL_ARCHIVE_VALID);
        ROUTE_EXPORT(inMultiplayerGame, GAME_IN_MULTIPLAYER_GAME);
        ROUTE_EXPORT(isDefined, GAME_IS_DEFINED);
        ROUTE_EXPORT(getDefine, GAME_GET_DEFINE);
        ROUTE_EXPORT(BotAIStartFrame, BOTAI_START_FRAME);
        ROUTE_EXPORT(AddBot_f, GAME_ADDBOT_F);
        ROUTE_EXPORT(GetTotalGameFrames, GAME_GETTOTALGAMEFRAMES);

        // handle cmds for variables, this is how a plugin would get these values if needed
        ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
        ROUTE_EXPORT_VAR(gentities, GAMEVP_GENTITIES);
        ROUTE_EXPORT_VAR(gentitySize, GAMEV_GENTITYSIZE);
        ROUTE_EXPORT_VAR(num_entities, GAMEV_NUM_ENTITIES);
        ROUTE_EXPORT_VAR(max_entities, GAMEV_MAX_ENTITIES);
        ROUTE_EXPORT_VAR(error_message, GAMEVP_ERRORMESSAGE);

    default:
        break;
    };

    // update export vars after returning from the mod
    update_exports();

    QMMLOG(QMM_LOG_TRACE, "QMM") << "STEF2_GameSupport::vmMain(" << ModMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";

    return ret;
}


void* STEF2_GameSupport::Entry(void* import, void*, APIType) {
    QMMLOG(QMM_LOG_DEBUG, "QMM") << "STEF2_GameSupport::Entry(" << import << ") called\n";

    // original import struct from engine
    // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
    game_import_t* gi = (game_import_t*)import;
    orig_import = *gi;

    // fill in variables of our hooked import struct to pass to the mod
    qmm_import.DebugLines = orig_import.DebugLines;
    qmm_import.numDebugLines = orig_import.numDebugLines;

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "STEF2_GameSupport::Entry(" << import << ") returning " << &qmm_export << "\n";

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // the only thing in this struct the engine uses before calling Init is the apiversion
    return &qmm_export;
}


bool STEF2_GameSupport::ModLoad(void* entry, APIType modapi) {
    if (modapi != QMM_API_GETGAMEAPI)
        return false;

    mod_GetGameAPI pfnGGA = (mod_GetGameAPI)entry;
    orig_export = (game_export_t*)pfnGGA(&qmm_import, nullptr);

    return !!orig_export;
}

void STEF2_GameSupport::ModUnload() {
    orig_export = nullptr;
}


const char* STEF2_GameSupport::EngMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_PRINTF);
        GEN_CASE(G_DPRINTF);
        GEN_CASE(G_WPRINTF);
        GEN_CASE(G_WDPRINTF);
        GEN_CASE(G_DEBUGPRINTF);
        GEN_CASE(G_LOCALIZEFILEPATH);
        GEN_CASE(G_ERROR);
        GEN_CASE(G_MILLISECONDS);
        GEN_CASE(G_MALLOC);
        GEN_CASE(G_FREE);
        GEN_CASE(G_CVAR);
        GEN_CASE(G_CVAR_GET);
        GEN_CASE(G_CVAR_SET);
        GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
        GEN_CASE(G_CVAR_REGISTER);
        GEN_CASE(G_CVAR_VARIABLEVALUE);
        GEN_CASE(G_CVAR_UPDATE);
        GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(G_ARGC);
        GEN_CASE(G_ARGV);
        GEN_CASE(G_ARGS);
        GEN_CASE(G_ADDCOMMAND);
        GEN_CASE(G_FS_READFILE);
        GEN_CASE(G_FS_EXISTS);
        GEN_CASE(G_FS_FREEFILE);
        GEN_CASE(G_FS_WRITEFILE);
        GEN_CASE(G_FS_FOPEN_FILE_WRITE);
        GEN_CASE(G_FS_FOPEN_FILE_APPEND);
        GEN_CASE(G_FS_LISTFILES);
        GEN_CASE(G_FS_PREPFILEWRITE);
        GEN_CASE(G_FS_WRITE);
        GEN_CASE(G_FS_READ);
        GEN_CASE(G_FS_FCLOSE_FILE);
        GEN_CASE(G_FS_FTELL);
        GEN_CASE(G_FS_FSEEK);
        GEN_CASE(G_FS_FLUSH);
        GEN_CASE(G_FS_DELETEFILE);
        GEN_CASE(G_FS_GETFILELIST);
        GEN_CASE(G_GETARCHIVEFILENAME);
        GEN_CASE(G_SEND_CONSOLE_COMMAND);
        GEN_CASE(G_DEBUGGRAPH);
        GEN_CASE(G_SEND_SERVER_COMMAND);
        GEN_CASE(G_GETNUMFREERELIABLESERVERCOMMANDS);
        GEN_CASE(G_SET_CONFIGSTRING);
        GEN_CASE(G_GET_CONFIGSTRING);
        GEN_CASE(G_SET_USERINFO);
        GEN_CASE(G_GET_USERINFO);
        GEN_CASE(G_SET_BRUSH_MODEL);
        GEN_CASE(G_TRACE);
        GEN_CASE(G_FULLTRACE);
        GEN_CASE(G_POINT_CONTENTS);
        GEN_CASE(G_POINTBRUSHNUM);
        GEN_CASE(G_IN_PVS);
        GEN_CASE(G_IN_PVS_IGNOREPORTALS);
        GEN_CASE(G_ADJUSTAREAPORTALSTATE);
        GEN_CASE(G_AREAS_CONNECTED);
        GEN_CASE(G_GETLIGHTINGGROUP);
        GEN_CASE(G_SETDYNAMICLIGHT);
        GEN_CASE(G_SETDYNAMICLIGHTDEFAULT);
        GEN_CASE(G_SETWINDDIRECTION);
        GEN_CASE(G_SETWINDINTENSITY);
        GEN_CASE(G_SETWEATHERINFO);
        GEN_CASE(G_SETTIMESCALE);
        GEN_CASE(G_LINKENTITY);
        GEN_CASE(G_UNLINKENTITY);
        GEN_CASE(G_AREAENTITIES);
        GEN_CASE(G_CLIPTOENTITY);
        GEN_CASE(G_OBJECTIVENAMEINDEX);
        GEN_CASE(G_ARCHETYPEINDEX);
        GEN_CASE(G_IMAGEINDEX);
        GEN_CASE(G_FAILEDCONDITION);
        GEN_CASE(G_ITEMINDEX);
        GEN_CASE(G_SOUNDINDEX);
        GEN_CASE(G_MODELINDEX);
        GEN_CASE(G_SETLIGHTSTYLE);
        GEN_CASE(G_GAMEDIR);
        GEN_CASE(G_ISMODEL);
        GEN_CASE(G_SETMODEL);
        GEN_CASE(G_SETVIEWMODEL);
        GEN_CASE(G_NUMANIMS);
        GEN_CASE(G_NUMSKINS);
        GEN_CASE(G_NUMSURFACES);
        GEN_CASE(G_NUMTAGS);
        GEN_CASE(G_NUMMORPHS);
        GEN_CASE(G_INITCOMMANDS);
        GEN_CASE(G_CALCULATEBOUNDS);
        GEN_CASE(G_TIKI_CACHEANIM);
        GEN_CASE(G_ANIM_NAMEFORNUM);
        GEN_CASE(G_ANIM_NUMFORNAME);
        GEN_CASE(G_ANIM_RANDOM);
        GEN_CASE(G_ANIM_NUMFRAMES);
        GEN_CASE(G_ANIM_TIME);
        GEN_CASE(G_ANIM_DELTA);
        GEN_CASE(G_ANIM_ABSOLUTEDELTA);
        GEN_CASE(G_ANIM_FLAGS);
        GEN_CASE(G_ANIM_HASCOMMANDS);
        GEN_CASE(G_FRAME_COMMANDS);
        GEN_CASE(G_FRAME_DELTA);
        GEN_CASE(G_FRAME_TIME);
        GEN_CASE(G_FRAME_BOUNDS);
        GEN_CASE(G_SURFACE_NAMETONUM);
        GEN_CASE(G_SURFACE_NUMTONAME);
        GEN_CASE(G_SURFACE_FLAGS);
        GEN_CASE(G_SURFACE_NUMSKINS);
        GEN_CASE(G_MORPH_NUMFORNAME);
        GEN_CASE(G_MORPH_NAMEFORNUM);
        GEN_CASE(G_GETEXPRESSION);
        GEN_CASE(G_TAG_NUMFORNAME);
        GEN_CASE(G_TAG_NAMEFORNUM);
        GEN_CASE(G_TAG_ORIENTATION);
        GEN_CASE(G_TAG_ORIENTATIONEX);
        GEN_CASE(G_BONE_GETPARENTNUM);
        GEN_CASE(G_ALIAS_ADD);
        GEN_CASE(G_ALIAS_FINDRANDOM);
        GEN_CASE(G_ALIAS_FIND);
        GEN_CASE(G_ALIAS_DUMP);
        GEN_CASE(G_ALIAS_CLEAR);
        GEN_CASE(G_ALIAS_FINDDIALOG);
        GEN_CASE(G_ALIAS_FINDSPECIFICANIM);
        GEN_CASE(G_ALIAS_CHECKLOOPANIM);
        GEN_CASE(G_ALIAS_GETLIST);
        GEN_CASE(G_ALIAS_UPDATEDIALOG);
        GEN_CASE(G_ALIAS_ADDACTORDIALOG);
        GEN_CASE(G_NAMEFORNUM);
        GEN_CASE(G_GLOBALALIAS_ADD);
        GEN_CASE(G_GLOBALALIAS_FINDRANDOM);
        GEN_CASE(G_GLOBALALIAS_FIND);
        GEN_CASE(G_GLOBALALIAS_DUMP);
        GEN_CASE(G_GLOBALALIAS_CLEAR);
        GEN_CASE(G_ISCLIENTACTIVE);
        GEN_CASE(G_CENTERPRINTF);
        GEN_CASE(G_LOCATIONPRINTF);
        GEN_CASE(G_SOUND);
        GEN_CASE(G_STOPSOUND);
        GEN_CASE(G_SOUNDLENGTH);
        GEN_CASE(G_GETNEXTMORPHTARGET);
        GEN_CASE(G_CALCCRC);
        GEN_CASE(GVP_DEBUGLINES);
        GEN_CASE(GVP_NUMDEBUGLINES);
        GEN_CASE(G_LOCATE_GAME_DATA);
        GEN_CASE(G_SETFARPLANE);
        GEN_CASE(G_TIKIRELOAD);
        GEN_CASE(G_TIKILOADFROMTS);
        GEN_CASE(G_TOOLSERVERGETDATA);
        GEN_CASE(G_SETSKYPORTAL);
        GEN_CASE(G_WIDGETPRINTF);
        GEN_CASE(G_PROCESSLOADINGSCREEN);
        GEN_CASE(G_MOBJECTIVE_GETDESCRIPTION);
        GEN_CASE(G_MOBJECTIVE_SETDESCRIPTION);
        GEN_CASE(G_MOBJECTIVE_GETSHOWOBJECTIVE);
        GEN_CASE(G_MOBJECTIVE_SETSHOWOBJECTIVE);
        GEN_CASE(G_MOBJECTIVE_GETOBJECTIVECOMPLETE);
        GEN_CASE(G_MOBJECTIVE_SETOBJECTIVECOMPLETE);
        GEN_CASE(G_MOBJECTIVE_GETOBJECTIVEFAILED);
        GEN_CASE(G_MOBJECTIVE_SETOBJECTIVEFAILED);
        GEN_CASE(G_MOBJECTIVE_GETNAMEFROMINDEX);
        GEN_CASE(G_MOBJECTIVE_GETINDEXFROMNAME);
        GEN_CASE(G_MOBJECTIVE_NEWOBJECTIVE);
        GEN_CASE(G_MOBJECTIVE_CLEAROBJECTIVELIST);
        GEN_CASE(G_MOBJECTIVE_PARSEOBJECTIVEFILE);
        GEN_CASE(G_MOBJECTIVE_UPDATE);
        GEN_CASE(G_MOBJECTIVE_GETNUMOBJECTIVES);
        GEN_CASE(G_MOBJECTIVE_GETNUMACTIVEOBJECTIVES);
        GEN_CASE(G_MOBJECTIVE_GETNUMCOMPLETEOBJECTIVES);
        GEN_CASE(G_MOBJECTIVE_GETNUMFAILEDOBJECTIVES);
        GEN_CASE(G_MOBJECTIVE_GETNUMINCOMPLETEOBJECTIVES);
        GEN_CASE(G_MI_GETSHADER);
        GEN_CASE(G_MI_SETSHADER);
        GEN_CASE(G_MI_GETINFORMATIONDATA);
        GEN_CASE(G_MI_SETINFORMATIONDATA);
        GEN_CASE(G_MI_GETNAMEFROMINDEX);
        GEN_CASE(G_MI_GETINDEXFROMNAME);
        GEN_CASE(G_MI_NEWINFORMATION);
        GEN_CASE(G_MI_CLEARINFORMATIONLIST);
        GEN_CASE(G_MI_SETSHOWINFORMATION);
        GEN_CASE(G_MI_GETSHOWINFORMATION);
        GEN_CASE(G_SR_INITIALIZESTRINGRESOURCE);
        GEN_CASE(G_SR_UNINITIALIZESTRINGRESOURCE);
        GEN_CASE(G_SR_LOADLEVELSTRINGS);
        GEN_CASE(G_GETVIEWMODEMASK);
        GEN_CASE(G_GETVIEWMODECLASSMASK);
        GEN_CASE(G_GETVIEWMODESENDINMODE);
        GEN_CASE(G_GETVIEWMODESENDNOTINMODE);
        GEN_CASE(G_GETVIEWMODESCREENBLEND);
        GEN_CASE(G_GETLEVELDEFS);
        GEN_CASE(G_ARESUBLEVELS);
        GEN_CASE(G_SURFACETYPETONAME);
        GEN_CASE(G_AAS_ENTITYINFO);
        GEN_CASE(G_AAS_INITIALIZED);
        GEN_CASE(G_AAS_PRESENCETYPEBOUNDINGBOX);
        GEN_CASE(G_AAS_TIME);
        GEN_CASE(G_AAS_POINTAREANUM);
        GEN_CASE(G_AAS_POINTREACHABILITYAREAINDEX);
        GEN_CASE(G_AAS_TRACEAREAS);
        GEN_CASE(G_AAS_BBOXAREAS);
        GEN_CASE(G_AAS_AREAINFO);
        GEN_CASE(G_AAS_POINTCONTENTS);
        GEN_CASE(G_AAS_NEXTBSPENTITY);
        GEN_CASE(G_AAS_VALUEFORBSPEPAIRKEY);
        GEN_CASE(G_AAS_VECTORFORBSPEPAIRKEY);
        GEN_CASE(G_AAS_FLOATFORBSPEPAIRKEY);
        GEN_CASE(G_AAS_INTFORBSPEPAIRKEY);
        GEN_CASE(G_AAS_AREAREACHABILITY);
        GEN_CASE(G_AAS_AREATRAVELTIMETOGOALAREA);
        GEN_CASE(G_AAS_ENABLEROUTINGAREA);
        GEN_CASE(G_AAS_PREDICTROUTE);
        GEN_CASE(G_AAS_ALTERNATIVEROUTEGOALS);
        GEN_CASE(G_AAS_SWIMMING);
        GEN_CASE(G_AAS_PREDICTCLIENTMOVEMENT);
        GEN_CASE(G_EA_COMMAND);
        GEN_CASE(G_EA_SAY);
        GEN_CASE(G_EA_SAYTEAM);
        GEN_CASE(G_EA_ACTION);
        GEN_CASE(G_EA_GESTURE);
        GEN_CASE(G_EA_TALK);
        GEN_CASE(G_EA_TOGGLEFIRESTATE);
        GEN_CASE(G_EA_ATTACK);
        GEN_CASE(G_EA_USE);
        GEN_CASE(G_EA_RESPAWN);
        GEN_CASE(G_EA_MOVEUP);
        GEN_CASE(G_EA_MOVEDOWN);
        GEN_CASE(G_EA_MOVEFORWARD);
        GEN_CASE(G_EA_MOVEBACK);
        GEN_CASE(G_EA_MOVELEFT);
        GEN_CASE(G_EA_MOVERIGHT);
        GEN_CASE(G_EA_CROUCH);
        GEN_CASE(G_EA_SELECTWEAPON);
        GEN_CASE(G_EA_JUMP);
        GEN_CASE(G_EA_DELAYEDJUMP);
        GEN_CASE(G_EA_MOVE);
        GEN_CASE(G_EA_VIEW);
        GEN_CASE(G_EA_ENDREGULAR);
        GEN_CASE(G_EA_GETINPUT);
        GEN_CASE(G_EA_RESETINPUT);
        GEN_CASE(G_BOTLOADCHARACTER);
        GEN_CASE(G_BOTFREECHARACTER);
        GEN_CASE(G_CHARACTERISTIC_FLOAT);
        GEN_CASE(G_CHARACTERISTIC_BFLOAT);
        GEN_CASE(G_CHARACTERISTIC_INTEGER);
        GEN_CASE(G_CHARACTERISTIC_BINTEGER);
        GEN_CASE(G_CHARACTERISTIC_STRING);
        GEN_CASE(G_BOTALLOCCHATSTATE);
        GEN_CASE(G_BOTFREECHATSTATE);
        GEN_CASE(G_BOTQUEUECONSOLEMESSAGE);
        GEN_CASE(G_BOTREMOVECONSOLEMESSAGE);
        GEN_CASE(G_BOTNEXTCONSOLEMESSAGE);
        GEN_CASE(G_BOTNUMCONSOLEMESSAGES);
        GEN_CASE(G_BOTINITIALCHAT);
        GEN_CASE(G_BOTNUMINITIALCHATS);
        GEN_CASE(G_BOTREPLYCHAT);
        GEN_CASE(G_BOTCHATLENGTH);
        GEN_CASE(G_BOTENTERCHAT);
        GEN_CASE(G_BOTGETCHATMESSAGE);
        GEN_CASE(G_STRINGCONTAINS);
        GEN_CASE(G_BOTFINDMATCH);
        GEN_CASE(G_BOTMATCHVARIABLE);
        GEN_CASE(G_UNIFYWHITESPACES);
        GEN_CASE(G_BOTREPLACESYNONYMS);
        GEN_CASE(G_BOTLOADCHATFILE);
        GEN_CASE(G_BOTSETCHATGENDER);
        GEN_CASE(G_BOTSETCHATNAME);
        GEN_CASE(G_BOTRESETGOALSTATE);
        GEN_CASE(G_BOTRESETAVOIDGOALS);
        GEN_CASE(G_BOTREMOVEFROMAVOIDGOALS);
        GEN_CASE(G_BOTPUSHGOAL);
        GEN_CASE(G_BOTPOPGOAL);
        GEN_CASE(G_BOTEMPTYGOALSTACK);
        GEN_CASE(G_BOTDUMPAVOIDGOALS);
        GEN_CASE(G_BOTDUMPGOALSTACK);
        GEN_CASE(G_BOTGOALNAME);
        GEN_CASE(G_BOTGETTOPGOAL);
        GEN_CASE(G_BOTGETSECONDGOAL);
        GEN_CASE(G_BOTCHOOSELTGITEM);
        GEN_CASE(G_BOTCHOOSENBGITEM);
        GEN_CASE(G_BOTTOUCHINGGOAL);
        GEN_CASE(G_BOTITEMGOALINVISBUTNOTVISIBLE);
        GEN_CASE(G_BOTGETLEVELITEMGOAL);
        GEN_CASE(G_BOTGETNEXTCAMPSPOTGOAL);
        GEN_CASE(G_BOTGETMAPLOCATIONGOAL);
        GEN_CASE(G_BOTAVOIDGOALTIME);
        GEN_CASE(G_BOTSETAVOIDGOALTIME);
        GEN_CASE(G_BOTINITLEVELITEMS);
        GEN_CASE(G_BOTUPDATEENTITYITEMS);
        GEN_CASE(G_BOTLOADITEMWEIGHTS);
        GEN_CASE(G_BOTFREEITEMWEIGHTS);
        GEN_CASE(G_BOTINTERBREEDGOALFUZZYLOGIC);
        GEN_CASE(G_BOTSAVEGOALFUZZYLOGIC);
        GEN_CASE(G_BOTMUTATEGOALFUZZYLOGIC);
        GEN_CASE(G_BOTALLOCGOALSTATE);
        GEN_CASE(G_BOTFREEGOALSTATE);
        GEN_CASE(G_BOTRESETMOVESTATE);
        GEN_CASE(G_BOTMOVETOGOAL);
        GEN_CASE(G_BOTMOVEINDIRECTION);
        GEN_CASE(G_BOTRESETAVOIDREACH);
        GEN_CASE(G_BOTRESETLASTAVOIDREACH);
        GEN_CASE(G_BOTREACHABILITYAREA);
        GEN_CASE(G_BOTMOVEMENTVIEWTARGET);
        GEN_CASE(G_BOTPREDICTVISIBLEPOSITION);
        GEN_CASE(G_BOTALLOCMOVESTATE);
        GEN_CASE(G_BOTFREEMOVESTATE);
        GEN_CASE(G_BOTINITMOVESTATE);
        GEN_CASE(G_BOTADDAVOIDSPOT);
        GEN_CASE(G_BOTCHOOSEBESTFIGHTWEAPON);
        GEN_CASE(G_BOTGETWEAPONINFO);
        GEN_CASE(G_BOTLOADWEAPONWEIGHTS);
        GEN_CASE(G_BOTALLOCWEAPONSTATE);
        GEN_CASE(G_BOTFREEWEAPONSTATE);
        GEN_CASE(G_BOTRESETWEAPONSTATE);
        GEN_CASE(G_GENETICPARENTSANDCHILDSELECTION);
        GEN_CASE(G_BOTPRINT);
        GEN_CASE(G_BOTPOINTCONTENTS);
        GEN_CASE(G_BSPENTITYDATA);
        GEN_CASE(G_BSPMODELMINSMAXSORIGIN);
        GEN_CASE(G_BOTCLIENTCOMMAND);
        GEN_CASE(G_AVAILABLEMEMORY);
        GEN_CASE(G_HUNKALLOC);
        GEN_CASE(G_FS_FOPEN_FILE);
        GEN_CASE(G_FS_SEEK);
        GEN_CASE(G_DEBUGLINECREATE);
        GEN_CASE(G_DEBUGLINEDELETE);
        GEN_CASE(G_DEBUGLINESHOW);
        GEN_CASE(G_DEBUGPOLYGONCREATE);
        GEN_CASE(G_DEBUGPOLYGONDELETE);
        GEN_CASE(G_DROP_CLIENT);
        GEN_CASE(G_SV_GETSERVERINFO);
        GEN_CASE(G_BOTALLOCATECLIENT);
        GEN_CASE(G_BOTGETSNAPSHOTENTITY);
        GEN_CASE(G_BOTGETCONSOLEMESSAGE);
        GEN_CASE(G_BOTLIBSETUP);
        GEN_CASE(G_BOTLIBSHUTDOWN);
        GEN_CASE(G_BOTLIBVARSET);
        GEN_CASE(G_BOTLIBVARGET);
        GEN_CASE(G_PC_ADDGLOBALDEFINE);
        GEN_CASE(G_PC_LOADSOURCEHANDLE);
        GEN_CASE(G_PC_FREESOURCEHANDLE);
        GEN_CASE(G_PC_SOURCEFILEANDLINE);
        GEN_CASE(G_BOTLIBSTARTFRAME);
        GEN_CASE(G_BOTLIBLOADMAP);
        GEN_CASE(G_BOTLIBUPDATEENTITY);
        GEN_CASE(G_TEST);
        GEN_CASE(G_BOTUSERCOMMAND);

        // polyfills
        GEN_CASE(G_GET_ENTITY_TOKEN);

    default:
        return "unknown";
    }
}


const char* STEF2_GameSupport::ModMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(GAMEV_APIVERSION);
        GEN_CASE(GAME_INIT);
        GEN_CASE(GAME_SHUTDOWN);
        GEN_CASE(GAME_CLEANUP);
        GEN_CASE(GAME_SPAWN_ENTITIES);
        GEN_CASE(GAME_POSTLOAD);
        GEN_CASE(GAME_POSTSUBLEVELLOAD);
        GEN_CASE(GAME_CLIENT_CONNECT);
        GEN_CASE(GAME_CLIENT_BEGIN);
        GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
        GEN_CASE(GAME_CLIENT_DISCONNECT);
        GEN_CASE(GAME_CLIENT_COMMAND);
        GEN_CASE(GAME_CLIENT_THINK);
        GEN_CASE(GAME_PREP_FRAME);
        GEN_CASE(GAME_RUN_FRAME);
        GEN_CASE(GAME_SEND_ENTITY);
        GEN_CASE(GAME_UPDATE_ENTITYSTATE_FOR_CLIENT);
        GEN_CASE(GAME_UPDATE_PLAYERSTATE_FOR_CLIENT);
        GEN_CASE(GAME_EXTRA_ENTITIES_TO_SEND);
        GEN_CASE(GAME_GETENTITY_CURRENT_ANIMFRAME);
        GEN_CASE(GAME_CONSOLE_COMMAND);
        GEN_CASE(GAME_WRITE_PERSISTANT);
        GEN_CASE(GAME_READ_PERSISTANT);
        GEN_CASE(GAME_WRITE_LEVEL);
        GEN_CASE(GAME_READ_LEVEL);
        GEN_CASE(GAME_LEVEL_ARCHIVE_VALID);
        GEN_CASE(GAME_IN_MULTIPLAYER_GAME);
        GEN_CASE(GAME_IS_DEFINED);
        GEN_CASE(GAME_GET_DEFINE);
        GEN_CASE(BOTAI_START_FRAME);
        GEN_CASE(GAME_ADDBOT_F);
        GEN_CASE(GAME_GETTOTALGAMEFRAMES);
        GEN_CASE(GAMEVP_GENTITIES);
        GEN_CASE(GAMEV_GENTITYSIZE);
        GEN_CASE(GAMEV_NUM_ENTITIES);
        GEN_CASE(GAMEV_MAX_ENTITIES);
        GEN_CASE(GAMEVP_ERRORMESSAGE);
    default:
        return "unknown";
    }
}


void STEF2_GameSupport::update_exports() {
    if (!orig_export)
        return;

    bool changed = false;

    // if entity data changed, we need to send a G_LOCATE_GAME_DATA so plugins can hook it
    if (qmm_export.gentities != orig_export->gentities
        || qmm_export.gentitySize != orig_export->gentitySize
        || qmm_export.num_entities != orig_export->num_entities
        ) {
        changed = true;
    }

    qmm_export.gentities = orig_export->gentities;
    qmm_export.gentitySize = orig_export->gentitySize;
    qmm_export.num_entities = orig_export->num_entities;
    qmm_export.max_entities = orig_export->max_entities;
    qmm_export.error_message = orig_export->error_message;

    if (changed) {
        // this will trigger this message to be fired to plugins, and then it will be handled
        // by the empty "case G_LOCATE_GAME_DATA" in MOHAA_syscall
        qmm_syscall(G_LOCATE_GAME_DATA, (intptr_t)qmm_export.gentities, qmm_export.num_entities, qmm_export.gentitySize, nullptr, 0);
    }
}


game_import_t STEF2_GameSupport::orig_import;


game_export_t* STEF2_GameSupport::orig_export = nullptr;


game_import_t STEF2_GameSupport::qmm_import = {
    GEN_IMPORT(Printf, G_PRINTF),
    GEN_IMPORT(DPrintf, G_DPRINTF),
    GEN_IMPORT(WPrintf, G_WPRINTF),
    GEN_IMPORT(WDPrintf, G_WDPRINTF),
    GEN_IMPORT(DebugPrintf, G_DEBUGPRINTF),
    GEN_IMPORT(LocalizeFilePath, G_LOCALIZEFILEPATH),
    GEN_IMPORT(Error, G_ERROR),
    GEN_IMPORT(Milliseconds, G_MILLISECONDS),
    GEN_IMPORT(Malloc, G_MALLOC),
    GEN_IMPORT(Free, G_FREE),
    GEN_IMPORT(cvar, G_CVAR),
    GEN_IMPORT(cvar_get, G_CVAR_GET),
    GEN_IMPORT(cvar_set, G_CVAR_SET),
    GEN_IMPORT(Cvar_VariableStringBuffer, G_CVAR_VARIABLE_STRING_BUFFER),
    GEN_IMPORT(Cvar_Register, G_CVAR_REGISTER),
    GEN_IMPORT_1_F(Cvar_VariableValue, G_CVAR_VARIABLEVALUE, const char*),
    GEN_IMPORT(Cvar_Update, G_CVAR_UPDATE),
    GEN_IMPORT(Cvar_VariableIntegerValue, G_CVAR_VARIABLE_INTEGER_VALUE),
    GEN_IMPORT(argc, G_ARGC),
    GEN_IMPORT(argv, G_ARGV),
    GEN_IMPORT(args, G_ARGS),
    GEN_IMPORT(AddCommand, G_ADDCOMMAND),
    GEN_IMPORT(FS_ReadFile, G_FS_READFILE),
    GEN_IMPORT(FS_Exists, G_FS_EXISTS),
    GEN_IMPORT(FS_FreeFile, G_FS_FREEFILE),
    GEN_IMPORT(FS_WriteFile, G_FS_WRITEFILE),
    GEN_IMPORT(FS_FOpenFileWrite, G_FS_FOPEN_FILE_WRITE),
    GEN_IMPORT(FS_FOpenFileAppend, G_FS_FOPEN_FILE_APPEND),
    GEN_IMPORT(FS_ListFiles, G_FS_LISTFILES),
    GEN_IMPORT(FS_PrepFileWrite, G_FS_PREPFILEWRITE),
    GEN_IMPORT(FS_Write, G_FS_WRITE),
    GEN_IMPORT(FS_Read, G_FS_READ),
    GEN_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE),
    GEN_IMPORT(FS_FTell, G_FS_FTELL),
    GEN_IMPORT(FS_FSeek, G_FS_FSEEK),
    GEN_IMPORT(FS_Flush, G_FS_FLUSH),
    GEN_IMPORT(FS_DeleteFile, G_FS_DELETEFILE),
    GEN_IMPORT(FS_GetFileList, G_FS_GETFILELIST),
    GEN_IMPORT(GetArchiveFileName, G_GETARCHIVEFILENAME),
    GEN_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND),
    GEN_IMPORT_2(DebugGraph, G_DEBUGGRAPH, void, float, int),
    GEN_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND),
    GEN_IMPORT(GetNumFreeReliableServerCommands, G_GETNUMFREERELIABLESERVERCOMMANDS),
    GEN_IMPORT(setConfigstring, G_SET_CONFIGSTRING),
    GEN_IMPORT(getConfigstring, G_GET_CONFIGSTRING),
    GEN_IMPORT(setUserinfo, G_SET_USERINFO),
    GEN_IMPORT(getUserinfo, G_GET_USERINFO),
    GEN_IMPORT(SetBrushModel, G_SET_BRUSH_MODEL),
    GEN_IMPORT(trace, G_TRACE),
    GEN_IMPORT(fulltrace, G_FULLTRACE),
    GEN_IMPORT(pointcontents, G_POINT_CONTENTS),
    GEN_IMPORT(pointbrushnum, G_POINTBRUSHNUM),
    GEN_IMPORT(inPVS, G_IN_PVS),
    GEN_IMPORT(inPVSIgnorePortals, G_IN_PVS_IGNOREPORTALS),
    GEN_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE),
    GEN_IMPORT(AreasConnected, G_AREAS_CONNECTED),
    GEN_IMPORT(GetLightingGroup, G_GETLIGHTINGGROUP),
    GEN_IMPORT_2(SetDynamicLight, G_SETDYNAMICLIGHT, void, int, float),
    GEN_IMPORT_2(SetDynamicLightDefault, G_SETDYNAMICLIGHTDEFAULT, void, int, float),
    GEN_IMPORT(SetWindDirection, G_SETWINDDIRECTION),
    GEN_IMPORT_1(SetWindIntensity, G_SETWINDINTENSITY, void, float),
    GEN_IMPORT(SetWeatherInfo, G_SETWEATHERINFO),
    GEN_IMPORT_1(SetTimeScale, G_SETTIMESCALE, void, float),
    GEN_IMPORT(linkentity, G_LINKENTITY),
    GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
    GEN_IMPORT(AreaEntities, G_AREAENTITIES),
    GEN_IMPORT(ClipToEntity, G_CLIPTOENTITY),
    GEN_IMPORT(objectivenameindex, G_OBJECTIVENAMEINDEX),
    GEN_IMPORT(archetypeindex, G_ARCHETYPEINDEX),
    GEN_IMPORT(imageindex, G_IMAGEINDEX),
    GEN_IMPORT(failedcondition, G_FAILEDCONDITION),
    GEN_IMPORT(itemindex, G_ITEMINDEX),
    GEN_IMPORT(soundindex, G_SOUNDINDEX),
    GEN_IMPORT(modelindex, G_MODELINDEX),
    GEN_IMPORT(SetLightStyle, G_SETLIGHTSTYLE),
    GEN_IMPORT(GameDir, G_GAMEDIR),
    GEN_IMPORT(IsModel, G_ISMODEL),
    GEN_IMPORT(setmodel, G_SETMODEL),
    GEN_IMPORT(setviewmodel, G_SETVIEWMODEL),
    GEN_IMPORT(NumAnims, G_NUMANIMS),
    GEN_IMPORT(NumSkins, G_NUMSKINS),
    GEN_IMPORT(NumSurfaces, G_NUMSURFACES),
    GEN_IMPORT(NumTags, G_NUMTAGS),
    GEN_IMPORT(NumMorphs, G_NUMMORPHS),
    GEN_IMPORT(InitCommands, G_INITCOMMANDS),
    GEN_IMPORT_4(CalculateBounds, G_CALCULATEBOUNDS, void, int, float, vec3_t, vec3_t),
    GEN_IMPORT(TIKI_CacheAnim, G_TIKI_CACHEANIM),
    GEN_IMPORT(Anim_NameForNum, G_ANIM_NAMEFORNUM),
    GEN_IMPORT(Anim_NumForName, G_ANIM_NUMFORNAME),
    GEN_IMPORT(Anim_Random, G_ANIM_RANDOM),
    GEN_IMPORT(Anim_NumFrames, G_ANIM_NUMFRAMES),
    GEN_IMPORT_2_F(Anim_Time, G_ANIM_TIME, int, int),
    GEN_IMPORT(Anim_Delta, G_ANIM_DELTA),
    GEN_IMPORT(Anim_AbsoluteDelta, G_ANIM_ABSOLUTEDELTA),
    GEN_IMPORT(Anim_Flags, G_ANIM_FLAGS),
    GEN_IMPORT(Anim_HasCommands, G_ANIM_HASCOMMANDS),
    GEN_IMPORT(Frame_Commands, G_FRAME_COMMANDS),
    GEN_IMPORT(Frame_Delta, G_FRAME_DELTA),
    GEN_IMPORT_3_F(Frame_Time, G_FRAME_TIME, int, int, int),
    GEN_IMPORT_6(Frame_Bounds, G_FRAME_BOUNDS, void, int, int, int, float, vec3_t, vec3_t),
    GEN_IMPORT(Surface_NameToNum, G_SURFACE_NAMETONUM),
    GEN_IMPORT(Surface_NumToName, G_SURFACE_NUMTONAME),
    GEN_IMPORT(Surface_Flags, G_SURFACE_FLAGS),
    GEN_IMPORT(Surface_NumSkins, G_SURFACE_NUMSKINS),
    GEN_IMPORT(Morph_NumForName, G_MORPH_NUMFORNAME),
    GEN_IMPORT(Morph_NameForNum, G_MORPH_NAMEFORNUM),
    GEN_IMPORT(GetExpression, G_GETEXPRESSION),
    GEN_IMPORT(Tag_NumForName, G_TAG_NUMFORNAME),
    GEN_IMPORT(Tag_NameForNum, G_TAG_NAMEFORNUM),
    GEN_IMPORT_8(Tag_Orientation, G_TAG_ORIENTATION, orientation_t*, orientation_t*, int, int, int, int, float, int*, vec4_t*),
    GEN_IMPORT_18(Tag_OrientationEx, G_TAG_ORIENTATIONEX, orientation_t*, orientation_t*, int, int, int, int, float, int*, vec4_t*, int, int, float, qboolean, qboolean, int, int, int, int, float),
    GEN_IMPORT(Bone_GetParentNum, G_BONE_GETPARENTNUM),
    GEN_IMPORT(Alias_Add, G_ALIAS_ADD),
    GEN_IMPORT(Alias_FindRandom, G_ALIAS_FINDRANDOM),
    GEN_IMPORT(Alias_Find, G_ALIAS_FIND),
    GEN_IMPORT(Alias_Dump, G_ALIAS_DUMP),
    GEN_IMPORT(Alias_Clear, G_ALIAS_CLEAR),
    GEN_IMPORT(Alias_FindDialog, G_ALIAS_FINDDIALOG),
    GEN_IMPORT(Alias_FindSpecificAnim, G_ALIAS_FINDSPECIFICANIM),
    GEN_IMPORT(Alias_CheckLoopAnim, G_ALIAS_CHECKLOOPANIM),
    GEN_IMPORT(Alias_GetList, G_ALIAS_GETLIST),
    GEN_IMPORT(Alias_UpdateDialog, G_ALIAS_UPDATEDIALOG),
    GEN_IMPORT(Alias_AddActorDialog, G_ALIAS_ADDACTORDIALOG),
    GEN_IMPORT(NameForNum, G_NAMEFORNUM),
    GEN_IMPORT(GlobalAlias_Add, G_GLOBALALIAS_ADD),
    GEN_IMPORT(GlobalAlias_FindRandom, G_GLOBALALIAS_FINDRANDOM),
    GEN_IMPORT(GlobalAlias_Find, G_GLOBALALIAS_FIND),
    GEN_IMPORT(GlobalAlias_Dump, G_GLOBALALIAS_DUMP),
    GEN_IMPORT(GlobalAlias_Clear, G_GLOBALALIAS_CLEAR),
    GEN_IMPORT(isClientActive, G_ISCLIENTACTIVE),
    GEN_IMPORT(centerprintf, G_CENTERPRINTF),
    GEN_IMPORT(locationprintf, G_LOCATIONPRINTF),
    GEN_IMPORT_8(Sound, G_SOUND, void, vec3_t*, int, int, const char*, float, float, float, qboolean),
    GEN_IMPORT(StopSound, G_STOPSOUND),
    GEN_IMPORT_1_F(SoundLength, G_SOUNDLENGTH, const char*),
    GEN_IMPORT(GetNextMorphTarget, G_GETNEXTMORPHTARGET),
    GEN_IMPORT(CalcCRC, G_CALCCRC),
    nullptr,	// DebugLines
    nullptr,	// numDebugLines
    GEN_IMPORT(LocateGameData, G_LOCATE_GAME_DATA),
    GEN_IMPORT(SetFarPlane, G_SETFARPLANE),
    GEN_IMPORT(TikiReload, G_TIKIRELOAD),
    GEN_IMPORT(TikiLoadFromTS, G_TIKILOADFROMTS),
    GEN_IMPORT(ToolServerGetData, G_TOOLSERVERGETDATA),
    GEN_IMPORT(SetSkyPortal, G_SETSKYPORTAL),
    GEN_IMPORT(WidgetPrintf, G_WIDGETPRINTF),
    GEN_IMPORT(ProcessLoadingScreen, G_PROCESSLOADINGSCREEN),
    GEN_IMPORT(MObjective_GetDescription, G_MOBJECTIVE_GETDESCRIPTION),
    GEN_IMPORT(MObjective_SetDescription, G_MOBJECTIVE_SETDESCRIPTION),
    GEN_IMPORT(MObjective_GetShowObjective, G_MOBJECTIVE_GETSHOWOBJECTIVE),
    GEN_IMPORT(MObjective_SetShowObjective, G_MOBJECTIVE_SETSHOWOBJECTIVE),
    GEN_IMPORT(MObjective_GetObjectiveComplete, G_MOBJECTIVE_GETOBJECTIVECOMPLETE),
    GEN_IMPORT(MObjective_SetObjectiveComplete, G_MOBJECTIVE_SETOBJECTIVECOMPLETE),
    GEN_IMPORT(MObjective_GetObjectiveFailed, G_MOBJECTIVE_GETOBJECTIVEFAILED),
    GEN_IMPORT(MObjective_SetObjectiveFailed, G_MOBJECTIVE_SETOBJECTIVEFAILED),
    GEN_IMPORT(MObjective_GetNameFromIndex, G_MOBJECTIVE_GETNAMEFROMINDEX),
    GEN_IMPORT(MObjective_GetIndexFromName, G_MOBJECTIVE_GETINDEXFROMNAME),
    GEN_IMPORT(MObjective_NewObjective, G_MOBJECTIVE_NEWOBJECTIVE),
    GEN_IMPORT(MObjective_ClearObjectiveList, G_MOBJECTIVE_CLEAROBJECTIVELIST),
    GEN_IMPORT(MObjective_ParseObjectiveFile, G_MOBJECTIVE_PARSEOBJECTIVEFILE),
    GEN_IMPORT(MObjective_Update, G_MOBJECTIVE_UPDATE),
    GEN_IMPORT(MObjective_GetNumObjectives, G_MOBJECTIVE_GETNUMOBJECTIVES),
    GEN_IMPORT(MObjective_GetNumActiveObjectives, G_MOBJECTIVE_GETNUMACTIVEOBJECTIVES),
    GEN_IMPORT(MObjective_GetNumCompleteObjectives, G_MOBJECTIVE_GETNUMCOMPLETEOBJECTIVES),
    GEN_IMPORT(MObjective_GetNumFailedObjectives, G_MOBJECTIVE_GETNUMFAILEDOBJECTIVES),
    GEN_IMPORT(MObjective_GetNumIncompleteObjectives, G_MOBJECTIVE_GETNUMINCOMPLETEOBJECTIVES),
    GEN_IMPORT(MI_GetShader, G_MI_GETSHADER),
    GEN_IMPORT(MI_SetShader, G_MI_SETSHADER),
    GEN_IMPORT(MI_GetInformationData, G_MI_GETINFORMATIONDATA),
    GEN_IMPORT(MI_SetInformationData, G_MI_SETINFORMATIONDATA),
    GEN_IMPORT(MI_GetNameFromIndex, G_MI_GETNAMEFROMINDEX),
    GEN_IMPORT(MI_GetIndexFromName, G_MI_GETINDEXFROMNAME),
    GEN_IMPORT(MI_NewInformation, G_MI_NEWINFORMATION),
    GEN_IMPORT(MI_ClearInformationList, G_MI_CLEARINFORMATIONLIST),
    GEN_IMPORT(MI_SetShowInformation, G_MI_SETSHOWINFORMATION),
    GEN_IMPORT(MI_GetShowInformation, G_MI_GETSHOWINFORMATION),
    GEN_IMPORT(SR_InitializeStringResource, G_SR_INITIALIZESTRINGRESOURCE),
    GEN_IMPORT(SR_UninitializeStringResource, G_SR_UNINITIALIZESTRINGRESOURCE),
    GEN_IMPORT(SR_LoadLevelStrings, G_SR_LOADLEVELSTRINGS),
    GEN_IMPORT(GetViewModeMask, G_GETVIEWMODEMASK),
    GEN_IMPORT(GetViewModeClassMask, G_GETVIEWMODECLASSMASK),
    GEN_IMPORT(GetViewModeSendInMode, G_GETVIEWMODESENDINMODE),
    GEN_IMPORT(GetViewModeSendNotInMode, G_GETVIEWMODESENDNOTINMODE),
    GEN_IMPORT(GetViewModeScreenBlend, G_GETVIEWMODESCREENBLEND),
    GEN_IMPORT(GetLevelDefs, G_GETLEVELDEFS),
    GEN_IMPORT(areSublevels, G_ARESUBLEVELS),
    GEN_IMPORT(SurfaceTypeToName, G_SURFACETYPETONAME),
    GEN_IMPORT(AAS_EntityInfo, G_AAS_ENTITYINFO),
    GEN_IMPORT(AAS_Initialized, G_AAS_INITIALIZED),
    GEN_IMPORT(AAS_PresenceTypeBoundingBox, G_AAS_PRESENCETYPEBOUNDINGBOX),
    GEN_IMPORT_0_F(AAS_Time, G_AAS_TIME),
    GEN_IMPORT(AAS_PointAreaNum, G_AAS_POINTAREANUM),
    GEN_IMPORT(AAS_PointReachabilityAreaIndex, G_AAS_POINTREACHABILITYAREAINDEX),
    GEN_IMPORT(AAS_TraceAreas, G_AAS_TRACEAREAS),
    GEN_IMPORT(AAS_BBoxAreas, G_AAS_BBOXAREAS),
    GEN_IMPORT(AAS_AreaInfo, G_AAS_AREAINFO),
    GEN_IMPORT(AAS_PointContents, G_AAS_POINTCONTENTS),
    GEN_IMPORT(AAS_NextBSPEntity, G_AAS_NEXTBSPENTITY),
    GEN_IMPORT(AAS_ValueForBSPEpairKey, G_AAS_VALUEFORBSPEPAIRKEY),
    GEN_IMPORT(AAS_VectorForBSPEpairKey, G_AAS_VECTORFORBSPEPAIRKEY),
    GEN_IMPORT(AAS_FloatForBSPEpairKey, G_AAS_FLOATFORBSPEPAIRKEY),
    GEN_IMPORT(AAS_IntForBSPEpairKey, G_AAS_INTFORBSPEPAIRKEY),
    GEN_IMPORT(AAS_AreaReachability, G_AAS_AREAREACHABILITY),
    GEN_IMPORT(AAS_AreaTravelTimeToGoalArea, G_AAS_AREATRAVELTIMETOGOALAREA),
    GEN_IMPORT(AAS_EnableRoutingArea, G_AAS_ENABLEROUTINGAREA),
    GEN_IMPORT(AAS_PredictRoute, G_AAS_PREDICTROUTE),
    GEN_IMPORT(AAS_AlternativeRouteGoals, G_AAS_ALTERNATIVEROUTEGOALS),
    GEN_IMPORT(AAS_Swimming, G_AAS_SWIMMING),
    GEN_IMPORT(AAS_PredictClientMovement, G_AAS_PREDICTCLIENTMOVEMENT),
    GEN_IMPORT(EA_Command, G_EA_COMMAND),
    GEN_IMPORT(EA_Say, G_EA_SAY),
    GEN_IMPORT(EA_SayTeam, G_EA_SAYTEAM),
    GEN_IMPORT(EA_Action, G_EA_ACTION),
    GEN_IMPORT(EA_Gesture, G_EA_GESTURE),
    GEN_IMPORT(EA_Talk, G_EA_TALK),
    GEN_IMPORT(EA_ToggleFireState, G_EA_TOGGLEFIRESTATE),
    GEN_IMPORT(EA_Attack, G_EA_ATTACK),
    GEN_IMPORT(EA_Use, G_EA_USE),
    GEN_IMPORT(EA_Respawn, G_EA_RESPAWN),
    GEN_IMPORT(EA_MoveUp, G_EA_MOVEUP),
    GEN_IMPORT(EA_MoveDown, G_EA_MOVEDOWN),
    GEN_IMPORT(EA_MoveForward, G_EA_MOVEFORWARD),
    GEN_IMPORT(EA_MoveBack, G_EA_MOVEBACK),
    GEN_IMPORT(EA_MoveLeft, G_EA_MOVELEFT),
    GEN_IMPORT(EA_MoveRight, G_EA_MOVERIGHT),
    GEN_IMPORT(EA_Crouch, G_EA_CROUCH),
    GEN_IMPORT(EA_SelectWeapon, G_EA_SELECTWEAPON),
    GEN_IMPORT(EA_Jump, G_EA_JUMP),
    GEN_IMPORT(EA_DelayedJump, G_EA_DELAYEDJUMP),
    GEN_IMPORT_3(EA_Move, G_EA_MOVE, void, int, vec3_t, float),
    GEN_IMPORT(EA_View, G_EA_VIEW),
    GEN_IMPORT_2(EA_EndRegular, G_EA_ENDREGULAR, void, int, float),
    GEN_IMPORT_3(EA_GetInput, G_EA_GETINPUT, void, int, float, bot_input_t*),
    GEN_IMPORT(EA_ResetInput, G_EA_RESETINPUT),
    GEN_IMPORT_2(BotLoadCharacter, G_BOTLOADCHARACTER, int, char*, float),
    GEN_IMPORT(BotFreeCharacter, G_BOTFREECHARACTER),
    GEN_IMPORT(Characteristic_Float, G_CHARACTERISTIC_FLOAT),
    GEN_IMPORT_4_F(Characteristic_BFloat, G_CHARACTERISTIC_BFLOAT, int, int, float, float),
    GEN_IMPORT(Characteristic_Integer, G_CHARACTERISTIC_INTEGER),
    GEN_IMPORT(Characteristic_BInteger, G_CHARACTERISTIC_BINTEGER),
    GEN_IMPORT(Characteristic_String, G_CHARACTERISTIC_STRING),
    GEN_IMPORT(BotAllocChatState, G_BOTALLOCCHATSTATE),
    GEN_IMPORT(BotFreeChatState, G_BOTFREECHATSTATE),
    GEN_IMPORT(BotQueueConsoleMessage, G_BOTQUEUECONSOLEMESSAGE),
    GEN_IMPORT(BotRemoveConsoleMessage, G_BOTREMOVECONSOLEMESSAGE),
    GEN_IMPORT(BotNextConsoleMessage, G_BOTNEXTCONSOLEMESSAGE),
    GEN_IMPORT(BotNumConsoleMessages, G_BOTNUMCONSOLEMESSAGES),
    GEN_IMPORT(BotInitialChat, G_BOTINITIALCHAT),
    GEN_IMPORT(BotNumInitialChats, G_BOTNUMINITIALCHATS),
    GEN_IMPORT(BotReplyChat, G_BOTREPLYCHAT),
    GEN_IMPORT(BotChatLength, G_BOTCHATLENGTH),
    GEN_IMPORT(BotEnterChat, G_BOTENTERCHAT),
    GEN_IMPORT(BotGetChatMessage, G_BOTGETCHATMESSAGE),
    GEN_IMPORT(StringContains, G_STRINGCONTAINS),
    GEN_IMPORT(BotFindMatch, G_BOTFINDMATCH),
    GEN_IMPORT(BotMatchVariable, G_BOTMATCHVARIABLE),
    GEN_IMPORT(UnifyWhiteSpaces, G_UNIFYWHITESPACES),
    GEN_IMPORT(BotReplaceSynonyms, G_BOTREPLACESYNONYMS),
    GEN_IMPORT(BotLoadChatFile, G_BOTLOADCHATFILE),
    GEN_IMPORT(BotSetChatGender, G_BOTSETCHATGENDER),
    GEN_IMPORT(BotSetChatName, G_BOTSETCHATNAME),
    GEN_IMPORT(BotResetGoalState, G_BOTRESETGOALSTATE),
    GEN_IMPORT(BotResetAvoidGoals, G_BOTRESETAVOIDGOALS),
    GEN_IMPORT(BotRemoveFromAvoidGoals, G_BOTREMOVEFROMAVOIDGOALS),
    GEN_IMPORT(BotPushGoal, G_BOTPUSHGOAL),
    GEN_IMPORT(BotPopGoal, G_BOTPOPGOAL),
    GEN_IMPORT(BotEmptyGoalStack, G_BOTEMPTYGOALSTACK),
    GEN_IMPORT(BotDumpAvoidGoals, G_BOTDUMPAVOIDGOALS),
    GEN_IMPORT(BotDumpGoalStack, G_BOTDUMPGOALSTACK),
    GEN_IMPORT(BotGoalName, G_BOTGOALNAME),
    GEN_IMPORT(BotGetTopGoal, G_BOTGETTOPGOAL),
    GEN_IMPORT(BotGetSecondGoal, G_BOTGETSECONDGOAL),
    GEN_IMPORT(BotChooseLTGItem, G_BOTCHOOSELTGITEM),
    GEN_IMPORT_6(BotChooseNBGItem, G_BOTCHOOSENBGITEM, int, int, vec3_t, int*, int, struct bot_goal_s*, float),
    GEN_IMPORT(BotTouchingGoal, G_BOTTOUCHINGGOAL),
    GEN_IMPORT(BotItemGoalInVisButNotVisible, G_BOTITEMGOALINVISBUTNOTVISIBLE),
    GEN_IMPORT(BotGetLevelItemGoal, G_BOTGETLEVELITEMGOAL),
    GEN_IMPORT(BotGetNextCampSpotGoal, G_BOTGETNEXTCAMPSPOTGOAL),
    GEN_IMPORT(BotGetMapLocationGoal, G_BOTGETMAPLOCATIONGOAL),
    GEN_IMPORT_2_F(BotAvoidGoalTime, G_BOTAVOIDGOALTIME, int, int),
    GEN_IMPORT_3(BotSetAvoidGoalTime, G_BOTSETAVOIDGOALTIME, void, int, int, float),
    GEN_IMPORT(BotInitLevelItems, G_BOTINITLEVELITEMS),
    GEN_IMPORT(BotUpdateEntityItems, G_BOTUPDATEENTITYITEMS),
    GEN_IMPORT(BotLoadItemWeights, G_BOTLOADITEMWEIGHTS),
    GEN_IMPORT(BotFreeItemWeights, G_BOTFREEITEMWEIGHTS),
    GEN_IMPORT(BotInterbreedGoalFuzzyLogic, G_BOTINTERBREEDGOALFUZZYLOGIC),
    GEN_IMPORT(BotSaveGoalFuzzyLogic, G_BOTSAVEGOALFUZZYLOGIC),
    GEN_IMPORT_2(BotMutateGoalFuzzyLogic, G_BOTMUTATEGOALFUZZYLOGIC, void, int, float),
    GEN_IMPORT(BotAllocGoalState, G_BOTALLOCGOALSTATE),
    GEN_IMPORT(BotFreeGoalState, G_BOTFREEGOALSTATE),
    GEN_IMPORT(BotResetMoveState, G_BOTRESETMOVESTATE),
    GEN_IMPORT(BotMoveToGoal, G_BOTMOVETOGOAL),
    GEN_IMPORT_4(BotMoveInDirection, G_BOTMOVEINDIRECTION, int, int, vec3_t, float, int),
    GEN_IMPORT(BotResetAvoidReach, G_BOTRESETAVOIDREACH),
    GEN_IMPORT(BotResetLastAvoidReach, G_BOTRESETLASTAVOIDREACH),
    GEN_IMPORT(BotReachabilityArea, G_BOTREACHABILITYAREA),
    GEN_IMPORT_5(BotMovementViewTarget, G_BOTMOVEMENTVIEWTARGET, int, int, struct bot_goal_s*, int, float, vec3_t),
    GEN_IMPORT(BotPredictVisiblePosition, G_BOTPREDICTVISIBLEPOSITION),
    GEN_IMPORT(BotAllocMoveState, G_BOTALLOCMOVESTATE),
    GEN_IMPORT(BotFreeMoveState, G_BOTFREEMOVESTATE),
    GEN_IMPORT(BotInitMoveState, G_BOTINITMOVESTATE),
    GEN_IMPORT_4(BotAddAvoidSpot, G_BOTADDAVOIDSPOT, void, int, vec3_t, float, int),
    GEN_IMPORT(BotChooseBestFightWeapon, G_BOTCHOOSEBESTFIGHTWEAPON),
    GEN_IMPORT(BotGetWeaponInfo, G_BOTGETWEAPONINFO),
    GEN_IMPORT(BotLoadWeaponWeights, G_BOTLOADWEAPONWEIGHTS),
    GEN_IMPORT(BotAllocWeaponState, G_BOTALLOCWEAPONSTATE),
    GEN_IMPORT(BotFreeWeaponState, G_BOTFREEWEAPONSTATE),
    GEN_IMPORT(BotResetWeaponState, G_BOTRESETWEAPONSTATE),
    GEN_IMPORT(GeneticParentsAndChildSelection, G_GENETICPARENTSANDCHILDSELECTION),
    GEN_IMPORT(Print, G_BOTPRINT),
    GEN_IMPORT(PointContents, G_BOTPOINTCONTENTS),
    GEN_IMPORT(BSPEntityData, G_BSPENTITYDATA),
    GEN_IMPORT(BSPModelMinsMaxsOrigin, G_BSPMODELMINSMAXSORIGIN),
    GEN_IMPORT(BotClientCommand, G_BOTCLIENTCOMMAND),
    GEN_IMPORT(AvailableMemory, G_AVAILABLEMEMORY),
    GEN_IMPORT(HunkAlloc, G_HUNKALLOC),
    GEN_IMPORT(FS_FOpenFile, G_FS_FOPEN_FILE),
    GEN_IMPORT(FS_Seek, G_FS_SEEK),
    GEN_IMPORT(DebugLineCreate, G_DEBUGLINECREATE),
    GEN_IMPORT(DebugLineDelete, G_DEBUGLINEDELETE),
    GEN_IMPORT(DebugLineShow, G_DEBUGLINESHOW),
    GEN_IMPORT(DebugPolygonCreate, G_DEBUGPOLYGONCREATE),
    GEN_IMPORT(DebugPolygonDelete, G_DEBUGPOLYGONDELETE),
    GEN_IMPORT(DropClient, G_DROP_CLIENT),
    GEN_IMPORT(SV_GetServerinfo, G_SV_GETSERVERINFO),
    GEN_IMPORT(BotAllocateClient, G_BOTALLOCATECLIENT),
    GEN_IMPORT(BotGetSnapshotEntity, G_BOTGETSNAPSHOTENTITY),
    GEN_IMPORT(BotGetConsoleMessage, G_BOTGETCONSOLEMESSAGE),
    GEN_IMPORT(BotLibSetup, G_BOTLIBSETUP),
    GEN_IMPORT(BotLibShutdown, G_BOTLIBSHUTDOWN),
    GEN_IMPORT(BotLibVarSet, G_BOTLIBVARSET),
    GEN_IMPORT(BotLibVarGet, G_BOTLIBVARGET),
    GEN_IMPORT(PC_AddGlobalDefine, G_PC_ADDGLOBALDEFINE),
    GEN_IMPORT(PC_LoadSourceHandle, G_PC_LOADSOURCEHANDLE),
    GEN_IMPORT(PC_FreeSourceHandle, G_PC_FREESOURCEHANDLE),
    GEN_IMPORT(PC_SourceFileAndLine, G_PC_SOURCEFILEANDLINE),
    GEN_IMPORT_1(BotLibStartFrame, G_BOTLIBSTARTFRAME, int, float),
    GEN_IMPORT(BotLibLoadMap, G_BOTLIBLOADMAP),
    GEN_IMPORT(BotLibUpdateEntity, G_BOTLIBUPDATEENTITY),
    GEN_IMPORT(Test, G_TEST),
    GEN_IMPORT(BotUserCommand, G_BOTUSERCOMMAND),
};


std::vector<std::string> STEF2_GameSupport::entity_tokens;
size_t STEF2_GameSupport::token_counter = 0;
void STEF2_GameSupport::SpawnEntities(const char* mapname, const char* entstring, int levelTime) {
    if (entstring) {
        entity_tokens = util_parse_entstring(entstring);
        token_counter = 0;
    }
    cgameinfo.is_from_QMM = true;
    (void)::vmMain(GAME_SPAWN_ENTITIES, mapname, entstring, levelTime);
}


game_export_t STEF2_GameSupport::qmm_export = {
    GAME_API_VERSION,	// apiversion
    GEN_EXPORT(Init, GAME_INIT),
    GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
    GEN_EXPORT(Cleanup, GAME_CLEANUP),
    STEF2_GameSupport::SpawnEntities,
    GEN_EXPORT(PostLoad, GAME_POSTLOAD),
    GEN_EXPORT(PostSublevelLoad, GAME_POSTSUBLEVELLOAD),
    GEN_EXPORT(ClientConnect, GAME_CLIENT_CONNECT),
    GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
    GEN_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED),
    GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
    GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
    GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
    GEN_EXPORT(PrepFrame, GAME_PREP_FRAME),
    GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
    GEN_EXPORT(SendEntity, GAME_SEND_ENTITY),
    GEN_EXPORT(UpdateEntityStateForClient, GAME_UPDATE_ENTITYSTATE_FOR_CLIENT),
    GEN_EXPORT(UpdatePlayerStateForClient, GAME_UPDATE_PLAYERSTATE_FOR_CLIENT),
    GEN_EXPORT(ExtraEntitiesToSend, GAME_EXTRA_ENTITIES_TO_SEND),
    GEN_EXPORT(GetEntityCurrentAnimFrame, GAME_GETENTITY_CURRENT_ANIMFRAME),
    GEN_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND),
    GEN_EXPORT(WritePersistant, GAME_WRITE_PERSISTANT),
    GEN_EXPORT(ReadPersistant, GAME_READ_PERSISTANT),
    GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
    GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
    GEN_EXPORT(LevelArchiveValid, GAME_LEVEL_ARCHIVE_VALID),
    GEN_EXPORT(inMultiplayerGame, GAME_IN_MULTIPLAYER_GAME),
    GEN_EXPORT(isDefined, GAME_IS_DEFINED),
    GEN_EXPORT(getDefine, GAME_GET_DEFINE),
    GEN_EXPORT(BotAIStartFrame, BOTAI_START_FRAME),
    GEN_EXPORT(AddBot_f, GAME_ADDBOT_F),
    GEN_EXPORT(GetTotalGameFrames, GAME_GETTOTALGAMEFRAMES),
    // the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
    nullptr,	// gentities
    0,			// gentitySize
    0,			// num_entities
    0,			// max_entities
    nullptr,	// errorMessage
};

#endif // QMM_ARCH_32
