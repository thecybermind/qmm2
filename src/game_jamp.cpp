/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <jamp/game/q_shared.h>
#include <jamp/game/g_public.h>

#include "game_api.h"
#include "log.h"
#include <string>
// QMM-specific JAMP header
#include "game_jamp.h"
#include "main.h"
#include "util.h"

GEN_QMM_MSGS(JAMP);
GEN_EXTS(JAMP);

GEN_FUNCS(JAMP);


// auto-detection logic for JAMP
static bool JAMP_autodetect(bool, supportedgame* game) {
    // QMM filename must match default or an OpenJK temp filename (if DLL was pulled from .pk3)
    if (!str_striequal(g_gameinfo.qmm_file, game->dllname)
        && !str_striequal(g_gameinfo.qmm_file.substr(0, 3), "ojk")
        && !str_striequal(path_baseext(g_gameinfo.qmm_file), "tmp"))
    {
        return false;
    }

    if (!str_stristr(g_gameinfo.exe_file, "jamp")
        && !str_stristr(g_gameinfo.exe_file, "openjk.")
        && !str_stristr(g_gameinfo.exe_file, "openjkded"))
    {
        return false;
    }

    return true;
}


// original syscall pointer that comes from the game engine
static eng_syscall orig_syscall = nullptr;

// pointer to vmMain that comes from the mod
static mod_vmMain orig_vmMain = nullptr;


// these variables are all for the GGA implementation for OpenJK

// a copy of the apiversion int that comes from the game engine
static intptr_t orig_apiversion = 0;

// a copy of the original import struct that comes from the game engine
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod
static game_export_t* orig_export = nullptr;

// struct with lambdas that call QMM's syscall function. this is given to the mod
static game_import_t qmm_import = {
    GEN_IMPORT(Print, G_PRINT),
    GEN_IMPORT(Error, G_ERROR),
    GEN_IMPORT(Milliseconds, G_MILLISECONDS),
    GEN_IMPORT(PrecisionTimerStart, G_PRECISIONTIMER_START),
    GEN_IMPORT(PrecisionTimerEnd, G_PRECISIONTIMER_END),
    GEN_IMPORT(SV_RegisterSharedMemory, G_SET_SHARED_BUFFER),
    GEN_IMPORT(RealTime, G_REAL_TIME),
    GEN_IMPORT(TrueMalloc, G_TRUEMALLOC),
    GEN_IMPORT(TrueFree, G_TRUEFREE),
    GEN_IMPORT(SnapVector, G_SNAPVECTOR),
    GEN_IMPORT(Cvar_Register, G_CVAR_REGISTER),
    GEN_IMPORT(Cvar_Set, G_CVAR_SET),
    GEN_IMPORT(Cvar_Update, G_CVAR_UPDATE),
    GEN_IMPORT(Cvar_VariableIntegerValue, G_CVAR_VARIABLE_INTEGER_VALUE),
    GEN_IMPORT(Cvar_VariableStringBuffer, G_CVAR_VARIABLE_STRING_BUFFER),
    GEN_IMPORT(Argc, G_ARGC),
    GEN_IMPORT(Argv, G_ARGV),
    GEN_IMPORT(FS_Close, G_FS_FCLOSE_FILE),
    GEN_IMPORT(FS_GetFileList, G_FS_GETFILELIST),
    GEN_IMPORT(FS_Open, G_FS_FOPEN_FILE),
    GEN_IMPORT(FS_Read, G_FS_READ),
    GEN_IMPORT(FS_Write, G_FS_WRITE),
    GEN_IMPORT(AdjustAreaPortalState, G_ADJUST_AREA_PORTAL_STATE),
    GEN_IMPORT(AreasConnected, G_AREAS_CONNECTED),
    GEN_IMPORT(DebugPolygonCreate, G_DEBUG_POLYGON_CREATE),
    GEN_IMPORT(DebugPolygonDelete, G_DEBUG_POLYGON_DELETE),
    GEN_IMPORT(DropClient, G_DROP_CLIENT),
    GEN_IMPORT(EntitiesInBox, G_ENTITIES_IN_BOX),
    GEN_IMPORT(EntityContact, G_ENTITY_CONTACT),
    GEN_IMPORT(GetConfigstring, G_GET_CONFIGSTRING),
    GEN_IMPORT(GetEntityToken, G_GET_ENTITY_TOKEN),
    GEN_IMPORT(GetServerinfo, G_GET_SERVERINFO),
    GEN_IMPORT(GetUsercmd, G_GET_USERCMD),
    GEN_IMPORT(GetUserinfo, G_GET_USERINFO),
    GEN_IMPORT(InPVS, G_IN_PVS),
    GEN_IMPORT(InPVSIgnorePortals, G_IN_PVS_IGNORE_PORTALS),
    GEN_IMPORT(LinkEntity, G_LINKENTITY),
    GEN_IMPORT(LocateGameData, G_LOCATE_GAME_DATA),
    GEN_IMPORT(PointContents, G_POINT_CONTENTS),
    GEN_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND),
    GEN_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND),
    GEN_IMPORT(SetBrushModel, G_SET_BRUSH_MODEL),
    GEN_IMPORT(SetConfigstring, G_SET_CONFIGSTRING),
    GEN_IMPORT(SetServerCull, G_SET_SERVER_CULL),
    GEN_IMPORT(SetUserinfo, G_SET_USERINFO),
    GEN_IMPORT(SiegePersSet, G_SIEGEPERSSET),
    GEN_IMPORT(SiegePersGet, G_SIEGEPERSGET),
    GEN_IMPORT(Trace, G_TRACE),
    GEN_IMPORT(UnlinkEntity, G_UNLINKENTITY),
    GEN_IMPORT(ROFF_Clean, G_ROFF_CLEAN),
    GEN_IMPORT(ROFF_UpdateEntities, G_ROFF_UPDATE_ENTITIES),
    GEN_IMPORT(ROFF_Cache, G_ROFF_CACHE),
    GEN_IMPORT(ROFF_Play, G_ROFF_PLAY),
    GEN_IMPORT(ROFF_Purge_Ent, G_ROFF_PURGE_ENT),
    GEN_IMPORT(ICARUS_RunScript, G_ICARUS_RUNSCRIPT),
    GEN_IMPORT(ICARUS_RegisterScript, G_ICARUS_REGISTERSCRIPT),
    GEN_IMPORT(ICARUS_Init, G_ICARUS_INIT),
    GEN_IMPORT(ICARUS_ValidEnt, G_ICARUS_VALIDENT),
    GEN_IMPORT(ICARUS_IsInitialized, G_ICARUS_ISINITIALIZED),
    GEN_IMPORT(ICARUS_MaintainTaskManager, G_ICARUS_MAINTAINTASKMANAGER),
    GEN_IMPORT(ICARUS_IsRunning, G_ICARUS_ISRUNNING),
    GEN_IMPORT(ICARUS_TaskIDPending, G_ICARUS_TASKIDPENDING),
    GEN_IMPORT(ICARUS_InitEnt, G_ICARUS_INITENT),
    GEN_IMPORT(ICARUS_FreeEnt, G_ICARUS_FREEENT),
    GEN_IMPORT(ICARUS_AssociateEnt, G_ICARUS_ASSOCIATEENT),
    GEN_IMPORT(ICARUS_Shutdown, G_ICARUS_SHUTDOWN),
    GEN_IMPORT(ICARUS_TaskIDSet, G_ICARUS_TASKIDSET),
    GEN_IMPORT(ICARUS_TaskIDComplete, G_ICARUS_TASKIDCOMPLETE),
    GEN_IMPORT(ICARUS_SetVar, G_ICARUS_SETVAR),
    GEN_IMPORT(ICARUS_VariableDeclared, G_ICARUS_VARIABLEDECLARED),
    GEN_IMPORT(ICARUS_GetFloatVariable, G_ICARUS_GETFLOATVARIABLE),
    GEN_IMPORT(ICARUS_GetStringVariable, G_ICARUS_GETSTRINGVARIABLE),
    GEN_IMPORT(ICARUS_GetVectorVariable, G_ICARUS_GETVECTORVARIABLE),
    GEN_IMPORT(Nav_Init, G_NAV_INIT),
    GEN_IMPORT(Nav_Free, G_NAV_FREE),
    GEN_IMPORT(Nav_Load, G_NAV_LOAD),
    GEN_IMPORT(Nav_Save, G_NAV_SAVE),
    GEN_IMPORT(Nav_AddRawPoint, G_NAV_ADDRAWPOINT),
    GEN_IMPORT(Nav_CalculatePaths, G_NAV_CALCULATEPATHS),
    GEN_IMPORT(Nav_HardConnect, G_NAV_HARDCONNECT),
    GEN_IMPORT(Nav_ShowNodes, G_NAV_SHOWNODES),
    GEN_IMPORT(Nav_ShowEdges, G_NAV_SHOWEDGES),
    GEN_IMPORT(Nav_ShowPath, G_NAV_SHOWPATH),
    GEN_IMPORT(Nav_GetNearestNode, G_NAV_GETNEARESTNODE),
    GEN_IMPORT(Nav_GetBestNode, G_NAV_GETBESTNODE),
    GEN_IMPORT(Nav_GetNodePosition, G_NAV_GETNODEPOSITION),
    GEN_IMPORT(Nav_GetNodeNumEdges, G_NAV_GETNODENUMEDGES),
    GEN_IMPORT(Nav_GetNodeEdge, G_NAV_GETNODEEDGE),
    GEN_IMPORT(Nav_GetNumNodes, G_NAV_GETNUMNODES),
    GEN_IMPORT(Nav_Connected, G_NAV_CONNECTED),
    GEN_IMPORT(Nav_GetPathCost, G_NAV_GETPATHCOST),
    GEN_IMPORT(Nav_GetEdgeCost, G_NAV_GETEDGECOST),
    GEN_IMPORT(Nav_GetProjectedNode, G_NAV_GETPROJECTEDNODE),
    GEN_IMPORT(Nav_CheckFailedNodes, G_NAV_CHECKFAILEDNODES),
    GEN_IMPORT(Nav_AddFailedNode, G_NAV_ADDFAILEDNODE),
    GEN_IMPORT(Nav_NodeFailed, G_NAV_NODEFAILED),
    GEN_IMPORT(Nav_NodesAreNeighbors, G_NAV_NODESARENEIGHBORS),
    GEN_IMPORT(Nav_ClearFailedEdge, G_NAV_CLEARFAILEDEDGE),
    GEN_IMPORT(Nav_ClearAllFailedEdges, G_NAV_CLEARALLFAILEDEDGES),
    GEN_IMPORT(Nav_EdgeFailed, G_NAV_EDGEFAILED),
    GEN_IMPORT(Nav_AddFailedEdge, G_NAV_ADDFAILEDEDGE),
    GEN_IMPORT(Nav_CheckFailedEdge, G_NAV_CHECKFAILEDEDGE),
    GEN_IMPORT(Nav_CheckAllFailedEdges, G_NAV_CHECKALLFAILEDEDGES),
    GEN_IMPORT(Nav_RouteBlocked, G_NAV_ROUTEBLOCKED),
    GEN_IMPORT(Nav_GetBestNodeAltRoute, G_NAV_GETBESTNODEALTROUTE),
    GEN_IMPORT(Nav_GetBestNodeAltRoute2, G_NAV_GETBESTNODEALT2),
    GEN_IMPORT(Nav_GetBestPathBetweenEnts, G_NAV_GETBESTPATHBETWEENENTS),
    GEN_IMPORT(Nav_GetNodeRadius, G_NAV_GETNODERADIUS),
    GEN_IMPORT(Nav_CheckBlockedEdges, G_NAV_CHECKBLOCKEDEDGES),
    GEN_IMPORT(Nav_ClearCheckedNodes, G_NAV_CLEARCHECKEDNODES),
    GEN_IMPORT(Nav_CheckedNode, G_NAV_CHECKEDNODE),
    GEN_IMPORT(Nav_SetCheckedNode, G_NAV_SETCHECKEDNODE),
    GEN_IMPORT(Nav_FlagAllNodes, G_NAV_FLAGALLNODES),
    GEN_IMPORT(Nav_GetPathsCalculated, G_NAV_GETPATHSCALCULATED),
    GEN_IMPORT(Nav_SetPathsCalculated, G_NAV_SETPATHSCALCULATED),
    GEN_IMPORT(BotAllocateClient, G_BOT_ALLOCATE_CLIENT),
    GEN_IMPORT(BotFreeClient, G_BOT_FREE_CLIENT),
    GEN_IMPORT(BotLoadCharacter, BOTLIB_AI_LOAD_CHARACTER),
    GEN_IMPORT(BotFreeCharacter, BOTLIB_AI_FREE_CHARACTER),
    GEN_IMPORT(Characteristic_Float, BOTLIB_AI_CHARACTERISTIC_FLOAT),
    GEN_IMPORT(Characteristic_BFloat, BOTLIB_AI_CHARACTERISTIC_BFLOAT),
    GEN_IMPORT(Characteristic_Integer, BOTLIB_AI_CHARACTERISTIC_INTEGER),
    GEN_IMPORT(Characteristic_BInteger, BOTLIB_AI_CHARACTERISTIC_BINTEGER),
    GEN_IMPORT(Characteristic_String, BOTLIB_AI_CHARACTERISTIC_STRING),
    GEN_IMPORT(BotAllocChatState, BOTLIB_AI_ALLOC_CHAT_STATE),
    GEN_IMPORT(BotFreeChatState, BOTLIB_AI_FREE_CHAT_STATE),
    GEN_IMPORT(BotQueueConsoleMessage, BOTLIB_AI_QUEUE_CONSOLE_MESSAGE),
    GEN_IMPORT(BotRemoveConsoleMessage, BOTLIB_AI_REMOVE_CONSOLE_MESSAGE),
    GEN_IMPORT(BotNextConsoleMessage, BOTLIB_AI_NEXT_CONSOLE_MESSAGE),
    GEN_IMPORT(BotNumConsoleMessages, BOTLIB_AI_NUM_CONSOLE_MESSAGE),
    GEN_IMPORT(BotInitialChat, BOTLIB_AI_INITIAL_CHAT),
    GEN_IMPORT(BotReplyChat, BOTLIB_AI_REPLY_CHAT),
    GEN_IMPORT(BotChatLength, BOTLIB_AI_CHAT_LENGTH),
    GEN_IMPORT(BotEnterChat, BOTLIB_AI_ENTER_CHAT),
    GEN_IMPORT(StringContains, BOTLIB_AI_STRING_CONTAINS),
    GEN_IMPORT(BotFindMatch, BOTLIB_AI_FIND_MATCH),
    GEN_IMPORT(BotMatchVariable, BOTLIB_AI_MATCH_VARIABLE),
    GEN_IMPORT(UnifyWhiteSpaces, BOTLIB_AI_UNIFY_WHITE_SPACES),
    GEN_IMPORT(BotReplaceSynonyms, BOTLIB_AI_REPLACE_SYNONYMS),
    GEN_IMPORT(BotLoadChatFile, BOTLIB_AI_LOAD_CHAT_FILE),
    GEN_IMPORT(BotSetChatGender, BOTLIB_AI_SET_CHAT_GENDER),
    GEN_IMPORT(BotSetChatName, BOTLIB_AI_SET_CHAT_NAME),
    GEN_IMPORT(BotResetGoalState, BOTLIB_AI_RESET_GOAL_STATE),
    GEN_IMPORT(BotResetAvoidGoals, BOTLIB_AI_RESET_AVOID_GOALS),
    GEN_IMPORT(BotPushGoal, BOTLIB_AI_PUSH_GOAL),
    GEN_IMPORT(BotPopGoal, BOTLIB_AI_POP_GOAL),
    GEN_IMPORT(BotEmptyGoalStack, BOTLIB_AI_EMPTY_GOAL_STACK),
    GEN_IMPORT(BotDumpAvoidGoals, BOTLIB_AI_DUMP_AVOID_GOALS),
    GEN_IMPORT(BotDumpGoalStack, BOTLIB_AI_DUMP_GOAL_STACK),
    GEN_IMPORT(BotGoalName, BOTLIB_AI_GOAL_NAME),
    GEN_IMPORT(BotGetTopGoal, BOTLIB_AI_GET_TOP_GOAL),
    GEN_IMPORT(BotGetSecondGoal, BOTLIB_AI_GET_SECOND_GOAL),
    GEN_IMPORT(BotChooseLTGItem, BOTLIB_AI_CHOOSE_LTG_ITEM),
    GEN_IMPORT(BotChooseNBGItem, BOTLIB_AI_CHOOSE_NBG_ITEM),
    GEN_IMPORT(BotTouchingGoal, BOTLIB_AI_TOUCHING_GOAL),
    GEN_IMPORT(BotItemGoalInVisButNotVisible, BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE),
    GEN_IMPORT(BotGetLevelItemGoal, BOTLIB_AI_GET_LEVEL_ITEM_GOAL),
    GEN_IMPORT(BotAvoidGoalTime, BOTLIB_AI_AVOID_GOAL_TIME),
    GEN_IMPORT(BotInitLevelItems, BOTLIB_AI_INIT_LEVEL_ITEMS),
    GEN_IMPORT(BotUpdateEntityItems, BOTLIB_AI_UPDATE_ENTITY_ITEMS),
    GEN_IMPORT(BotLoadItemWeights, BOTLIB_AI_LOAD_ITEM_WEIGHTS),
    GEN_IMPORT(BotFreeItemWeights, BOTLIB_AI_FREE_ITEM_WEIGHTS),
    GEN_IMPORT(BotSaveGoalFuzzyLogic, BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC),
    GEN_IMPORT(BotAllocGoalState, BOTLIB_AI_ALLOC_GOAL_STATE),
    GEN_IMPORT(BotFreeGoalState, BOTLIB_AI_FREE_GOAL_STATE),
    GEN_IMPORT(BotResetMoveState, BOTLIB_AI_RESET_MOVE_STATE),
    GEN_IMPORT(BotMoveToGoal, BOTLIB_AI_MOVE_TO_GOAL),
    GEN_IMPORT(BotMoveInDirection, BOTLIB_AI_MOVE_IN_DIRECTION),
    GEN_IMPORT(BotResetAvoidReach, BOTLIB_AI_RESET_AVOID_REACH),
    GEN_IMPORT(BotResetLastAvoidReach, BOTLIB_AI_RESET_LAST_AVOID_REACH),
    GEN_IMPORT(BotReachabilityArea, BOTLIB_AI_REACHABILITY_AREA),
    GEN_IMPORT(BotMovementViewTarget, BOTLIB_AI_MOVEMENT_VIEW_TARGET),
    GEN_IMPORT(BotAllocMoveState, BOTLIB_AI_ALLOC_MOVE_STATE),
    GEN_IMPORT(BotFreeMoveState, BOTLIB_AI_FREE_MOVE_STATE),
    GEN_IMPORT(BotInitMoveState, BOTLIB_AI_INIT_MOVE_STATE),
    GEN_IMPORT(BotChooseBestFightWeapon, BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON),
    GEN_IMPORT(BotGetWeaponInfo, BOTLIB_AI_GET_WEAPON_INFO),
    GEN_IMPORT(BotLoadWeaponWeights, BOTLIB_AI_LOAD_WEAPON_WEIGHTS),
    GEN_IMPORT(BotAllocWeaponState, BOTLIB_AI_ALLOC_WEAPON_STATE),
    GEN_IMPORT(BotFreeWeaponState, BOTLIB_AI_FREE_WEAPON_STATE),
    GEN_IMPORT(BotResetWeaponState, BOTLIB_AI_RESET_WEAPON_STATE),
    GEN_IMPORT(GeneticParentsAndChildSelection, BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION),
    GEN_IMPORT(BotInterbreedGoalFuzzyLogic, BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC),
    GEN_IMPORT(BotMutateGoalFuzzyLogic, BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC),
    GEN_IMPORT(BotGetNextCampSpotGoal, BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL),
    GEN_IMPORT(BotGetMapLocationGoal, BOTLIB_AI_GET_MAP_LOCATION_GOAL),
    GEN_IMPORT(BotNumInitialChats, BOTLIB_AI_NUM_INITIAL_CHATS),
    GEN_IMPORT(BotGetChatMessage, BOTLIB_AI_GET_CHAT_MESSAGE),
    GEN_IMPORT(BotRemoveFromAvoidGoals, BOTLIB_AI_REMOVE_FROM_AVOID_GOALS),
    GEN_IMPORT(BotPredictVisiblePosition, BOTLIB_AI_PREDICT_VISIBLE_POSITION),
    GEN_IMPORT(BotSetAvoidGoalTime, BOTLIB_AI_SET_AVOID_GOAL_TIME),
    GEN_IMPORT(BotAddAvoidSpot, BOTLIB_AI_ADD_AVOID_SPOT),
    GEN_IMPORT(BotLibSetup, BOTLIB_SETUP),
    GEN_IMPORT(BotLibShutdown, BOTLIB_SHUTDOWN),
    GEN_IMPORT(BotLibVarSet, BOTLIB_LIBVAR_SET),
    GEN_IMPORT(BotLibVarGet, BOTLIB_LIBVAR_GET),
    GEN_IMPORT(BotLibDefine, BOTLIB_PC_ADD_GLOBAL_DEFINE),
    GEN_IMPORT(BotLibStartFrame, BOTLIB_START_FRAME),
    GEN_IMPORT(BotLibLoadMap, BOTLIB_LOAD_MAP),
    GEN_IMPORT(BotLibUpdateEntity, BOTLIB_UPDATENTITY),
    GEN_IMPORT(BotLibTest, BOTLIB_TEST),
    GEN_IMPORT(BotGetSnapshotEntity, BOTLIB_GET_SNAPSHOT_ENTITY),
    GEN_IMPORT(BotGetServerCommand, BOTLIB_GET_CONSOLE_MESSAGE),
    GEN_IMPORT(BotUserCommand, BOTLIB_USER_COMMAND),
    GEN_IMPORT(BotUpdateWaypoints, G_BOT_UPDATEWAYPOINTS),
    GEN_IMPORT(BotCalculatePaths, G_BOT_CALCULATEPATHS),
    GEN_IMPORT(AAS_EnableRoutingArea, BOTLIB_AAS_ENABLE_ROUTING_AREA),
    GEN_IMPORT(AAS_BBoxAreas, BOTLIB_AAS_BBOX_AREAS),
    GEN_IMPORT(AAS_AreaInfo, BOTLIB_AAS_AREA_INFO),
    GEN_IMPORT(AAS_EntityInfo, BOTLIB_AAS_ENTITY_INFO),
    GEN_IMPORT(AAS_Initialized, BOTLIB_AAS_INITIALIZED),
    GEN_IMPORT(AAS_PresenceTypeBoundingBox, BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX),
    GEN_IMPORT(AAS_Time, BOTLIB_AAS_TIME),
    GEN_IMPORT(AAS_PointAreaNum, BOTLIB_AAS_POINT_AREA_NUM),
    GEN_IMPORT(AAS_TraceAreas, BOTLIB_AAS_TRACE_AREAS),
    GEN_IMPORT(AAS_PointContents, BOTLIB_AAS_POINT_CONTENTS),
    GEN_IMPORT(AAS_NextBSPEntity, BOTLIB_AAS_NEXT_BSP_ENTITY),
    GEN_IMPORT(AAS_ValueForBSPEpairKey, BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY),
    GEN_IMPORT(AAS_VectorForBSPEpairKey, BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY),
    GEN_IMPORT(AAS_FloatForBSPEpairKey, BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY),
    GEN_IMPORT(AAS_IntForBSPEpairKey, BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY),
    GEN_IMPORT(AAS_AreaReachability, BOTLIB_AAS_AREA_REACHABILITY),
    GEN_IMPORT(AAS_AreaTravelTimeToGoalArea, BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA),
    GEN_IMPORT(AAS_Swimming, BOTLIB_AAS_SWIMMING),
    GEN_IMPORT(AAS_PredictClientMovement, BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT),
    GEN_IMPORT(AAS_AlternativeRouteGoals, BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL),
    GEN_IMPORT(AAS_PredictRoute, BOTLIB_AAS_PREDICT_ROUTE),
    GEN_IMPORT(AAS_PointReachabilityAreaIndex, BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX),
    GEN_IMPORT(EA_Say, BOTLIB_EA_SAY),
    GEN_IMPORT(EA_SayTeam, BOTLIB_EA_SAY_TEAM),
    GEN_IMPORT(EA_Command, BOTLIB_EA_COMMAND),
    GEN_IMPORT(EA_Action, BOTLIB_EA_ACTION),
    GEN_IMPORT(EA_Gesture, BOTLIB_EA_GESTURE),
    GEN_IMPORT(EA_Talk, BOTLIB_EA_TALK),
    GEN_IMPORT(EA_Attack, BOTLIB_EA_ATTACK),
    GEN_IMPORT(EA_Alt_Attack, BOTLIB_EA_ALT_ATTACK),
    GEN_IMPORT(EA_ForcePower, BOTLIB_EA_FORCEPOWER),
    GEN_IMPORT(EA_Use, BOTLIB_EA_USE),
    GEN_IMPORT(EA_Respawn, BOTLIB_EA_RESPAWN),
    GEN_IMPORT(EA_Crouch, BOTLIB_EA_CROUCH),
    GEN_IMPORT(EA_MoveUp, BOTLIB_EA_MOVE_UP),
    GEN_IMPORT(EA_MoveDown, BOTLIB_EA_MOVE_DOWN),
    GEN_IMPORT(EA_MoveForward, BOTLIB_EA_MOVE_FORWARD),
    GEN_IMPORT(EA_MoveBack, BOTLIB_EA_MOVE_BACK),
    GEN_IMPORT(EA_MoveLeft, BOTLIB_EA_MOVE_LEFT),
    GEN_IMPORT(EA_MoveRight, BOTLIB_EA_MOVE_RIGHT),
    GEN_IMPORT(EA_SelectWeapon, BOTLIB_EA_SELECT_WEAPON),
    GEN_IMPORT(EA_Jump, BOTLIB_EA_JUMP),
    GEN_IMPORT(EA_DelayedJump, BOTLIB_EA_DELAYED_JUMP),
    GEN_IMPORT(EA_Move, BOTLIB_EA_MOVE),
    GEN_IMPORT(EA_View, BOTLIB_EA_VIEW),
    GEN_IMPORT(EA_EndRegular, BOTLIB_EA_END_REGULAR),
    GEN_IMPORT(EA_GetInput, BOTLIB_EA_GET_INPUT),
    GEN_IMPORT(EA_ResetInput, BOTLIB_EA_RESET_INPUT),
    GEN_IMPORT(PC_LoadSource, BOTLIB_PC_LOAD_SOURCE),
    GEN_IMPORT(PC_FreeSource, BOTLIB_PC_FREE_SOURCE),
    GEN_IMPORT(PC_ReadToken, BOTLIB_PC_READ_TOKEN),
    GEN_IMPORT(PC_SourceFileAndLine, BOTLIB_PC_SOURCE_FILE_AND_LINE),
    GEN_IMPORT(R_RegisterSkin, G_R_REGISTERSKIN),
    GEN_IMPORT(SetActiveSubBSP, G_SET_ACTIVE_SUBBSP),
    GEN_IMPORT(CM_RegisterTerrain, G_CM_REGISTER_TERRAIN),
    GEN_IMPORT(RMG_Init, G_RMG_INIT),
    GEN_IMPORT(G2API_ListModelBones, G_G2_LISTBONES),
    GEN_IMPORT(G2API_ListModelSurfaces, G_G2_LISTSURFACES),
    GEN_IMPORT(G2API_HaveWeGhoul2Models, G_G2_HAVEWEGHOULMODELS),
    GEN_IMPORT(G2API_SetGhoul2ModelIndexes, G_G2_SETMODELS),
    GEN_IMPORT(G2API_GetBoltMatrix, G_G2_GETBOLT),
    GEN_IMPORT(G2API_GetBoltMatrix_NoReconstruct, G_G2_GETBOLT_NOREC),
    GEN_IMPORT(G2API_GetBoltMatrix_NoRecNoRot, G_G2_GETBOLT_NOREC_NOROT),
    GEN_IMPORT(G2API_InitGhoul2Model, G_G2_INITGHOUL2MODEL),
    GEN_IMPORT(G2API_SetSkin, G_G2_SETSKIN),
    GEN_IMPORT(G2API_Ghoul2Size, G_G2_SIZE),
    GEN_IMPORT(G2API_AddBolt, G_G2_ADDBOLT),
    GEN_IMPORT(G2API_SetBoltInfo, G_G2_SETBOLTINFO),
    GEN_IMPORT(G2API_SetBoneAngles, G_G2_ANGLEOVERRIDE),
    GEN_IMPORT(G2API_SetBoneAnim, G_G2_PLAYANIM),
    GEN_IMPORT(G2API_GetBoneAnim, G_G2_GETBONEANIM),
    GEN_IMPORT(G2API_GetGLAName, G_G2_GETGLANAME),
    GEN_IMPORT(G2API_CopyGhoul2Instance, G_G2_COPYGHOUL2INSTANCE),
    GEN_IMPORT(G2API_CopySpecificGhoul2Model, G_G2_COPYSPECIFICGHOUL2MODEL),
    GEN_IMPORT(G2API_DuplicateGhoul2Instance, G_G2_DUPLICATEGHOUL2INSTANCE),
    GEN_IMPORT(G2API_HasGhoul2ModelOnIndex, G_G2_HASGHOUL2MODELONINDEX),
    GEN_IMPORT(G2API_RemoveGhoul2Model, G_G2_REMOVEGHOUL2MODEL),
    GEN_IMPORT(G2API_RemoveGhoul2Models, G_G2_REMOVEGHOUL2MODELS),
    GEN_IMPORT(G2API_CleanGhoul2Models, G_G2_CLEANMODELS),
    GEN_IMPORT(G2API_CollisionDetect, G_G2_COLLISIONDETECT),
    GEN_IMPORT(G2API_CollisionDetectCache, G_G2_COLLISIONDETECTCACHE),
    GEN_IMPORT(G2API_SetRootSurface, G_G2_SETROOTSURFACE),
    GEN_IMPORT(G2API_SetSurfaceOnOff, G_G2_SETSURFACEONOFF),
    GEN_IMPORT(G2API_SetNewOrigin, G_G2_SETNEWORIGIN),
    GEN_IMPORT(G2API_DoesBoneExist, G_G2_DOESBONEEXIST),
    GEN_IMPORT(G2API_GetSurfaceRenderStatus, G_G2_GETSURFACERENDERSTATUS),
    GEN_IMPORT(G2API_AbsurdSmoothing, G_G2_ABSURDSMOOTHING),
    GEN_IMPORT(G2API_SetRagDoll, G_G2_SETRAGDOLL),
    GEN_IMPORT(G2API_AnimateG2Models, G_G2_ANIMATEG2MODELS),
    GEN_IMPORT(G2API_RagPCJConstraint, G_G2_RAGPCJCONSTRAINT),
    GEN_IMPORT(G2API_RagPCJGradientSpeed, G_G2_RAGPCJGRADIENTSPEED),
    GEN_IMPORT(G2API_RagEffectorGoal, G_G2_RAGEFFECTORGOAL),
    GEN_IMPORT(G2API_GetRagBonePos, G_G2_GETRAGBONEPOS),
    GEN_IMPORT(G2API_RagEffectorKick, G_G2_RAGEFFECTORKICK),
    GEN_IMPORT(G2API_RagForceSolve, G_G2_RAGFORCESOLVE),
    GEN_IMPORT(G2API_SetBoneIKState, G_G2_SETBONEIKSTATE),
    GEN_IMPORT(G2API_IKMove, G_G2_IKMOVE),
    GEN_IMPORT(G2API_RemoveBone, G_G2_REMOVEBONE),
    GEN_IMPORT(G2API_AttachInstanceToEntNum, G_G2_ATTACHINSTANCETOENTNUM),
    GEN_IMPORT(G2API_ClearAttachedInstance, G_G2_CLEARATTACHEDINSTANCE),
    GEN_IMPORT(G2API_CleanEntAttachments, G_G2_CLEANENTATTACHMENTS),
    GEN_IMPORT(G2API_OverrideServer, G_G2_OVERRIDESERVER),
    GEN_IMPORT(G2API_GetSurfaceName, G_G2_GETSURFACENAME),
};

// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
    GEN_EXPORT(InitGame, GAME_INIT),
    GEN_EXPORT(ShutdownGame, GAME_SHUTDOWN),
    GEN_EXPORT(ClientConnect, GAME_CLIENT_CONNECT),
    GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
    GEN_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED),
    GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
    GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
    GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
    GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
    GEN_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND),
    GEN_EXPORT(BotAIStartFrame, BOTAI_START_FRAME),
    GEN_EXPORT(ROFF_NotetrackCallback, GAME_ROFF_NOTETRACK_CALLBACK),
    GEN_EXPORT(SpawnRMGEntity, GAME_SPAWN_RMG_ENTITY),
    GEN_EXPORT(ICARUS_PlaySound, GAME_ICARUS_PLAYSOUND),
    GEN_EXPORT(ICARUS_Set, GAME_ICARUS_SET),
    GEN_EXPORT(ICARUS_Lerp2Pos, GAME_ICARUS_LERP2POS),
    GEN_EXPORT(ICARUS_Lerp2Origin, GAME_ICARUS_LERP2ORIGIN),
    GEN_EXPORT(ICARUS_Lerp2Angles, GAME_ICARUS_LERP2ANGLES),
    GEN_EXPORT(ICARUS_GetTag, GAME_ICARUS_GETTAG),
    GEN_EXPORT(ICARUS_Lerp2Start, GAME_ICARUS_LERP2START),
    GEN_EXPORT(ICARUS_Lerp2End, GAME_ICARUS_LERP2END),
    GEN_EXPORT(ICARUS_Use, GAME_ICARUS_USE),
    GEN_EXPORT(ICARUS_Kill, GAME_ICARUS_KILL),
    GEN_EXPORT(ICARUS_Remove, GAME_ICARUS_REMOVE),
    GEN_EXPORT(ICARUS_Play, GAME_ICARUS_PLAY),
    GEN_EXPORT(ICARUS_GetFloat, GAME_ICARUS_GETFLOAT),
    GEN_EXPORT(ICARUS_GetVector, GAME_ICARUS_GETVECTOR),
    GEN_EXPORT(ICARUS_GetString, GAME_ICARUS_GETSTRING),
    GEN_EXPORT(ICARUS_SoundIndex, GAME_ICARUS_SOUNDINDEX),
    GEN_EXPORT(ICARUS_GetSetIDForString, GAME_ICARUS_GETSETIDFORSTRING),
    GEN_EXPORT(NAV_ClearPathToPoint, GAME_NAV_CLEARPATHTOPOINT),
    GEN_EXPORT(NPC_ClearLOS2, GAME_NAV_CLEARLOS),
    GEN_EXPORT(NAVNEW_ClearPathBetweenPoints, GAME_NAV_CLEARPATHBETWEENPOINTS),
    GEN_EXPORT(NAV_CheckNodeFailedForEnt, GAME_NAV_CHECKNODEFAILEDFORENT),
    GEN_EXPORT(NAV_EntIsUnlockedDoor, GAME_NAV_ENTISUNLOCKEDDOOR),
    GEN_EXPORT(NAV_EntIsDoor, GAME_NAV_ENTISDOOR),
    GEN_EXPORT(NAV_EntIsBreakable, GAME_NAV_ENTISBREAKABLE),
    GEN_EXPORT(NAV_EntIsRemovableUsable, GAME_NAV_ENTISREMOVABLEUSABLE),
    GEN_EXPORT(NAV_FindCombatPointWaypoints, GAME_NAV_FINDCOMBATPOINTWAYPOINTS),
    GEN_EXPORT(BG_GetItemIndexByTag, GAME_GETITEMINDEXBYTAG),
};


// wrapper syscall function that calls actual engine func from orig_import or orig_syscall
// this is how QMM and plugins will call into the engine
static intptr_t JAMP_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_syscall({} {}) called\n", JAMP_eng_msg_names(cmd), cmd);
#endif

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    // if QMM was loaded with the official JAMP or OpenJK "legacy" API (which shouldn't happen, but whatever)
    if (orig_syscall) {
        switch (cmd) {
        // handle special cmds which QMM uses but JAMP doesn't have an analogue for
        case G_ARGS: {
            // quake2: char* (*args)(void);
            static std::string s;
            static char buf[MAX_STRING_CHARS];
            s = "";
            int i = 1;
            while (i < orig_syscall(G_ARGC)) {
                orig_syscall(G_ARGV, i, buf, sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
                if (i != 1)
                    s += " ";
                s += buf;
            }
            ret = (intptr_t)s.c_str();
            break;
        }

        default:
            // all normal engine functions go to engine
            ret = orig_syscall(cmd, QMM_PUT_SYSCALL_ARGS());
        }
    }
    // if QMM was loaded with the OpenJK "new" API
    else if (orig_import.Print) {
        switch (cmd) {
            ROUTE_IMPORT(Print, G_PRINT);
            ROUTE_IMPORT(Error, G_ERROR);
            ROUTE_IMPORT(Milliseconds, G_MILLISECONDS);
            ROUTE_IMPORT(PrecisionTimerStart, G_PRECISIONTIMER_START);
            ROUTE_IMPORT(PrecisionTimerEnd, G_PRECISIONTIMER_END);
            ROUTE_IMPORT(SV_RegisterSharedMemory, G_SET_SHARED_BUFFER);
            ROUTE_IMPORT(RealTime, G_REAL_TIME);
            ROUTE_IMPORT(TrueMalloc, G_TRUEMALLOC);
            ROUTE_IMPORT(TrueFree, G_TRUEFREE);
            ROUTE_IMPORT(SnapVector, G_SNAPVECTOR);
            ROUTE_IMPORT(Cvar_Register, G_CVAR_REGISTER);
            ROUTE_IMPORT(Cvar_Set, G_CVAR_SET);
            ROUTE_IMPORT(Cvar_Update, G_CVAR_UPDATE);
            ROUTE_IMPORT(Cvar_VariableIntegerValue, G_CVAR_VARIABLE_INTEGER_VALUE);
            ROUTE_IMPORT(Cvar_VariableStringBuffer, G_CVAR_VARIABLE_STRING_BUFFER);
            ROUTE_IMPORT(Argc, G_ARGC);
            ROUTE_IMPORT(Argv, G_ARGV);
            ROUTE_IMPORT(FS_Close, G_FS_FCLOSE_FILE);
            ROUTE_IMPORT(FS_GetFileList, G_FS_GETFILELIST);
            ROUTE_IMPORT(FS_Open, G_FS_FOPEN_FILE);
            ROUTE_IMPORT(FS_Read, G_FS_READ);
            ROUTE_IMPORT(FS_Write, G_FS_WRITE);
            ROUTE_IMPORT(AdjustAreaPortalState, G_ADJUST_AREA_PORTAL_STATE);
            ROUTE_IMPORT(AreasConnected, G_AREAS_CONNECTED);
            ROUTE_IMPORT(DebugPolygonCreate, G_DEBUG_POLYGON_CREATE);
            ROUTE_IMPORT(DebugPolygonDelete, G_DEBUG_POLYGON_DELETE);
            ROUTE_IMPORT(DropClient, G_DROP_CLIENT);
            ROUTE_IMPORT(EntitiesInBox, G_ENTITIES_IN_BOX);
            ROUTE_IMPORT(EntityContact, G_ENTITY_CONTACT);
            ROUTE_IMPORT(GetConfigstring, G_GET_CONFIGSTRING);
            ROUTE_IMPORT(GetEntityToken, G_GET_ENTITY_TOKEN);
            ROUTE_IMPORT(GetServerinfo, G_GET_SERVERINFO);
            ROUTE_IMPORT(GetUsercmd, G_GET_USERCMD);
            ROUTE_IMPORT(GetUserinfo, G_GET_USERINFO);
            ROUTE_IMPORT(InPVS, G_IN_PVS);
            ROUTE_IMPORT(InPVSIgnorePortals, G_IN_PVS_IGNORE_PORTALS);
            ROUTE_IMPORT(LinkEntity, G_LINKENTITY);
            ROUTE_IMPORT(LocateGameData, G_LOCATE_GAME_DATA);
            ROUTE_IMPORT(PointContents, G_POINT_CONTENTS);
            ROUTE_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND);
            ROUTE_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND);
            ROUTE_IMPORT(SetBrushModel, G_SET_BRUSH_MODEL);
            ROUTE_IMPORT(SetConfigstring, G_SET_CONFIGSTRING);
            ROUTE_IMPORT(SetServerCull, G_SET_SERVER_CULL);
            ROUTE_IMPORT(SetUserinfo, G_SET_USERINFO);
            ROUTE_IMPORT(SiegePersSet, G_SIEGEPERSSET);
            ROUTE_IMPORT(SiegePersGet, G_SIEGEPERSGET);
            ROUTE_IMPORT(Trace, G_TRACE);
            ROUTE_IMPORT(UnlinkEntity, G_UNLINKENTITY);
            ROUTE_IMPORT(ROFF_Clean, G_ROFF_CLEAN);
            ROUTE_IMPORT(ROFF_UpdateEntities, G_ROFF_UPDATE_ENTITIES);
            ROUTE_IMPORT(ROFF_Cache, G_ROFF_CACHE);
            ROUTE_IMPORT(ROFF_Play, G_ROFF_PLAY);
            ROUTE_IMPORT(ROFF_Purge_Ent, G_ROFF_PURGE_ENT);
            ROUTE_IMPORT(ICARUS_RunScript, G_ICARUS_RUNSCRIPT);
            ROUTE_IMPORT(ICARUS_RegisterScript, G_ICARUS_REGISTERSCRIPT);
            ROUTE_IMPORT(ICARUS_Init, G_ICARUS_INIT);
            ROUTE_IMPORT(ICARUS_ValidEnt, G_ICARUS_VALIDENT);
            ROUTE_IMPORT(ICARUS_IsInitialized, G_ICARUS_ISINITIALIZED);
            ROUTE_IMPORT(ICARUS_MaintainTaskManager, G_ICARUS_MAINTAINTASKMANAGER);
            ROUTE_IMPORT(ICARUS_IsRunning, G_ICARUS_ISRUNNING);
            ROUTE_IMPORT(ICARUS_TaskIDPending, G_ICARUS_TASKIDPENDING);
            ROUTE_IMPORT(ICARUS_InitEnt, G_ICARUS_INITENT);
            ROUTE_IMPORT(ICARUS_FreeEnt, G_ICARUS_FREEENT);
            ROUTE_IMPORT(ICARUS_AssociateEnt, G_ICARUS_ASSOCIATEENT);
            ROUTE_IMPORT(ICARUS_Shutdown, G_ICARUS_SHUTDOWN);
            ROUTE_IMPORT(ICARUS_TaskIDSet, G_ICARUS_TASKIDSET);
            ROUTE_IMPORT(ICARUS_TaskIDComplete, G_ICARUS_TASKIDCOMPLETE);
            ROUTE_IMPORT(ICARUS_SetVar, G_ICARUS_SETVAR);
            ROUTE_IMPORT(ICARUS_VariableDeclared, G_ICARUS_VARIABLEDECLARED);
            ROUTE_IMPORT(ICARUS_GetFloatVariable, G_ICARUS_GETFLOATVARIABLE);
            ROUTE_IMPORT(ICARUS_GetStringVariable, G_ICARUS_GETSTRINGVARIABLE);
            ROUTE_IMPORT(ICARUS_GetVectorVariable, G_ICARUS_GETVECTORVARIABLE);
            ROUTE_IMPORT(Nav_Init, G_NAV_INIT);
            ROUTE_IMPORT(Nav_Free, G_NAV_FREE);
            ROUTE_IMPORT(Nav_Load, G_NAV_LOAD);
            ROUTE_IMPORT(Nav_Save, G_NAV_SAVE);
            ROUTE_IMPORT(Nav_AddRawPoint, G_NAV_ADDRAWPOINT);
            ROUTE_IMPORT(Nav_CalculatePaths, G_NAV_CALCULATEPATHS);
            ROUTE_IMPORT(Nav_HardConnect, G_NAV_HARDCONNECT);
            ROUTE_IMPORT(Nav_ShowNodes, G_NAV_SHOWNODES);
            ROUTE_IMPORT(Nav_ShowEdges, G_NAV_SHOWEDGES);
            ROUTE_IMPORT(Nav_ShowPath, G_NAV_SHOWPATH);
            ROUTE_IMPORT(Nav_GetNearestNode, G_NAV_GETNEARESTNODE);
            ROUTE_IMPORT(Nav_GetBestNode, G_NAV_GETBESTNODE);
            ROUTE_IMPORT(Nav_GetNodePosition, G_NAV_GETNODEPOSITION);
            ROUTE_IMPORT(Nav_GetNodeNumEdges, G_NAV_GETNODENUMEDGES);
            ROUTE_IMPORT(Nav_GetNodeEdge, G_NAV_GETNODEEDGE);
            ROUTE_IMPORT(Nav_GetNumNodes, G_NAV_GETNUMNODES);
            ROUTE_IMPORT(Nav_Connected, G_NAV_CONNECTED);
            ROUTE_IMPORT(Nav_GetPathCost, G_NAV_GETPATHCOST);
            ROUTE_IMPORT(Nav_GetEdgeCost, G_NAV_GETEDGECOST);
            ROUTE_IMPORT(Nav_GetProjectedNode, G_NAV_GETPROJECTEDNODE);
            ROUTE_IMPORT(Nav_CheckFailedNodes, G_NAV_CHECKFAILEDNODES);
            ROUTE_IMPORT(Nav_AddFailedNode, G_NAV_ADDFAILEDNODE);
            ROUTE_IMPORT(Nav_NodeFailed, G_NAV_NODEFAILED);
            ROUTE_IMPORT(Nav_NodesAreNeighbors, G_NAV_NODESARENEIGHBORS);
            ROUTE_IMPORT(Nav_ClearFailedEdge, G_NAV_CLEARFAILEDEDGE);
            ROUTE_IMPORT(Nav_ClearAllFailedEdges, G_NAV_CLEARALLFAILEDEDGES);
            ROUTE_IMPORT(Nav_EdgeFailed, G_NAV_EDGEFAILED);
            ROUTE_IMPORT(Nav_AddFailedEdge, G_NAV_ADDFAILEDEDGE);
            ROUTE_IMPORT(Nav_CheckFailedEdge, G_NAV_CHECKFAILEDEDGE);
            ROUTE_IMPORT(Nav_CheckAllFailedEdges, G_NAV_CHECKALLFAILEDEDGES);
            ROUTE_IMPORT(Nav_RouteBlocked, G_NAV_ROUTEBLOCKED);
            ROUTE_IMPORT(Nav_GetBestNodeAltRoute, G_NAV_GETBESTNODEALTROUTE);
            ROUTE_IMPORT(Nav_GetBestNodeAltRoute2, G_NAV_GETBESTNODEALT2);
            ROUTE_IMPORT(Nav_GetBestPathBetweenEnts, G_NAV_GETBESTPATHBETWEENENTS);
            ROUTE_IMPORT(Nav_GetNodeRadius, G_NAV_GETNODERADIUS);
            ROUTE_IMPORT(Nav_CheckBlockedEdges, G_NAV_CHECKBLOCKEDEDGES);
            ROUTE_IMPORT(Nav_ClearCheckedNodes, G_NAV_CLEARCHECKEDNODES);
            ROUTE_IMPORT(Nav_CheckedNode, G_NAV_CHECKEDNODE);
            ROUTE_IMPORT(Nav_SetCheckedNode, G_NAV_SETCHECKEDNODE);
            ROUTE_IMPORT(Nav_FlagAllNodes, G_NAV_FLAGALLNODES);
            ROUTE_IMPORT(Nav_GetPathsCalculated, G_NAV_GETPATHSCALCULATED);
            ROUTE_IMPORT(Nav_SetPathsCalculated, G_NAV_SETPATHSCALCULATED);
            ROUTE_IMPORT(BotAllocateClient, G_BOT_ALLOCATE_CLIENT);
            ROUTE_IMPORT(BotFreeClient, G_BOT_FREE_CLIENT);
            ROUTE_IMPORT(BotLoadCharacter, BOTLIB_AI_LOAD_CHARACTER);
            ROUTE_IMPORT(BotFreeCharacter, BOTLIB_AI_FREE_CHARACTER);
            ROUTE_IMPORT(Characteristic_Float, BOTLIB_AI_CHARACTERISTIC_FLOAT);
            ROUTE_IMPORT(Characteristic_BFloat, BOTLIB_AI_CHARACTERISTIC_BFLOAT);
            ROUTE_IMPORT(Characteristic_Integer, BOTLIB_AI_CHARACTERISTIC_INTEGER);
            ROUTE_IMPORT(Characteristic_BInteger, BOTLIB_AI_CHARACTERISTIC_BINTEGER);
            ROUTE_IMPORT(Characteristic_String, BOTLIB_AI_CHARACTERISTIC_STRING);
            ROUTE_IMPORT(BotAllocChatState, BOTLIB_AI_ALLOC_CHAT_STATE);
            ROUTE_IMPORT(BotFreeChatState, BOTLIB_AI_FREE_CHAT_STATE);
            ROUTE_IMPORT(BotQueueConsoleMessage, BOTLIB_AI_QUEUE_CONSOLE_MESSAGE);
            ROUTE_IMPORT(BotRemoveConsoleMessage, BOTLIB_AI_REMOVE_CONSOLE_MESSAGE);
            ROUTE_IMPORT(BotNextConsoleMessage, BOTLIB_AI_NEXT_CONSOLE_MESSAGE);
            ROUTE_IMPORT(BotNumConsoleMessages, BOTLIB_AI_NUM_CONSOLE_MESSAGE);
            ROUTE_IMPORT(BotInitialChat, BOTLIB_AI_INITIAL_CHAT);
            ROUTE_IMPORT(BotReplyChat, BOTLIB_AI_REPLY_CHAT);
            ROUTE_IMPORT(BotChatLength, BOTLIB_AI_CHAT_LENGTH);
            ROUTE_IMPORT(BotEnterChat, BOTLIB_AI_ENTER_CHAT);
            ROUTE_IMPORT(StringContains, BOTLIB_AI_STRING_CONTAINS);
            ROUTE_IMPORT(BotFindMatch, BOTLIB_AI_FIND_MATCH);
            ROUTE_IMPORT(BotMatchVariable, BOTLIB_AI_MATCH_VARIABLE);
            ROUTE_IMPORT(UnifyWhiteSpaces, BOTLIB_AI_UNIFY_WHITE_SPACES);
            ROUTE_IMPORT(BotReplaceSynonyms, BOTLIB_AI_REPLACE_SYNONYMS);
            ROUTE_IMPORT(BotLoadChatFile, BOTLIB_AI_LOAD_CHAT_FILE);
            ROUTE_IMPORT(BotSetChatGender, BOTLIB_AI_SET_CHAT_GENDER);
            ROUTE_IMPORT(BotSetChatName, BOTLIB_AI_SET_CHAT_NAME);
            ROUTE_IMPORT(BotResetGoalState, BOTLIB_AI_RESET_GOAL_STATE);
            ROUTE_IMPORT(BotResetAvoidGoals, BOTLIB_AI_RESET_AVOID_GOALS);
            ROUTE_IMPORT(BotPushGoal, BOTLIB_AI_PUSH_GOAL);
            ROUTE_IMPORT(BotPopGoal, BOTLIB_AI_POP_GOAL);
            ROUTE_IMPORT(BotEmptyGoalStack, BOTLIB_AI_EMPTY_GOAL_STACK);
            ROUTE_IMPORT(BotDumpAvoidGoals, BOTLIB_AI_DUMP_AVOID_GOALS);
            ROUTE_IMPORT(BotDumpGoalStack, BOTLIB_AI_DUMP_GOAL_STACK);
            ROUTE_IMPORT(BotGoalName, BOTLIB_AI_GOAL_NAME);
            ROUTE_IMPORT(BotGetTopGoal, BOTLIB_AI_GET_TOP_GOAL);
            ROUTE_IMPORT(BotGetSecondGoal, BOTLIB_AI_GET_SECOND_GOAL);
            ROUTE_IMPORT(BotChooseLTGItem, BOTLIB_AI_CHOOSE_LTG_ITEM);
            ROUTE_IMPORT(BotChooseNBGItem, BOTLIB_AI_CHOOSE_NBG_ITEM);
            ROUTE_IMPORT(BotTouchingGoal, BOTLIB_AI_TOUCHING_GOAL);
            ROUTE_IMPORT(BotItemGoalInVisButNotVisible, BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE);
            ROUTE_IMPORT(BotGetLevelItemGoal, BOTLIB_AI_GET_LEVEL_ITEM_GOAL);
            ROUTE_IMPORT(BotAvoidGoalTime, BOTLIB_AI_AVOID_GOAL_TIME);
            ROUTE_IMPORT(BotInitLevelItems, BOTLIB_AI_INIT_LEVEL_ITEMS);
            ROUTE_IMPORT(BotUpdateEntityItems, BOTLIB_AI_UPDATE_ENTITY_ITEMS);
            ROUTE_IMPORT(BotLoadItemWeights, BOTLIB_AI_LOAD_ITEM_WEIGHTS);
            ROUTE_IMPORT(BotFreeItemWeights, BOTLIB_AI_FREE_ITEM_WEIGHTS);
            ROUTE_IMPORT(BotSaveGoalFuzzyLogic, BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC);
            ROUTE_IMPORT(BotAllocGoalState, BOTLIB_AI_ALLOC_GOAL_STATE);
            ROUTE_IMPORT(BotFreeGoalState, BOTLIB_AI_FREE_GOAL_STATE);
            ROUTE_IMPORT(BotResetMoveState, BOTLIB_AI_RESET_MOVE_STATE);
            ROUTE_IMPORT(BotMoveToGoal, BOTLIB_AI_MOVE_TO_GOAL);
            ROUTE_IMPORT(BotMoveInDirection, BOTLIB_AI_MOVE_IN_DIRECTION);
            ROUTE_IMPORT(BotResetAvoidReach, BOTLIB_AI_RESET_AVOID_REACH);
            ROUTE_IMPORT(BotResetLastAvoidReach, BOTLIB_AI_RESET_LAST_AVOID_REACH);
            ROUTE_IMPORT(BotReachabilityArea, BOTLIB_AI_REACHABILITY_AREA);
            ROUTE_IMPORT(BotMovementViewTarget, BOTLIB_AI_MOVEMENT_VIEW_TARGET);
            ROUTE_IMPORT(BotAllocMoveState, BOTLIB_AI_ALLOC_MOVE_STATE);
            ROUTE_IMPORT(BotFreeMoveState, BOTLIB_AI_FREE_MOVE_STATE);
            ROUTE_IMPORT(BotInitMoveState, BOTLIB_AI_INIT_MOVE_STATE);
            ROUTE_IMPORT(BotChooseBestFightWeapon, BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON);
            ROUTE_IMPORT(BotGetWeaponInfo, BOTLIB_AI_GET_WEAPON_INFO);
            ROUTE_IMPORT(BotLoadWeaponWeights, BOTLIB_AI_LOAD_WEAPON_WEIGHTS);
            ROUTE_IMPORT(BotAllocWeaponState, BOTLIB_AI_ALLOC_WEAPON_STATE);
            ROUTE_IMPORT(BotFreeWeaponState, BOTLIB_AI_FREE_WEAPON_STATE);
            ROUTE_IMPORT(BotResetWeaponState, BOTLIB_AI_RESET_WEAPON_STATE);
            ROUTE_IMPORT(GeneticParentsAndChildSelection, BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION);
            ROUTE_IMPORT(BotInterbreedGoalFuzzyLogic, BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC);
            ROUTE_IMPORT(BotMutateGoalFuzzyLogic, BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC);
            ROUTE_IMPORT(BotGetNextCampSpotGoal, BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL);
            ROUTE_IMPORT(BotGetMapLocationGoal, BOTLIB_AI_GET_MAP_LOCATION_GOAL);
            ROUTE_IMPORT(BotNumInitialChats, BOTLIB_AI_NUM_INITIAL_CHATS);
            ROUTE_IMPORT(BotGetChatMessage, BOTLIB_AI_GET_CHAT_MESSAGE);
            ROUTE_IMPORT(BotRemoveFromAvoidGoals, BOTLIB_AI_REMOVE_FROM_AVOID_GOALS);
            ROUTE_IMPORT(BotPredictVisiblePosition, BOTLIB_AI_PREDICT_VISIBLE_POSITION);
            ROUTE_IMPORT(BotSetAvoidGoalTime, BOTLIB_AI_SET_AVOID_GOAL_TIME);
            ROUTE_IMPORT(BotAddAvoidSpot, BOTLIB_AI_ADD_AVOID_SPOT);
            ROUTE_IMPORT(BotLibSetup, BOTLIB_SETUP);
            ROUTE_IMPORT(BotLibShutdown, BOTLIB_SHUTDOWN);
            ROUTE_IMPORT(BotLibVarSet, BOTLIB_LIBVAR_SET);
            ROUTE_IMPORT(BotLibVarGet, BOTLIB_LIBVAR_GET);
            ROUTE_IMPORT(BotLibDefine, BOTLIB_PC_ADD_GLOBAL_DEFINE);
            ROUTE_IMPORT(BotLibStartFrame, BOTLIB_START_FRAME);
            ROUTE_IMPORT(BotLibLoadMap, BOTLIB_LOAD_MAP);
            ROUTE_IMPORT(BotLibUpdateEntity, BOTLIB_UPDATENTITY);
            ROUTE_IMPORT(BotLibTest, BOTLIB_TEST);
            ROUTE_IMPORT(BotGetSnapshotEntity, BOTLIB_GET_SNAPSHOT_ENTITY);
            ROUTE_IMPORT(BotGetServerCommand, BOTLIB_GET_CONSOLE_MESSAGE);
            ROUTE_IMPORT(BotUserCommand, BOTLIB_USER_COMMAND);
            ROUTE_IMPORT(BotUpdateWaypoints, G_BOT_UPDATEWAYPOINTS);
            ROUTE_IMPORT(BotCalculatePaths, G_BOT_CALCULATEPATHS);
            ROUTE_IMPORT(AAS_EnableRoutingArea, BOTLIB_AAS_ENABLE_ROUTING_AREA);
            ROUTE_IMPORT(AAS_BBoxAreas, BOTLIB_AAS_BBOX_AREAS);
            ROUTE_IMPORT(AAS_AreaInfo, BOTLIB_AAS_AREA_INFO);
            ROUTE_IMPORT(AAS_EntityInfo, BOTLIB_AAS_ENTITY_INFO);
            ROUTE_IMPORT(AAS_Initialized, BOTLIB_AAS_INITIALIZED);
            ROUTE_IMPORT(AAS_PresenceTypeBoundingBox, BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX);
            ROUTE_IMPORT(AAS_Time, BOTLIB_AAS_TIME);
            ROUTE_IMPORT(AAS_PointAreaNum, BOTLIB_AAS_POINT_AREA_NUM);
            ROUTE_IMPORT(AAS_TraceAreas, BOTLIB_AAS_TRACE_AREAS);
            ROUTE_IMPORT(AAS_PointContents, BOTLIB_AAS_POINT_CONTENTS);
            ROUTE_IMPORT(AAS_NextBSPEntity, BOTLIB_AAS_NEXT_BSP_ENTITY);
            ROUTE_IMPORT(AAS_ValueForBSPEpairKey, BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY);
            ROUTE_IMPORT(AAS_VectorForBSPEpairKey, BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY);
            ROUTE_IMPORT(AAS_FloatForBSPEpairKey, BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY);
            ROUTE_IMPORT(AAS_IntForBSPEpairKey, BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY);
            ROUTE_IMPORT(AAS_AreaReachability, BOTLIB_AAS_AREA_REACHABILITY);
            ROUTE_IMPORT(AAS_AreaTravelTimeToGoalArea, BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA);
            ROUTE_IMPORT(AAS_Swimming, BOTLIB_AAS_SWIMMING);
            ROUTE_IMPORT(AAS_PredictClientMovement, BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT);
            ROUTE_IMPORT(AAS_AlternativeRouteGoals, BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL);
            ROUTE_IMPORT(AAS_PredictRoute, BOTLIB_AAS_PREDICT_ROUTE);
            ROUTE_IMPORT(AAS_PointReachabilityAreaIndex, BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX);
            ROUTE_IMPORT(EA_Say, BOTLIB_EA_SAY);
            ROUTE_IMPORT(EA_SayTeam, BOTLIB_EA_SAY_TEAM);
            ROUTE_IMPORT(EA_Command, BOTLIB_EA_COMMAND);
            ROUTE_IMPORT(EA_Action, BOTLIB_EA_ACTION);
            ROUTE_IMPORT(EA_Gesture, BOTLIB_EA_GESTURE);
            ROUTE_IMPORT(EA_Talk, BOTLIB_EA_TALK);
            ROUTE_IMPORT(EA_Attack, BOTLIB_EA_ATTACK);
            ROUTE_IMPORT(EA_Alt_Attack, BOTLIB_EA_ALT_ATTACK);
            ROUTE_IMPORT(EA_ForcePower, BOTLIB_EA_FORCEPOWER);
            ROUTE_IMPORT(EA_Use, BOTLIB_EA_USE);
            ROUTE_IMPORT(EA_Respawn, BOTLIB_EA_RESPAWN);
            ROUTE_IMPORT(EA_Crouch, BOTLIB_EA_CROUCH);
            ROUTE_IMPORT(EA_MoveUp, BOTLIB_EA_MOVE_UP);
            ROUTE_IMPORT(EA_MoveDown, BOTLIB_EA_MOVE_DOWN);
            ROUTE_IMPORT(EA_MoveForward, BOTLIB_EA_MOVE_FORWARD);
            ROUTE_IMPORT(EA_MoveBack, BOTLIB_EA_MOVE_BACK);
            ROUTE_IMPORT(EA_MoveLeft, BOTLIB_EA_MOVE_LEFT);
            ROUTE_IMPORT(EA_MoveRight, BOTLIB_EA_MOVE_RIGHT);
            ROUTE_IMPORT(EA_SelectWeapon, BOTLIB_EA_SELECT_WEAPON);
            ROUTE_IMPORT(EA_Jump, BOTLIB_EA_JUMP);
            ROUTE_IMPORT(EA_DelayedJump, BOTLIB_EA_DELAYED_JUMP);
            ROUTE_IMPORT(EA_Move, BOTLIB_EA_MOVE);
            ROUTE_IMPORT(EA_View, BOTLIB_EA_VIEW);
            ROUTE_IMPORT(EA_EndRegular, BOTLIB_EA_END_REGULAR);
            ROUTE_IMPORT(EA_GetInput, BOTLIB_EA_GET_INPUT);
            ROUTE_IMPORT(EA_ResetInput, BOTLIB_EA_RESET_INPUT);
            ROUTE_IMPORT(PC_LoadSource, BOTLIB_PC_LOAD_SOURCE);
            ROUTE_IMPORT(PC_FreeSource, BOTLIB_PC_FREE_SOURCE);
            ROUTE_IMPORT(PC_ReadToken, BOTLIB_PC_READ_TOKEN);
            ROUTE_IMPORT(PC_SourceFileAndLine, BOTLIB_PC_SOURCE_FILE_AND_LINE);
            ROUTE_IMPORT(R_RegisterSkin, G_R_REGISTERSKIN);
            ROUTE_IMPORT(SetActiveSubBSP, G_SET_ACTIVE_SUBBSP);
            ROUTE_IMPORT(CM_RegisterTerrain, G_CM_REGISTER_TERRAIN);
            ROUTE_IMPORT(RMG_Init, G_RMG_INIT);
            ROUTE_IMPORT(G2API_ListModelBones, G_G2_LISTBONES);
            ROUTE_IMPORT(G2API_ListModelSurfaces, G_G2_LISTSURFACES);
            ROUTE_IMPORT(G2API_HaveWeGhoul2Models, G_G2_HAVEWEGHOULMODELS);
            ROUTE_IMPORT(G2API_SetGhoul2ModelIndexes, G_G2_SETMODELS);
            ROUTE_IMPORT(G2API_GetBoltMatrix, G_G2_GETBOLT);
            ROUTE_IMPORT(G2API_GetBoltMatrix_NoReconstruct, G_G2_GETBOLT_NOREC);
            ROUTE_IMPORT(G2API_GetBoltMatrix_NoRecNoRot, G_G2_GETBOLT_NOREC_NOROT);
            ROUTE_IMPORT(G2API_InitGhoul2Model, G_G2_INITGHOUL2MODEL);
            ROUTE_IMPORT(G2API_SetSkin, G_G2_SETSKIN);
            ROUTE_IMPORT(G2API_Ghoul2Size, G_G2_SIZE);
            ROUTE_IMPORT(G2API_AddBolt, G_G2_ADDBOLT);
            ROUTE_IMPORT(G2API_SetBoltInfo, G_G2_SETBOLTINFO);
            ROUTE_IMPORT(G2API_SetBoneAngles, G_G2_ANGLEOVERRIDE);
            ROUTE_IMPORT(G2API_SetBoneAnim, G_G2_PLAYANIM);
            ROUTE_IMPORT(G2API_GetBoneAnim, G_G2_GETBONEANIM);
            ROUTE_IMPORT(G2API_GetGLAName, G_G2_GETGLANAME);
            ROUTE_IMPORT(G2API_CopyGhoul2Instance, G_G2_COPYGHOUL2INSTANCE);
            ROUTE_IMPORT(G2API_CopySpecificGhoul2Model, G_G2_COPYSPECIFICGHOUL2MODEL);
            ROUTE_IMPORT(G2API_DuplicateGhoul2Instance, G_G2_DUPLICATEGHOUL2INSTANCE);
            ROUTE_IMPORT(G2API_HasGhoul2ModelOnIndex, G_G2_HASGHOUL2MODELONINDEX);
            ROUTE_IMPORT(G2API_RemoveGhoul2Model, G_G2_REMOVEGHOUL2MODEL);
            ROUTE_IMPORT(G2API_RemoveGhoul2Models, G_G2_REMOVEGHOUL2MODELS);
            ROUTE_IMPORT(G2API_CleanGhoul2Models, G_G2_CLEANMODELS);
            ROUTE_IMPORT(G2API_CollisionDetect, G_G2_COLLISIONDETECT);
            ROUTE_IMPORT(G2API_CollisionDetectCache, G_G2_COLLISIONDETECTCACHE);
            ROUTE_IMPORT(G2API_SetRootSurface, G_G2_SETROOTSURFACE);
            ROUTE_IMPORT(G2API_SetSurfaceOnOff, G_G2_SETSURFACEONOFF);
            ROUTE_IMPORT(G2API_SetNewOrigin, G_G2_SETNEWORIGIN);
            ROUTE_IMPORT(G2API_DoesBoneExist, G_G2_DOESBONEEXIST);
            ROUTE_IMPORT(G2API_GetSurfaceRenderStatus, G_G2_GETSURFACERENDERSTATUS);
            ROUTE_IMPORT(G2API_AbsurdSmoothing, G_G2_ABSURDSMOOTHING);
            ROUTE_IMPORT(G2API_SetRagDoll, G_G2_SETRAGDOLL);
            ROUTE_IMPORT(G2API_AnimateG2Models, G_G2_ANIMATEG2MODELS);
            ROUTE_IMPORT(G2API_RagPCJConstraint, G_G2_RAGPCJCONSTRAINT);
            ROUTE_IMPORT(G2API_RagPCJGradientSpeed, G_G2_RAGPCJGRADIENTSPEED);
            ROUTE_IMPORT(G2API_RagEffectorGoal, G_G2_RAGEFFECTORGOAL);
            ROUTE_IMPORT(G2API_GetRagBonePos, G_G2_GETRAGBONEPOS);
            ROUTE_IMPORT(G2API_RagEffectorKick, G_G2_RAGEFFECTORKICK);
            ROUTE_IMPORT(G2API_RagForceSolve, G_G2_RAGFORCESOLVE);
            ROUTE_IMPORT(G2API_SetBoneIKState, G_G2_SETBONEIKSTATE);
            ROUTE_IMPORT(G2API_IKMove, G_G2_IKMOVE);
            ROUTE_IMPORT(G2API_RemoveBone, G_G2_REMOVEBONE);
            ROUTE_IMPORT(G2API_AttachInstanceToEntNum, G_G2_ATTACHINSTANCETOENTNUM);
            ROUTE_IMPORT(G2API_ClearAttachedInstance, G_G2_CLEARATTACHEDINSTANCE);
            ROUTE_IMPORT(G2API_CleanEntAttachments, G_G2_CLEANENTATTACHMENTS);
            ROUTE_IMPORT(G2API_OverrideServer, G_G2_OVERRIDESERVER);
            ROUTE_IMPORT(G2API_GetSurfaceName, G_G2_GETSURFACENAME);

        // handle special cmds which QMM uses but JAMP doesn't have an analogue for
        case G_ARGS: {
            // quake2: char* (*args)(void);
            static std::string s;
            static char buf[MAX_STRING_CHARS];
            s = "";
            int i = 1;
            while (i < orig_import.Argc()) {
                orig_import.Argv(i, buf, sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
                if (i != 1)
                    s += " ";
                s += buf;
            }
            ret = (intptr_t)s.c_str();
            break;
        }

        default:
            break;
        };
    }

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_syscall({} {}) returning {}\n", JAMP_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
static intptr_t JAMP_vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_vmMain({} {}) called\n", JAMP_mod_msg_names(cmd), cmd);
#endif

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    // if the loaded JAMP mod uses the official JAMP or OpenJK "legacy" API
    if (orig_vmMain) {
        // all normal mod functions go to mod
        ret = orig_vmMain(cmd, QMM_PUT_VMMAIN_ARGS());
    }
    // if the loaded JAMP mod uses the OpenJK "new" API
    else if (orig_export) {
        switch (cmd) {
            ROUTE_EXPORT(InitGame, GAME_INIT);
            ROUTE_EXPORT(ShutdownGame, GAME_SHUTDOWN);
            ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
            ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
            ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
            ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
            ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
            ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
            ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
            ROUTE_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND);
            ROUTE_EXPORT(BotAIStartFrame, BOTAI_START_FRAME);
            ROUTE_EXPORT(ROFF_NotetrackCallback, GAME_ROFF_NOTETRACK_CALLBACK);
            ROUTE_EXPORT(SpawnRMGEntity, GAME_SPAWN_RMG_ENTITY);
            ROUTE_EXPORT(ICARUS_PlaySound, GAME_ICARUS_PLAYSOUND);
            ROUTE_EXPORT(ICARUS_Set, GAME_ICARUS_SET);
            ROUTE_EXPORT(ICARUS_Lerp2Pos, GAME_ICARUS_LERP2POS);
            ROUTE_EXPORT(ICARUS_Lerp2Origin, GAME_ICARUS_LERP2ORIGIN);
            ROUTE_EXPORT(ICARUS_Lerp2Angles, GAME_ICARUS_LERP2ANGLES);
            ROUTE_EXPORT(ICARUS_GetTag, GAME_ICARUS_GETTAG);
            ROUTE_EXPORT(ICARUS_Lerp2Start, GAME_ICARUS_LERP2START);
            ROUTE_EXPORT(ICARUS_Lerp2End, GAME_ICARUS_LERP2END);
            ROUTE_EXPORT(ICARUS_Use, GAME_ICARUS_USE);
            ROUTE_EXPORT(ICARUS_Kill, GAME_ICARUS_KILL);
            ROUTE_EXPORT(ICARUS_Remove, GAME_ICARUS_REMOVE);
            ROUTE_EXPORT(ICARUS_Play, GAME_ICARUS_PLAY);
            ROUTE_EXPORT(ICARUS_GetFloat, GAME_ICARUS_GETFLOAT);
            ROUTE_EXPORT(ICARUS_GetVector, GAME_ICARUS_GETVECTOR);
            ROUTE_EXPORT(ICARUS_GetString, GAME_ICARUS_GETSTRING);
            ROUTE_EXPORT(ICARUS_SoundIndex, GAME_ICARUS_SOUNDINDEX);
            ROUTE_EXPORT(ICARUS_GetSetIDForString, GAME_ICARUS_GETSETIDFORSTRING);
            ROUTE_EXPORT(NAV_ClearPathToPoint, GAME_NAV_CLEARPATHTOPOINT);
            ROUTE_EXPORT(NPC_ClearLOS2, GAME_NAV_CLEARLOS);
            ROUTE_EXPORT(NAVNEW_ClearPathBetweenPoints, GAME_NAV_CLEARPATHBETWEENPOINTS);
            ROUTE_EXPORT(NAV_CheckNodeFailedForEnt, GAME_NAV_CHECKNODEFAILEDFORENT);
            ROUTE_EXPORT(NAV_EntIsUnlockedDoor, GAME_NAV_ENTISUNLOCKEDDOOR);
            ROUTE_EXPORT(NAV_EntIsDoor, GAME_NAV_ENTISDOOR);
            ROUTE_EXPORT(NAV_EntIsBreakable, GAME_NAV_ENTISBREAKABLE);
            ROUTE_EXPORT(NAV_EntIsRemovableUsable, GAME_NAV_ENTISREMOVABLEUSABLE);
            ROUTE_EXPORT(NAV_FindCombatPointWaypoints, GAME_NAV_FINDCOMBATPOINTWAYPOINTS);
            ROUTE_EXPORT(BG_GetItemIndexByTag, GAME_GETITEMINDEXBYTAG);

        default:
            break;
        };
    }

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_vmMain({} {}) returning {}\n", JAMP_mod_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


static void* JAMP_entry(void* arg0, void* arg1, bool is_GetGameAPI) {
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_entry({}, {}, {}) called\n", arg0, arg1, is_GetGameAPI);

    if (is_GetGameAPI) {
        orig_apiversion = (intptr_t)arg0;

        // original import struct from engine
        // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
        game_import_t* gi = (game_import_t*)arg1;
        orig_import = *gi;

        // fill in variables of our hooked import struct to pass to the mod

        // pointer to wrapper vmMain function that calls either orig_vmMain or func in orig_export
        g_gameinfo.pfnvmMain = JAMP_vmMain;

        // pointer to wrapper syscall function that calls either orig_syscall or func in orig_import
        g_gameinfo.pfnsyscall = JAMP_syscall;

        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_entry({}, {}, {}) returning {}\n", arg0, arg1, is_GetGameAPI, (void*)&qmm_export);

        // struct full of export lambdas to QMM's vmMain
        // this gets returned to the game engine, but we haven't loaded the mod yet.
        return &qmm_export;
    }
    else {
        // store original syscall from engine
        orig_syscall = (eng_syscall)arg0;

        // pointer to wrapper vmMain function that calls either orig_vmMain or func in orig_export
        g_gameinfo.pfnvmMain = JAMP_vmMain;

        // pointer to wrapper syscall function that calls either orig_syscall or func in orig_import
        g_gameinfo.pfnsyscall = JAMP_syscall;

        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_entry({}, {}, {}) returning\n", arg0, arg1, is_GetGameAPI);

        return nullptr;
    }
}


static bool JAMP_mod_load(void* entry, bool is_GetGameAPI) {
    if (is_GetGameAPI) {
        mod_GetGameAPI pfnGGA = (mod_GetGameAPI)entry;
        // api version gets passed before import pointer
        orig_export = (game_export_t*)pfnGGA((void*)orig_apiversion, &qmm_import);

        return !!orig_export;
    }
    else {
        orig_vmMain = (mod_vmMain)entry;
        return !!orig_vmMain;
    }
}


static void JAMP_mod_unload() {
    orig_export = nullptr;
    orig_vmMain = nullptr;
}


static const char* JAMP_eng_msg_names(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_PRINT);
        GEN_CASE(G_ERROR);
        GEN_CASE(G_MILLISECONDS);
        GEN_CASE(G_PRECISIONTIMER_START);
        GEN_CASE(G_PRECISIONTIMER_END);
        GEN_CASE(G_CVAR_REGISTER);
        GEN_CASE(G_CVAR_UPDATE);
        GEN_CASE(G_CVAR_SET);
        GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
        GEN_CASE(G_ARGC);
        GEN_CASE(G_ARGV);
        GEN_CASE(G_FS_FOPEN_FILE);
        GEN_CASE(G_FS_READ);
        GEN_CASE(G_FS_WRITE);
        GEN_CASE(G_FS_FCLOSE_FILE);
        GEN_CASE(G_SEND_CONSOLE_COMMAND);
        GEN_CASE(G_LOCATE_GAME_DATA);
        GEN_CASE(G_DROP_CLIENT);
        GEN_CASE(G_SEND_SERVER_COMMAND);
        GEN_CASE(G_SET_CONFIGSTRING);
        GEN_CASE(G_GET_CONFIGSTRING);
        GEN_CASE(G_GET_USERINFO);
        GEN_CASE(G_SET_USERINFO);
        GEN_CASE(G_GET_SERVERINFO);
        GEN_CASE(G_SET_SERVER_CULL);
        GEN_CASE(G_SET_BRUSH_MODEL);
        GEN_CASE(G_TRACE);
        GEN_CASE(G_G2TRACE);
        GEN_CASE(G_POINT_CONTENTS);
        GEN_CASE(G_IN_PVS);
        GEN_CASE(G_IN_PVS_IGNORE_PORTALS);
        GEN_CASE(G_ADJUST_AREA_PORTAL_STATE);
        GEN_CASE(G_AREAS_CONNECTED);
        GEN_CASE(G_LINKENTITY);
        GEN_CASE(G_UNLINKENTITY);
        GEN_CASE(G_ENTITIES_IN_BOX);
        GEN_CASE(G_ENTITY_CONTACT);
        GEN_CASE(G_BOT_ALLOCATE_CLIENT);
        GEN_CASE(G_BOT_FREE_CLIENT);
        GEN_CASE(G_GET_USERCMD);
        GEN_CASE(G_GET_ENTITY_TOKEN);
        GEN_CASE(G_SIEGEPERSSET);
        GEN_CASE(G_SIEGEPERSGET);
        GEN_CASE(G_FS_GETFILELIST);
        GEN_CASE(G_DEBUG_POLYGON_CREATE);
        GEN_CASE(G_DEBUG_POLYGON_DELETE);
        GEN_CASE(G_REAL_TIME);
        GEN_CASE(G_SNAPVECTOR);
        GEN_CASE(G_TRACECAPSULE);
        GEN_CASE(G_ENTITY_CONTACTCAPSULE);
        // GEN_CASE(SP_REGISTER_SERVER_CMD); // commented out in SDK
        GEN_CASE(SP_GETSTRINGTEXTSTRING);
        GEN_CASE(G_ROFF_CLEAN);
        GEN_CASE(G_ROFF_UPDATE_ENTITIES);
        GEN_CASE(G_ROFF_CACHE);
        GEN_CASE(G_ROFF_PLAY);
        GEN_CASE(G_ROFF_PURGE_ENT);
        GEN_CASE(G_TRUEMALLOC);
        GEN_CASE(G_TRUEFREE);
        GEN_CASE(G_ICARUS_RUNSCRIPT);
        GEN_CASE(G_ICARUS_REGISTERSCRIPT);
        GEN_CASE(G_ICARUS_INIT);
        GEN_CASE(G_ICARUS_VALIDENT);
        GEN_CASE(G_ICARUS_ISINITIALIZED);
        GEN_CASE(G_ICARUS_MAINTAINTASKMANAGER);
        GEN_CASE(G_ICARUS_ISRUNNING);
        GEN_CASE(G_ICARUS_TASKIDPENDING);
        GEN_CASE(G_ICARUS_INITENT);
        GEN_CASE(G_ICARUS_FREEENT);
        GEN_CASE(G_ICARUS_ASSOCIATEENT);
        GEN_CASE(G_ICARUS_SHUTDOWN);
        GEN_CASE(G_ICARUS_TASKIDSET);
        GEN_CASE(G_ICARUS_TASKIDCOMPLETE);
        GEN_CASE(G_ICARUS_SETVAR);
        GEN_CASE(G_ICARUS_VARIABLEDECLARED);
        GEN_CASE(G_ICARUS_GETFLOATVARIABLE);
        GEN_CASE(G_ICARUS_GETSTRINGVARIABLE);
        GEN_CASE(G_ICARUS_GETVECTORVARIABLE);
        GEN_CASE(G_SET_SHARED_BUFFER);
        GEN_CASE(G_MEMSET);
        GEN_CASE(G_MEMCPY);
        GEN_CASE(G_STRNCPY);
        GEN_CASE(G_SIN);
        GEN_CASE(G_COS);
        GEN_CASE(G_ATAN2);
        GEN_CASE(G_SQRT);
        GEN_CASE(G_MATRIXMULTIPLY);
        GEN_CASE(G_ANGLEVECTORS);
        GEN_CASE(G_PERPENDICULARVECTOR);
        GEN_CASE(G_FLOOR);
        GEN_CASE(G_CEIL);
        GEN_CASE(G_TESTPRINTINT);
        GEN_CASE(G_TESTPRINTFLOAT);
        GEN_CASE(G_ACOS);
        GEN_CASE(G_ASIN);
        GEN_CASE(G_NAV_INIT);
        GEN_CASE(G_NAV_FREE);
        GEN_CASE(G_NAV_LOAD);
        GEN_CASE(G_NAV_SAVE);
        GEN_CASE(G_NAV_ADDRAWPOINT);
        GEN_CASE(G_NAV_CALCULATEPATHS);
        GEN_CASE(G_NAV_HARDCONNECT);
        GEN_CASE(G_NAV_SHOWNODES);
        GEN_CASE(G_NAV_SHOWEDGES);
        GEN_CASE(G_NAV_SHOWPATH);
        GEN_CASE(G_NAV_GETNEARESTNODE);
        GEN_CASE(G_NAV_GETBESTNODE);
        GEN_CASE(G_NAV_GETNODEPOSITION);
        GEN_CASE(G_NAV_GETNODENUMEDGES);
        GEN_CASE(G_NAV_GETNODEEDGE);
        GEN_CASE(G_NAV_GETNUMNODES);
        GEN_CASE(G_NAV_CONNECTED);
        GEN_CASE(G_NAV_GETPATHCOST);
        GEN_CASE(G_NAV_GETEDGECOST);
        GEN_CASE(G_NAV_GETPROJECTEDNODE);
        GEN_CASE(G_NAV_CHECKFAILEDNODES);
        GEN_CASE(G_NAV_ADDFAILEDNODE);
        GEN_CASE(G_NAV_NODEFAILED);
        GEN_CASE(G_NAV_NODESARENEIGHBORS);
        GEN_CASE(G_NAV_CLEARFAILEDEDGE);
        GEN_CASE(G_NAV_CLEARALLFAILEDEDGES);
        GEN_CASE(G_NAV_EDGEFAILED);
        GEN_CASE(G_NAV_ADDFAILEDEDGE);
        GEN_CASE(G_NAV_CHECKFAILEDEDGE);
        GEN_CASE(G_NAV_CHECKALLFAILEDEDGES);
        GEN_CASE(G_NAV_ROUTEBLOCKED);
        GEN_CASE(G_NAV_GETBESTNODEALTROUTE);
        GEN_CASE(G_NAV_GETBESTNODEALT2);
        GEN_CASE(G_NAV_GETBESTPATHBETWEENENTS);
        GEN_CASE(G_NAV_GETNODERADIUS);
        GEN_CASE(G_NAV_CHECKBLOCKEDEDGES);
        GEN_CASE(G_NAV_CLEARCHECKEDNODES);
        GEN_CASE(G_NAV_CHECKEDNODE);
        GEN_CASE(G_NAV_SETCHECKEDNODE);
        GEN_CASE(G_NAV_FLAGALLNODES);
        GEN_CASE(G_NAV_GETPATHSCALCULATED);
        GEN_CASE(G_NAV_SETPATHSCALCULATED);
        GEN_CASE(BOTLIB_SETUP);
        GEN_CASE(BOTLIB_SHUTDOWN);
        GEN_CASE(BOTLIB_LIBVAR_SET);
        GEN_CASE(BOTLIB_LIBVAR_GET);
        GEN_CASE(BOTLIB_PC_ADD_GLOBAL_DEFINE);
        GEN_CASE(BOTLIB_START_FRAME);
        GEN_CASE(BOTLIB_LOAD_MAP);
        GEN_CASE(BOTLIB_UPDATENTITY);
        GEN_CASE(BOTLIB_TEST);
        GEN_CASE(BOTLIB_GET_SNAPSHOT_ENTITY);
        GEN_CASE(BOTLIB_GET_CONSOLE_MESSAGE);
        GEN_CASE(BOTLIB_USER_COMMAND);
        GEN_CASE(BOTLIB_AAS_ENABLE_ROUTING_AREA);
        GEN_CASE(BOTLIB_AAS_BBOX_AREAS);
        GEN_CASE(BOTLIB_AAS_AREA_INFO);
        GEN_CASE(BOTLIB_AAS_ENTITY_INFO);
        GEN_CASE(BOTLIB_AAS_INITIALIZED);
        GEN_CASE(BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX);
        GEN_CASE(BOTLIB_AAS_TIME);
        GEN_CASE(BOTLIB_AAS_POINT_AREA_NUM);
        GEN_CASE(BOTLIB_AAS_TRACE_AREAS);
        GEN_CASE(BOTLIB_AAS_POINT_CONTENTS);
        GEN_CASE(BOTLIB_AAS_NEXT_BSP_ENTITY);
        GEN_CASE(BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_AREA_REACHABILITY);
        GEN_CASE(BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA);
        GEN_CASE(BOTLIB_AAS_SWIMMING);
        GEN_CASE(BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT);
        GEN_CASE(BOTLIB_EA_SAY);
        GEN_CASE(BOTLIB_EA_SAY_TEAM);
        GEN_CASE(BOTLIB_EA_COMMAND);
        GEN_CASE(BOTLIB_EA_ACTION);
        GEN_CASE(BOTLIB_EA_GESTURE);
        GEN_CASE(BOTLIB_EA_TALK);
        GEN_CASE(BOTLIB_EA_ATTACK);
        GEN_CASE(BOTLIB_EA_ALT_ATTACK);
        GEN_CASE(BOTLIB_EA_FORCEPOWER);
        GEN_CASE(BOTLIB_EA_USE);
        GEN_CASE(BOTLIB_EA_RESPAWN);
        GEN_CASE(BOTLIB_EA_CROUCH);
        GEN_CASE(BOTLIB_EA_MOVE_UP);
        GEN_CASE(BOTLIB_EA_MOVE_DOWN);
        GEN_CASE(BOTLIB_EA_MOVE_FORWARD);
        GEN_CASE(BOTLIB_EA_MOVE_BACK);
        GEN_CASE(BOTLIB_EA_MOVE_LEFT);
        GEN_CASE(BOTLIB_EA_MOVE_RIGHT);
        GEN_CASE(BOTLIB_EA_SELECT_WEAPON);
        GEN_CASE(BOTLIB_EA_JUMP);
        GEN_CASE(BOTLIB_EA_DELAYED_JUMP);
        GEN_CASE(BOTLIB_EA_MOVE);
        GEN_CASE(BOTLIB_EA_VIEW);
        GEN_CASE(BOTLIB_EA_END_REGULAR);
        GEN_CASE(BOTLIB_EA_GET_INPUT);
        GEN_CASE(BOTLIB_EA_RESET_INPUT);
        GEN_CASE(BOTLIB_AI_LOAD_CHARACTER);
        GEN_CASE(BOTLIB_AI_FREE_CHARACTER);
        GEN_CASE(BOTLIB_AI_CHARACTERISTIC_FLOAT);
        GEN_CASE(BOTLIB_AI_CHARACTERISTIC_BFLOAT);
        GEN_CASE(BOTLIB_AI_CHARACTERISTIC_INTEGER);
        GEN_CASE(BOTLIB_AI_CHARACTERISTIC_BINTEGER);
        GEN_CASE(BOTLIB_AI_CHARACTERISTIC_STRING);
        GEN_CASE(BOTLIB_AI_ALLOC_CHAT_STATE);
        GEN_CASE(BOTLIB_AI_FREE_CHAT_STATE);
        GEN_CASE(BOTLIB_AI_QUEUE_CONSOLE_MESSAGE);
        GEN_CASE(BOTLIB_AI_REMOVE_CONSOLE_MESSAGE);
        GEN_CASE(BOTLIB_AI_NEXT_CONSOLE_MESSAGE);
        GEN_CASE(BOTLIB_AI_NUM_CONSOLE_MESSAGE);
        GEN_CASE(BOTLIB_AI_INITIAL_CHAT);
        GEN_CASE(BOTLIB_AI_REPLY_CHAT);
        GEN_CASE(BOTLIB_AI_CHAT_LENGTH);
        GEN_CASE(BOTLIB_AI_ENTER_CHAT);
        GEN_CASE(BOTLIB_AI_STRING_CONTAINS);
        GEN_CASE(BOTLIB_AI_FIND_MATCH);
        GEN_CASE(BOTLIB_AI_MATCH_VARIABLE);
        GEN_CASE(BOTLIB_AI_UNIFY_WHITE_SPACES);
        GEN_CASE(BOTLIB_AI_REPLACE_SYNONYMS);
        GEN_CASE(BOTLIB_AI_LOAD_CHAT_FILE);
        GEN_CASE(BOTLIB_AI_SET_CHAT_GENDER);
        GEN_CASE(BOTLIB_AI_SET_CHAT_NAME);
        GEN_CASE(BOTLIB_AI_RESET_GOAL_STATE);
        GEN_CASE(BOTLIB_AI_RESET_AVOID_GOALS);
        GEN_CASE(BOTLIB_AI_PUSH_GOAL);
        GEN_CASE(BOTLIB_AI_POP_GOAL);
        GEN_CASE(BOTLIB_AI_EMPTY_GOAL_STACK);
        GEN_CASE(BOTLIB_AI_DUMP_AVOID_GOALS);
        GEN_CASE(BOTLIB_AI_DUMP_GOAL_STACK);
        GEN_CASE(BOTLIB_AI_GOAL_NAME);
        GEN_CASE(BOTLIB_AI_GET_TOP_GOAL);
        GEN_CASE(BOTLIB_AI_GET_SECOND_GOAL);
        GEN_CASE(BOTLIB_AI_CHOOSE_LTG_ITEM);
        GEN_CASE(BOTLIB_AI_CHOOSE_NBG_ITEM);
        GEN_CASE(BOTLIB_AI_TOUCHING_GOAL);
        GEN_CASE(BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE);
        GEN_CASE(BOTLIB_AI_GET_LEVEL_ITEM_GOAL);
        GEN_CASE(BOTLIB_AI_AVOID_GOAL_TIME);
        GEN_CASE(BOTLIB_AI_INIT_LEVEL_ITEMS);
        GEN_CASE(BOTLIB_AI_UPDATE_ENTITY_ITEMS);
        GEN_CASE(BOTLIB_AI_LOAD_ITEM_WEIGHTS);
        GEN_CASE(BOTLIB_AI_FREE_ITEM_WEIGHTS);
        GEN_CASE(BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC);
        GEN_CASE(BOTLIB_AI_ALLOC_GOAL_STATE);
        GEN_CASE(BOTLIB_AI_FREE_GOAL_STATE);
        GEN_CASE(BOTLIB_AI_RESET_MOVE_STATE);
        GEN_CASE(BOTLIB_AI_MOVE_TO_GOAL);
        GEN_CASE(BOTLIB_AI_MOVE_IN_DIRECTION);
        GEN_CASE(BOTLIB_AI_RESET_AVOID_REACH);
        GEN_CASE(BOTLIB_AI_RESET_LAST_AVOID_REACH);
        GEN_CASE(BOTLIB_AI_REACHABILITY_AREA);
        GEN_CASE(BOTLIB_AI_MOVEMENT_VIEW_TARGET);
        GEN_CASE(BOTLIB_AI_ALLOC_MOVE_STATE);
        GEN_CASE(BOTLIB_AI_FREE_MOVE_STATE);
        GEN_CASE(BOTLIB_AI_INIT_MOVE_STATE);
        GEN_CASE(BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON);
        GEN_CASE(BOTLIB_AI_GET_WEAPON_INFO);
        GEN_CASE(BOTLIB_AI_LOAD_WEAPON_WEIGHTS);
        GEN_CASE(BOTLIB_AI_ALLOC_WEAPON_STATE);
        GEN_CASE(BOTLIB_AI_FREE_WEAPON_STATE);
        GEN_CASE(BOTLIB_AI_RESET_WEAPON_STATE);
        GEN_CASE(BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION);
        GEN_CASE(BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC);
        GEN_CASE(BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC);
        GEN_CASE(BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL);
        GEN_CASE(BOTLIB_AI_GET_MAP_LOCATION_GOAL);
        GEN_CASE(BOTLIB_AI_NUM_INITIAL_CHATS);
        GEN_CASE(BOTLIB_AI_GET_CHAT_MESSAGE);
        GEN_CASE(BOTLIB_AI_REMOVE_FROM_AVOID_GOALS);
        GEN_CASE(BOTLIB_AI_PREDICT_VISIBLE_POSITION);
        GEN_CASE(BOTLIB_AI_SET_AVOID_GOAL_TIME);
        GEN_CASE(BOTLIB_AI_ADD_AVOID_SPOT);
        GEN_CASE(BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL);
        GEN_CASE(BOTLIB_AAS_PREDICT_ROUTE);
        GEN_CASE(BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX);
        GEN_CASE(BOTLIB_PC_LOAD_SOURCE);
        GEN_CASE(BOTLIB_PC_FREE_SOURCE);
        GEN_CASE(BOTLIB_PC_READ_TOKEN);
        GEN_CASE(BOTLIB_PC_SOURCE_FILE_AND_LINE);
        GEN_CASE(G_R_REGISTERSKIN);
        GEN_CASE(G_G2_LISTBONES);
        GEN_CASE(G_G2_LISTSURFACES);
        GEN_CASE(G_G2_HAVEWEGHOULMODELS);
        GEN_CASE(G_G2_SETMODELS);
        GEN_CASE(G_G2_GETBOLT);
        GEN_CASE(G_G2_GETBOLT_NOREC);
        GEN_CASE(G_G2_GETBOLT_NOREC_NOROT);
        GEN_CASE(G_G2_INITGHOUL2MODEL);
        GEN_CASE(G_G2_SETSKIN);
        GEN_CASE(G_G2_SIZE);
        GEN_CASE(G_G2_ADDBOLT);
        GEN_CASE(G_G2_SETBOLTINFO);
        GEN_CASE(G_G2_ANGLEOVERRIDE);
        GEN_CASE(G_G2_PLAYANIM);
        GEN_CASE(G_G2_GETBONEANIM);
        GEN_CASE(G_G2_GETGLANAME);
        GEN_CASE(G_G2_COPYGHOUL2INSTANCE);
        GEN_CASE(G_G2_COPYSPECIFICGHOUL2MODEL);
        GEN_CASE(G_G2_DUPLICATEGHOUL2INSTANCE);
        GEN_CASE(G_G2_HASGHOUL2MODELONINDEX);
        GEN_CASE(G_G2_REMOVEGHOUL2MODEL);
        GEN_CASE(G_G2_REMOVEGHOUL2MODELS);
        GEN_CASE(G_G2_CLEANMODELS);
        GEN_CASE(G_G2_COLLISIONDETECT);
        GEN_CASE(G_G2_COLLISIONDETECTCACHE);
        GEN_CASE(G_G2_SETROOTSURFACE);
        GEN_CASE(G_G2_SETSURFACEONOFF);
        GEN_CASE(G_G2_SETNEWORIGIN);
        GEN_CASE(G_G2_DOESBONEEXIST);
        GEN_CASE(G_G2_GETSURFACERENDERSTATUS);
        GEN_CASE(G_G2_ABSURDSMOOTHING);
        GEN_CASE(G_G2_SETRAGDOLL);
        GEN_CASE(G_G2_ANIMATEG2MODELS);
        GEN_CASE(G_G2_RAGPCJCONSTRAINT);
        GEN_CASE(G_G2_RAGPCJGRADIENTSPEED);
        GEN_CASE(G_G2_RAGEFFECTORGOAL);
        GEN_CASE(G_G2_GETRAGBONEPOS);
        GEN_CASE(G_G2_RAGEFFECTORKICK);
        GEN_CASE(G_G2_RAGFORCESOLVE);
        GEN_CASE(G_G2_SETBONEIKSTATE);
        GEN_CASE(G_G2_IKMOVE);
        GEN_CASE(G_G2_REMOVEBONE);
        GEN_CASE(G_G2_ATTACHINSTANCETOENTNUM);
        GEN_CASE(G_G2_CLEARATTACHEDINSTANCE);
        GEN_CASE(G_G2_CLEANENTATTACHMENTS);
        GEN_CASE(G_G2_OVERRIDESERVER);
        GEN_CASE(G_G2_GETSURFACENAME);
        GEN_CASE(G_SET_ACTIVE_SUBBSP);
        GEN_CASE(G_CM_REGISTER_TERRAIN);
        GEN_CASE(G_RMG_INIT);
        GEN_CASE(G_BOT_UPDATEWAYPOINTS);
        GEN_CASE(G_BOT_CALCULATEPATHS);

        // polyfills
        GEN_CASE(G_ARGS);
    default:
        return "unknown";
    }
}


static const char* JAMP_mod_msg_names(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(GAME_INIT);
        GEN_CASE(GAME_SHUTDOWN);
        GEN_CASE(GAME_CLIENT_CONNECT);
        GEN_CASE(GAME_CLIENT_BEGIN);
        GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
        GEN_CASE(GAME_CLIENT_DISCONNECT);
        GEN_CASE(GAME_CLIENT_COMMAND);
        GEN_CASE(GAME_CLIENT_THINK);
        GEN_CASE(GAME_RUN_FRAME);
        GEN_CASE(GAME_CONSOLE_COMMAND);
        GEN_CASE(BOTAI_START_FRAME);
        GEN_CASE(GAME_ROFF_NOTETRACK_CALLBACK);
        GEN_CASE(GAME_SPAWN_RMG_ENTITY);
        GEN_CASE(GAME_ICARUS_PLAYSOUND);
        GEN_CASE(GAME_ICARUS_SET);
        GEN_CASE(GAME_ICARUS_LERP2POS);
        GEN_CASE(GAME_ICARUS_LERP2ORIGIN);
        GEN_CASE(GAME_ICARUS_LERP2ANGLES);
        GEN_CASE(GAME_ICARUS_GETTAG);
        GEN_CASE(GAME_ICARUS_LERP2START);
        GEN_CASE(GAME_ICARUS_LERP2END);
        GEN_CASE(GAME_ICARUS_USE);
        GEN_CASE(GAME_ICARUS_KILL);
        GEN_CASE(GAME_ICARUS_REMOVE);
        GEN_CASE(GAME_ICARUS_PLAY);
        GEN_CASE(GAME_ICARUS_GETFLOAT);
        GEN_CASE(GAME_ICARUS_GETVECTOR);
        GEN_CASE(GAME_ICARUS_GETSTRING);
        GEN_CASE(GAME_ICARUS_SOUNDINDEX);
        GEN_CASE(GAME_ICARUS_GETSETIDFORSTRING);
        GEN_CASE(GAME_NAV_CLEARPATHTOPOINT);
        GEN_CASE(GAME_NAV_CLEARLOS);
        GEN_CASE(GAME_NAV_CLEARPATHBETWEENPOINTS);
        GEN_CASE(GAME_NAV_CHECKNODEFAILEDFORENT);
        GEN_CASE(GAME_NAV_ENTISUNLOCKEDDOOR);
        GEN_CASE(GAME_NAV_ENTISDOOR);
        GEN_CASE(GAME_NAV_ENTISBREAKABLE);
        GEN_CASE(GAME_NAV_ENTISREMOVABLEUSABLE);
        GEN_CASE(GAME_NAV_FINDCOMBATPOINTWAYPOINTS);
        GEN_CASE(GAME_GETITEMINDEXBYTAG);

    default:
        return "unknown";
    }
}
