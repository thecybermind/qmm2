/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <q3a/game/q_shared.h>
#include <q3a/game/g_public.h>

#include "game_api.h"
#include "log.h"
// QMM-specific Q3A header
#include "game_q3a.h"
#include "main.h"
#include "mod.h"

GEN_QMM_MSGS(Q3A);
GEN_EXTS(Q3A);

// a copy of the original syscall pointer that comes from the game engine
static eng_syscall_t orig_syscall = nullptr;

// mod vmMain is access via g_mod.pfnvmMain


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t Q3A_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q3A_syscall({} {}) called\n", Q3A_eng_msg_names(cmd), cmd);
#endif

    intptr_t ret = 0;

    switch (cmd) {
    // handle special cmds which QMM uses but Q3A doesn't have an analogue for
    case G_ARGS: {
        // quake2: char* (*args)(void);
        static std::string s;
        char buf[MAX_STRING_CHARS];
        s = "";
        int i = 1;
        while (i < orig_syscall(G_ARGC)) {
            orig_syscall(G_ARGV, buf, sizeof(buf));
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

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q3A_syscall({} {}) returning {}\n", Q3A_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t Q3A_vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q3A_vmMain({} {}) called\n", Q3A_mod_msg_names(cmd), cmd);

    if (!g_mod.pfnvmMain)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    // all normal mod functions go to mod
    ret = g_mod.pfnvmMain(cmd, QMM_PUT_VMMAIN_ARGS());

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q3A_vmMain({} {}) returning {}\n", Q3A_mod_msg_names(cmd), cmd, ret);

    return ret;
}


void Q3A_dllEntry(eng_syscall_t syscall) {
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q3A_dllEntry({}) called\n", (void*)syscall);

    // store original syscall from engine
    orig_syscall = syscall;

    // pointer to wrapper vmMain function that calls actual mod vmMain func g_mod.pfnvmMain
    g_gameinfo.pfnvmMain = Q3A_vmMain;

    // pointer to wrapper syscall function that calls actual engine syscall func
    g_gameinfo.pfnsyscall = Q3A_syscall;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q3A_dllEntry({}) returning\n", (void*)syscall);
}


const char* Q3A_eng_msg_names(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_PRINT);
        GEN_CASE(G_ERROR);
        GEN_CASE(G_MILLISECONDS);
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
        GEN_CASE(G_SET_BRUSH_MODEL);
        GEN_CASE(G_TRACE);
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
        GEN_CASE(G_FS_GETFILELIST);
        GEN_CASE(G_DEBUG_POLYGON_CREATE);
        GEN_CASE(G_DEBUG_POLYGON_DELETE);
        GEN_CASE(G_REAL_TIME);
        GEN_CASE(G_SNAPVECTOR);
        GEN_CASE(G_TRACECAPSULE);
        GEN_CASE(G_ENTITY_CONTACTCAPSULE);
        GEN_CASE(G_FS_SEEK);
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
        GEN_CASE(BOTLIB_AAS_ENTITY_INFO);
        GEN_CASE(BOTLIB_AAS_INITIALIZED);
        GEN_CASE(BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX);
        GEN_CASE(BOTLIB_AAS_TIME);
        GEN_CASE(BOTLIB_AAS_POINT_AREA_NUM);
        GEN_CASE(BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX);
        GEN_CASE(BOTLIB_AAS_TRACE_AREAS);
        GEN_CASE(BOTLIB_AAS_BBOX_AREAS);
        GEN_CASE(BOTLIB_AAS_AREA_INFO);
        GEN_CASE(BOTLIB_AAS_POINT_CONTENTS);
        GEN_CASE(BOTLIB_AAS_NEXT_BSP_ENTITY);
        GEN_CASE(BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_AREA_REACHABILITY);
        GEN_CASE(BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA);
        GEN_CASE(BOTLIB_AAS_ENABLE_ROUTING_AREA);
        GEN_CASE(BOTLIB_AAS_PREDICT_ROUTE);
        GEN_CASE(BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL);
        GEN_CASE(BOTLIB_AAS_SWIMMING);
        GEN_CASE(BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT);
        GEN_CASE(BOTLIB_EA_SAY);
        GEN_CASE(BOTLIB_EA_SAY_TEAM);
        GEN_CASE(BOTLIB_EA_COMMAND);
        GEN_CASE(BOTLIB_EA_ACTION);
        GEN_CASE(BOTLIB_EA_GESTURE);
        GEN_CASE(BOTLIB_EA_TALK);
        GEN_CASE(BOTLIB_EA_ATTACK);
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
        GEN_CASE(BOTLIB_AI_NUM_INITIAL_CHATS);
        GEN_CASE(BOTLIB_AI_REPLY_CHAT);
        GEN_CASE(BOTLIB_AI_CHAT_LENGTH);
        GEN_CASE(BOTLIB_AI_ENTER_CHAT);
        GEN_CASE(BOTLIB_AI_GET_CHAT_MESSAGE);
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
        GEN_CASE(BOTLIB_AI_REMOVE_FROM_AVOID_GOALS);
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
        GEN_CASE(BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL);
        GEN_CASE(BOTLIB_AI_GET_MAP_LOCATION_GOAL);
        GEN_CASE(BOTLIB_AI_AVOID_GOAL_TIME);
        GEN_CASE(BOTLIB_AI_SET_AVOID_GOAL_TIME);
        GEN_CASE(BOTLIB_AI_INIT_LEVEL_ITEMS);
        GEN_CASE(BOTLIB_AI_UPDATE_ENTITY_ITEMS);
        GEN_CASE(BOTLIB_AI_LOAD_ITEM_WEIGHTS);
        GEN_CASE(BOTLIB_AI_FREE_ITEM_WEIGHTS);
        GEN_CASE(BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC);
        GEN_CASE(BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC);
        GEN_CASE(BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC);
        GEN_CASE(BOTLIB_AI_ALLOC_GOAL_STATE);
        GEN_CASE(BOTLIB_AI_FREE_GOAL_STATE);
        GEN_CASE(BOTLIB_AI_RESET_MOVE_STATE);
        GEN_CASE(BOTLIB_AI_ADD_AVOID_SPOT);
        GEN_CASE(BOTLIB_AI_MOVE_TO_GOAL);
        GEN_CASE(BOTLIB_AI_MOVE_IN_DIRECTION);
        GEN_CASE(BOTLIB_AI_RESET_AVOID_REACH);
        GEN_CASE(BOTLIB_AI_RESET_LAST_AVOID_REACH);
        GEN_CASE(BOTLIB_AI_REACHABILITY_AREA);
        GEN_CASE(BOTLIB_AI_MOVEMENT_VIEW_TARGET);
        GEN_CASE(BOTLIB_AI_PREDICT_VISIBLE_POSITION);
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
        GEN_CASE(BOTLIB_PC_LOAD_SOURCE);
        GEN_CASE(BOTLIB_PC_FREE_SOURCE);
        GEN_CASE(BOTLIB_PC_READ_TOKEN);
        GEN_CASE(BOTLIB_PC_SOURCE_FILE_AND_LINE);
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

        // polyfills
        GEN_CASE(G_ARGS);

    default:
        return "unknown";
    }
}


const char* Q3A_mod_msg_names(intptr_t cmd) {
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
    default:
        return "unknown";
    }
}


/* Entry point: qvm mod->qmm
   This is the syscall function called by a QVM mod as a way to pass info to or get info from the engine.
   It modifies pointer arguments (if they are not NULL, the QVM data segment base address is added), and then
   the call is passed to the normal syscall function that DLL mods call.
*/
// vec3_t are arrays, so convert them as pointers
// for double pointers (gentity_t** and vec3_t*), convert them once with vmptr()
int Q3A_vmsyscall(uint8_t* membase, int cmd, int* args) {
#ifdef _DEBUG
    LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q3A_vmsyscall({} {}) called\n", Q3A_eng_msg_names(cmd), cmd);
#endif

    intptr_t ret = 0;

    switch (cmd) {
    case G_MILLISECONDS:			// (void)
    case G_ARGC:				// (void)
    case G_BOT_ALLOCATE_CLIENT:		// (void)
    case BOTLIB_SETUP:			// (void)
    case BOTLIB_SHUTDOWN:			// (void)
    case BOTLIB_AAS_INITIALIZED:		// (void)
    case BOTLIB_AAS_TIME:			// (void)
    case BOTLIB_AI_ALLOC_CHAT_STATE:	// (void)
    case BOTLIB_AI_INIT_LEVEL_ITEMS:	// (void)
    case BOTLIB_AI_UPDATE_ENTITY_ITEMS:	// (void)
    case BOTLIB_AI_ALLOC_MOVE_STATE:	// (void)
    case BOTLIB_AI_ALLOC_WEAPON_STATE:	// (void)
        ret = qmm_syscall(cmd);
        break;
    case G_PRINT:				// (const char* string);
    case G_ERROR:				// (const char* string);
    case G_CVAR_UPDATE:			// (vmCvar_t* vmCvar);
    case G_CVAR_VARIABLE_INTEGER_VALUE:	// (const char* var_name);
    case G_LINKENTITY:			// (gentity_t* ent);
    case G_UNLINKENTITY:			// (gentity_t* ent);
    case G_REAL_TIME:			// (qtime_t* qtime)
    case G_SNAPVECTOR:			// (float* v)
    case BOTLIB_PC_ADD_GLOBAL_DEFINE:	// (char* string)
    case BOTLIB_LOAD_MAP:			// (const char* mapname)
    case BOTLIB_AAS_POINT_AREA_NUM:		// (vec3_t point)
    case BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX:	// (vec3_t point)
    case BOTLIB_AAS_POINT_CONTENTS:		// (vec3_t point)
    case BOTLIB_AAS_SWIMMING:		// (vec3_t origin)
    case BOTLIB_AI_UNIFY_WHITE_SPACES:	// (char* string)
    case BOTLIB_PC_LOAD_SOURCE:		// (const char*)
        ret = qmm_syscall(cmd, vmptr(0));
        break;
    case G_FS_FCLOSE_FILE:			// (fileHandle_t f);
    case G_BOT_FREE_CLIENT:			// (int clientNum);
    case G_DEBUG_POLYGON_DELETE:		// (int id)
    case BOTLIB_START_FRAME:		// (float time)
    case BOTLIB_AAS_NEXT_BSP_ENTITY:	// (int ent)
    case BOTLIB_AAS_AREA_REACHABILITY:	// (int areanum)
    case BOTLIB_EA_GESTURE:			// (int client)
    case BOTLIB_EA_TALK:			// (int client)
    case BOTLIB_EA_ATTACK:			// (int client)
    case BOTLIB_EA_USE:			// (int client)
    case BOTLIB_EA_RESPAWN:			// (int client)
    case BOTLIB_EA_CROUCH:			// (int client)
    case BOTLIB_EA_MOVE_UP:			// (int client)
    case BOTLIB_EA_MOVE_DOWN:		// (int client)
    case BOTLIB_EA_MOVE_FORWARD:		// (int client)
    case BOTLIB_EA_MOVE_BACK:		// (int client)
    case BOTLIB_EA_MOVE_LEFT:		// (int client)
    case BOTLIB_EA_MOVE_RIGHT:		// (int client)
    case BOTLIB_EA_JUMP:			// (int client)
    case BOTLIB_EA_DELAYED_JUMP:		// (int client)
    case BOTLIB_EA_RESET_INPUT:		// (int client)
    case BOTLIB_AI_FREE_CHARACTER:		// (int character)
    case BOTLIB_AI_FREE_CHAT_STATE:		// (int handle)
    case BOTLIB_AI_CHAT_LENGTH:		// (int chatstate)
    case BOTLIB_AI_NUM_CONSOLE_MESSAGE:	// (int chatstate)
    case BOTLIB_AI_RESET_GOAL_STATE:	// (int goalstate)
    case BOTLIB_AI_RESET_AVOID_GOALS:	// (int goalstate)
    case BOTLIB_AI_POP_GOAL:		// (int goalstate)
    case BOTLIB_AI_EMPTY_GOAL_STACK:	// (int goalstate)
    case BOTLIB_AI_DUMP_AVOID_GOALS:	// (int goalstate)
    case BOTLIB_AI_DUMP_GOAL_STACK:		// (int goalstate)
    case BOTLIB_AI_FREE_ITEM_WEIGHTS:	// (int goalstate)
    case BOTLIB_AI_ALLOC_GOAL_STATE:	// (int state)
    case BOTLIB_AI_FREE_GOAL_STATE:		// (int handle)
    case BOTLIB_AI_RESET_MOVE_STATE:	// (int movestate)
    case BOTLIB_AI_RESET_AVOID_REACH:	// (int movestate)
    case BOTLIB_AI_RESET_LAST_AVOID_REACH:	// (int movestate)
    case BOTLIB_AI_FREE_MOVE_STATE:		// (int handle)
    case BOTLIB_AI_FREE_WEAPON_STATE:	// (int)
    case BOTLIB_AI_RESET_WEAPON_STATE:	// (int)
    case BOTLIB_PC_FREE_SOURCE:		// (int)
    case G_SIN:				// (double)
    case G_COS:				// (double)
    case G_SQRT:				// (double)
    case G_FLOOR:				// (double)
    case G_CEIL:				// (double)
        ret = qmm_syscall(cmd, vmarg(0));
        break;
    case G_CVAR_SET:			// (const char* var_name, const char* value);
    case G_SET_BRUSH_MODEL:			// (gentity_t* ent, const char* name);
    case G_IN_PVS:				// (const vec3_t p1, const vec3_t p2);
    case G_IN_PVS_IGNORE_PORTALS:		// (const vec3_t p1, const vec3_t p2);
    case BOTLIB_LIBVAR_SET:			// (char* var_name, char* value)
    case BOTLIB_AI_TOUCHING_GOAL:		// (vec3_t origin, void /* struct bot_goal_s* */ goal)
    case BOTLIB_AI_GET_MAP_LOCATION_GOAL:	// (char* name, void /* struct bot_goal_s* */ goal)
    case G_PERPENDICULARVECTOR:		// (vec3_t dst, const vec3_t src)
        ret = qmm_syscall(cmd, vmptr(0), vmptr(1));
        break;
    case G_AREAS_CONNECTED:			// (int area1, int area2);
    case BOTLIB_GET_SNAPSHOT_ENTITY:	// (int clientNum, int sequence)
    case BOTLIB_AAS_ENABLE_ROUTING_AREA:	// (int areanum, int enable)
    case BOTLIB_EA_ACTION:			// (int client, int action)
    case BOTLIB_EA_SELECT_WEAPON:		// (int client, int weapon)
    case BOTLIB_EA_END_REGULAR:		// (int client, float thinktime)
    case BOTLIB_AI_CHARACTERISTIC_FLOAT:	// (int character, int index)
    case BOTLIB_AI_CHARACTERISTIC_INTEGER:	// (int character, int index)
    case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:	// (int chatstate, int handle)
    case BOTLIB_AI_SET_CHAT_GENDER:		// (int chatstate, int gender)
    case BOTLIB_AI_REMOVE_FROM_AVOID_GOALS:	// (int goalstate, int number)
    case BOTLIB_AI_AVOID_GOAL_TIME:		// (int goalstate, int number)
    case BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC: // (int goalstate, float range)
    case G_ATAN2:				// (double, double)
        ret = qmm_syscall(cmd, vmarg(0), vmarg(1));
        break;
    case G_SEND_CONSOLE_COMMAND:		// (int exec_when, const char* text)
    case G_DROP_CLIENT:			// (int clientNum, const char* reason);
    case G_SEND_SERVER_COMMAND:		// (int clientNum, const char* fmt);
    case G_SET_CONFIGSTRING:		// (int num, const char* string);
    case G_SET_USERINFO:			// (int num, const char* buffer);
    case G_GET_USERCMD:			// (int clientNum, usercmd_t* cmd)
    case BOTLIB_UPDATENTITY:		// (int ent, void /* struct bot_updateentity_s* */ bue)
    case BOTLIB_USER_COMMAND:		// (int clientNum, usercmd_t* ucmd)
    case BOTLIB_AAS_ENTITY_INFO:		// (int entnum, void /* struct aas_entityinfo_s* */ info)
    case BOTLIB_AAS_AREA_INFO:		// (int areanum, void /* struct aas_areainfo_s* */ info)
    case BOTLIB_EA_SAY:			// (int client, char* str)
    case BOTLIB_EA_SAY_TEAM:		// (int client, char* str)
    case BOTLIB_EA_COMMAND:			// (int client, char* command)
    case BOTLIB_EA_VIEW:			// (int client, vec3_t viewangles)
    case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:	// (int chatstate, void /* struct bot_consolemessage_s* */ cm)
    case BOTLIB_AI_NUM_INITIAL_CHATS:	// (int chatstate, char* type)
    case BOTLIB_AI_PUSH_GOAL:		// (int goalstate, void* goal)
    case BOTLIB_AI_GET_TOP_GOAL:		// (int goalstate, void /* struct bot_goal_s* */ goal)
    case BOTLIB_AI_GET_SECOND_GOAL:		// (int goalstate, void /* struct bot_goal_s* */ goal)
    case BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:	// (int num, void /* struct bot_goal_s* */ goal)
    case BOTLIB_AI_LOAD_ITEM_WEIGHTS:	// (int, char*)
    case BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:	// (int, char*)
    case BOTLIB_AI_INIT_MOVE_STATE:		// (int handle, void* initmove)
    case BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:	// (int weaponstate, int* inventory)
    case BOTLIB_AI_LOAD_WEAPON_WEIGHTS:	// (int, char*)
    case BOTLIB_PC_READ_TOKEN:		// (int, void*)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1));
        break;
    case G_GET_SERVERINFO:			// (char* buffer, int bufferSize);
    case G_POINT_CONTENTS:			// (const vec3_t point, int passEntityNum);
    case G_ADJUST_AREA_PORTAL_STATE:	// (gentity_t* ent, qboolean open);
    case G_GET_ENTITY_TOKEN:		// (char* buffer, int bufferSize)
    case BOTLIB_AI_LOAD_CHARACTER:		// (char* charfile, float skill)
    case BOTLIB_AI_REPLACE_SYNONYMS:	// (char* string, unsigned long int context)
    case BOTLIB_AI_REACHABILITY_AREA:	// (vec3_t origin, int testground)
    case G_TESTPRINTINT:			// (char*, int)
    case G_TESTPRINTFLOAT:			// (char*, float)
        ret = qmm_syscall(cmd, vmptr(0), vmarg(1));
        break;
    case G_ARGV:				// (int n, char* buffer, int bufferLength);
    case G_GET_CONFIGSTRING:		// (int num, char* buffer, int bufferSize);
    case G_GET_USERINFO:			// (int num, char* buffer, int bufferSize);
    case BOTLIB_GET_CONSOLE_MESSAGE:	// (int clientNum, char* message, int size)
    case BOTLIB_EA_MOVE:			// (int client, vec3_t dir, float speed)
    case BOTLIB_AI_GET_CHAT_MESSAGE:	// (int chatstate, char* buf, int size)
    case BOTLIB_AI_SET_CHAT_NAME:		// (int chatstate, char* name, int client)
    case BOTLIB_AI_GOAL_NAME:		// (int number, char* name, int size)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmarg(2));
        break;
    case G_CVAR_VARIABLE_STRING_BUFFER:	// (const char* var_name, char* buffer, int bufsize);
    case G_FS_FOPEN_FILE:			// (const char* qpath, fileHandle_t* file, fsMode_t mode);
    case BOTLIB_LIBVAR_GET:			// (char* var_name, char* value, int size)
    case BOTLIB_AI_STRING_CONTAINS:		// (char* str1, char* str2, int casesensitive)
    case BOTLIB_AI_FIND_MATCH:		// (char* str, void /* struct bot_match_s* */ match, unsigned long int context)
    case G_MEMCPY:				// (void* dest, const void* src, size_t count)
    case G_STRNCPY:				// (char* strDest, const char* strSource, size_t count)
        ret = qmm_syscall(cmd, vmptr(0), vmptr(1), vmarg(2));
        break;
    case G_FS_READ:				// (void* buffer, int len, fileHandle_t f);
    case G_FS_WRITE:			// (const void* buffer, int len, fileHandle_t f);
    case G_MEMSET:				// (void* dest, int c, size_t count)
        ret = qmm_syscall(cmd, vmptr(0), vmarg(1), vmarg(2));
        break;
    case G_ENTITY_CONTACT:			// (const vec3_t mins, const vec3_t maxs, const gentity_t* ent);
    case G_MATRIXMULTIPLY:			// (float in1[3][3], float in2[3][3], float out[3][3])
        ret = qmm_syscall(cmd, vmptr(0), vmptr(1), vmptr(2));
        break;
    case G_ENTITY_CONTACTCAPSULE:		// (const vec3_t mins, const vec3_t maxs, const gentity_t* ent);
    case G_FS_SEEK:				// (fileHandle_t f, long offset, int origin)
    case BOTLIB_AI_ENTER_CHAT:		// (int chatstate, int client, int sendto)
    case BOTLIB_AI_SET_AVOID_GOAL_TIME:	// (int goalstate, int number, float avoidtime)
    case BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:	// (int,int,int)
        ret = qmm_syscall(cmd, vmarg(0), vmarg(1), vmarg(2));
        break;
    case G_DEBUG_POLYGON_CREATE:		// (int color, int numPoints, vec3_t* points)
    case BOTLIB_EA_GET_INPUT:		// (int client, float thinktime, void /* struct bot_input_s* */ input)
    case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:	// (int chatstate, int type, char* message)
    case BOTLIB_AI_GET_WEAPON_INFO:		// (int weaponstate, int weapon, void /* struct weaponinfo_s* */ weaponinfo)
        ret = qmm_syscall(cmd, vmarg(0), vmarg(1), vmptr(2));
        break;
    case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:	// (int presencetype, vec3_t mins, vec3_t maxs)
    case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:	// (int ent, char* key, vec3_t v)
    case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:	// (int ent, char* key, float* value)
    case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:	// (int ent, char* key, int* value)
    case BOTLIB_AI_LOAD_CHAT_FILE:		// (int chatstate, char* chatfile, char* chatname)
    case BOTLIB_AI_GET_LEVEL_ITEM_GOAL:	// (int index, char* classname, void /* struct bot_goal_s* */ goal)
    case BOTLIB_PC_SOURCE_FILE_AND_LINE:	// (int handle, char* filename, int* line)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmptr(2));
        break;
    case G_CVAR_REGISTER:			// (vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags);
    case G_ENTITIES_IN_BOX:			// (const vec3_t mins, const vec3_t maxs, gentity_t* *list, int maxcount);
    case G_FS_GETFILELIST:			// ( const char* path, const char* extension, char* listbuf, int bufsize) {
    case BOTLIB_AAS_BBOX_AREAS:		// (vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas)
        ret = qmm_syscall(cmd, vmptr(0), vmptr(1), vmptr(2), vmarg(3));
        break;
    case BOTLIB_TEST:			// (int parm0, char* parm1, vec3_t parm2, vec3_t parm3)
    case BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:	// (int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s* */ goal)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmptr(2), vmptr(3));
        break;
    case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:	// (int ent, char* key, char* value, int size)
    case BOTLIB_AI_CHOOSE_LTG_ITEM:		// (int goalstate, vec3_t origin, int* inventory, int travelflags)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmptr(2), vmarg(3));
        break;
    case G_ANGLEVECTORS:			// (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
        ret = qmm_syscall(cmd, vmptr(0), vmptr(1), vmptr(2), vmptr(3));
        break;
    case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:	// (int areanum, vec3_t origin, int goalareanum, int travelflags)
    case BOTLIB_AI_ADD_AVOID_SPOT:		// (int movestate, vec3_t origin, float radius, int type)
    case BOTLIB_AI_MOVE_IN_DIRECTION:	// (int movestate, vec3_t dir, float speed, int type)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmarg(2), vmarg(3));
        break;
    case BOTLIB_AI_CHARACTERISTIC_BFLOAT:	// (int character, int index, float min, float max)
    case BOTLIB_AI_CHARACTERISTIC_BINTEGER:	// (int character, int index, int min, int max)
        ret = qmm_syscall(cmd, vmarg(0), vmarg(1), vmarg(2), vmarg(3));
        break;
    case BOTLIB_AI_MATCH_VARIABLE:		// (void /* struct bot_match_s* */ match, int variable, char* buf, int size)
    case BOTLIB_AI_MOVE_TO_GOAL:		// (void /* struct bot_moveresult_s* */ result, int movestate, void /* struct bot_goal_s* */ goal, int travelflags)
        ret = qmm_syscall(cmd, vmptr(0), vmarg(1), vmptr(2), vmarg(3));
        break;
    case BOTLIB_AI_CHARACTERISTIC_STRING:	// (int character, int index, char* buf, int size)
        ret = qmm_syscall(cmd, vmarg(0), vmarg(1), vmptr(2), vmarg(3));
        break;
    case G_LOCATE_GAME_DATA:		// (gentity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient);
        ret = qmm_syscall(cmd, vmptr(0), vmarg(1), vmarg(2), vmptr(3), vmarg(4));
        break;
    case BOTLIB_AAS_TRACE_AREAS:		// (vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas)
        ret = qmm_syscall(cmd, vmptr(0), vmptr(1), vmptr(2), vmptr(3), vmarg(4));
        break;
    case BOTLIB_AI_MOVEMENT_VIEW_TARGET:	// (int movestate, void /* struct bot_goal_s* */ goal, int travelflags, float lookahead, vec3_t tvmarget)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmarg(2), vmarg(3), vmptr(4));
        break;
    case BOTLIB_AI_PREDICT_VISIBLE_POSITION:	// (vec3_t origin, int areanum, void /* struct bot_goal_s* */ goal, int travelflags, vec3_t tvmarget)
        ret = qmm_syscall(cmd, vmptr(0), vmarg(1), vmptr(2), vmarg(3), vmptr(4));
        break;
    case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:	// (int numranks, float* ranks, int* parent1, int* parent2, int* child)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmptr(2), vmptr(3), vmptr(4));
        break;
    case G_TRACECAPSULE:			// (trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
        ret = qmm_syscall(cmd, vmarg(0), vmarg(1), vmarg(2), vmarg(3), vmarg(4), vmarg(5));
        break;
    case BOTLIB_AI_CHOOSE_NBG_ITEM:		// (int goalstate, vec3_t origin, int* inventory, int travelflags, void /* struct bot_goal_s* */ ltg, float maxtime)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmptr(2), vmarg(3), vmptr(4), vmarg(5));
        break;
    case G_TRACE:				// (trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
        ret = qmm_syscall(cmd, vmptr(0), vmptr(1), vmptr(2), vmptr(3), vmptr(4), vmarg(5), vmarg(6));
        break;
    case BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL:	// (vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags, void /* struct aas_altroutegoal_s* */ altroutegoals, int maxaltroutegoals, int type)
        ret = qmm_syscall(cmd, vmptr(0), vmarg(1), vmptr(2), vmarg(3), vmarg(4), vmptr(5), vmarg(6), vmarg(7));
        break;
    case BOTLIB_AI_INITIAL_CHAT:		// (int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmarg(2), vmptr(3), vmptr(4), vmptr(5), vmptr(6), vmptr(7), vmptr(8), vmptr(9), vmptr(10));
        break;
    case BOTLIB_AAS_PREDICT_ROUTE:		// (void /* struct aas_predictroute_s* */ route, int areanum, vec3_t origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum)
        ret = qmm_syscall(cmd, vmptr(0), vmarg(1), vmptr(2), vmarg(3), vmarg(4), vmarg(5), vmarg(6), vmarg(7), vmarg(8), vmarg(9), vmarg(10), vmarg(11));
        break;
    case BOTLIB_AI_REPLY_CHAT:		// (int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
        ret = qmm_syscall(cmd, vmarg(0), vmptr(1), vmarg(2), vmarg(3), vmptr(4), vmptr(5), vmptr(6), vmptr(7), vmptr(8), vmptr(9), vmptr(10), vmptr(11));
        break;
    case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:	// (void /* struct aas_clientmove_s* */ move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize)
        ret = qmm_syscall(cmd, vmptr(0), vmarg(1), vmptr(2), vmarg(3), vmarg(4), vmptr(5), vmptr(6), vmarg(7), vmarg(8), vmarg(9), vmarg(10), vmarg(11), vmarg(12));
        break;

    default:
        ret = 0;
    }

#ifdef _DEBUG
    LOG(QMM_LOG_TRACE, "QMM") << fmt::format("Q3A_vmsyscall({} {}) returning {}\n", Q3A_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}
