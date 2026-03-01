/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <wet/game/q_shared.h>
#include <wet/game/g_public.h>

#include "game_api.h"
#include "log.h"
// QMM-specific WET header
#include "game_wet.h"
#include "main.h"
#include "mod.h"

GEN_QMM_MSGS(WET);
GEN_EXTS(WET);

// original syscall pointer that comes from the game engine
static eng_syscall orig_syscall = nullptr;

// pointer to vmMain that comes from the mod
static mod_vmMain orig_vmMain = nullptr;

// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t WET_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("WET_syscall({} {}) called\n", WET_eng_msg_names(cmd), cmd);
#endif

    intptr_t ret = 0;

    switch (cmd) {
    // handle special cmds which QMM uses but WET doesn't have an analogue for
    case G_ARGS: {
        // quake2: char* (*args)(void);
        static std::string s;
        static char buf[MAX_STRING_CHARS];
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
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("WET_syscall({} {}) returning {}\n", WET_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t WET_vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("WET_vmMain({} {}) called\n", WET_mod_msg_names(cmd), cmd);
#endif

    if (!orig_vmMain)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    // all normal mod functions go to mod
    ret = orig_vmMain(cmd, QMM_PUT_VMMAIN_ARGS());

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("WET_vmMain({} {}) returning {}\n", WET_mod_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


void WET_dllEntry(eng_syscall syscall) {
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("WET_dllEntry({}) called\n", (void*)syscall);

    // store original syscall from engine
    orig_syscall = syscall;

    // pointer to wrapper vmMain function that calls actual mod vmMain func orig_vmMain
    g_gameinfo.pfnvmMain = WET_vmMain;

    // pointer to wrapper syscall function that calls actual engine syscall func
    g_gameinfo.pfnsyscall = WET_syscall;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("WET_dllEntry({}) returning\n", (void*)syscall);
}


bool WET_mod_load(void* entry) {
    orig_vmMain = (mod_vmMain)entry;

    return !!orig_vmMain;
}


void WET_mod_unload() {
    orig_vmMain = nullptr;
}


const char* WET_eng_msg_names(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_PRINT);
        GEN_CASE(G_ERROR);
        GEN_CASE(G_MILLISECONDS);
        GEN_CASE(G_CVAR_REGISTER);
        GEN_CASE(G_CVAR_UPDATE);
        GEN_CASE(G_CVAR_SET);
        GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
        GEN_CASE(G_CVAR_LATCHEDVARIABLESTRINGBUFFER);
        GEN_CASE(G_ARGC);
        GEN_CASE(G_ARGV);
        GEN_CASE(G_FS_FOPEN_FILE);
        GEN_CASE(G_FS_READ);
        GEN_CASE(G_FS_WRITE);
        GEN_CASE(G_FS_RENAME);
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
        GEN_CASE(G_GETTAG);
        GEN_CASE(G_REGISTERTAG);
        GEN_CASE(G_REGISTERSOUND);
        GEN_CASE(G_GET_SOUND_LENGTH);
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
        GEN_CASE(BOTLIB_AAS_ENTITY_VISIBLE);
        GEN_CASE(BOTLIB_AAS_IN_FIELD_OF_VISION);
        GEN_CASE(BOTLIB_AAS_VISIBLE_CLIENTS);
        GEN_CASE(BOTLIB_AAS_ENTITY_INFO);
        GEN_CASE(BOTLIB_AAS_INITIALIZED);
        GEN_CASE(BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX);
        GEN_CASE(BOTLIB_AAS_TIME);
        GEN_CASE(BOTLIB_AAS_SETCURRENTWORLD);
        GEN_CASE(BOTLIB_AAS_POINT_AREA_NUM);
        GEN_CASE(BOTLIB_AAS_TRACE_AREAS);
        GEN_CASE(BOTLIB_AAS_BBOX_AREAS);
        GEN_CASE(BOTLIB_AAS_AREA_CENTER);
        GEN_CASE(BOTLIB_AAS_AREA_WAYPOINT);
        GEN_CASE(BOTLIB_AAS_POINT_CONTENTS);
        GEN_CASE(BOTLIB_AAS_NEXT_BSP_ENTITY);
        GEN_CASE(BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY);
        GEN_CASE(BOTLIB_AAS_AREA_REACHABILITY);
        GEN_CASE(BOTLIB_AAS_AREA_LADDER);
        GEN_CASE(BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA);
        GEN_CASE(BOTLIB_AAS_SWIMMING);
        GEN_CASE(BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT);
        GEN_CASE(BOTLIB_AAS_RT_SHOWROUTE);
        // GEN_CASE(BOTLIB_AAS_RT_GETHIDEPOS);
        // GEN_CASE(BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE);
        GEN_CASE(BOTLIB_AAS_NEARESTHIDEAREA);
        GEN_CASE(BOTLIB_AAS_LISTAREASINRANGE);
        GEN_CASE(BOTLIB_AAS_AVOIDDANGERAREA);
        GEN_CASE(BOTLIB_AAS_RETREAT);
        GEN_CASE(BOTLIB_AAS_ALTROUTEGOALS);
        GEN_CASE(BOTLIB_AAS_SETAASBLOCKINGENTITY);
        GEN_CASE(BOTLIB_AAS_RECORDTEAMDEATHAREA);
        GEN_CASE(BOTLIB_EA_SAY);
        GEN_CASE(BOTLIB_EA_SAY_TEAM);
        GEN_CASE(BOTLIB_EA_USE_ITEM);
        GEN_CASE(BOTLIB_EA_DROP_ITEM);
        GEN_CASE(BOTLIB_EA_USE_INV);
        GEN_CASE(BOTLIB_EA_DROP_INV);
        GEN_CASE(BOTLIB_EA_GESTURE);
        GEN_CASE(BOTLIB_EA_COMMAND);
        GEN_CASE(BOTLIB_EA_SELECT_WEAPON);
        GEN_CASE(BOTLIB_EA_TALK);
        GEN_CASE(BOTLIB_EA_ATTACK);
        GEN_CASE(BOTLIB_EA_RELOAD);
        GEN_CASE(BOTLIB_EA_USE);
        GEN_CASE(BOTLIB_EA_RESPAWN);
        GEN_CASE(BOTLIB_EA_JUMP);
        GEN_CASE(BOTLIB_EA_DELAYED_JUMP);
        GEN_CASE(BOTLIB_EA_CROUCH);
        GEN_CASE(BOTLIB_EA_WALK);
        GEN_CASE(BOTLIB_EA_MOVE_UP);
        GEN_CASE(BOTLIB_EA_MOVE_DOWN);
        GEN_CASE(BOTLIB_EA_MOVE_FORWARD);
        GEN_CASE(BOTLIB_EA_MOVE_BACK);
        GEN_CASE(BOTLIB_EA_MOVE_LEFT);
        GEN_CASE(BOTLIB_EA_MOVE_RIGHT);
        GEN_CASE(BOTLIB_EA_MOVE);
        GEN_CASE(BOTLIB_EA_VIEW);
        GEN_CASE(BOTLIB_EA_PRONE);
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
        GEN_CASE(BOTLIB_AI_INIT_AVOID_REACH);
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
        GEN_CASE(BOTLIB_PC_UNREAD_TOKEN);
        GEN_CASE(PB_STAT_REPORT);
        GEN_CASE(G_SENDMESSAGE);
        GEN_CASE(G_MESSAGESTATUS);
        GEN_CASE(G_TRAP_GETVALUE);
        GEN_CASE(TVG_GET_PLAYERSTATE);
        GEN_CASE(G_DEMOSUPPORT);
        GEN_CASE(G_SNAPSHOT_CALLBACK_EXT);
        GEN_CASE(G_SNAPSHOT_SETCLIENTMASK);

        // polyfills
        GEN_CASE(G_ARGS);

    default:
        return "unknown";
    }
}


const char* WET_mod_msg_names(intptr_t cmd) {
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
        GEN_CASE(GAME_SNAPSHOT_CALLBACK);
        GEN_CASE(BOTAI_START_FRAME);
        GEN_CASE(BOT_VISIBLEFROMPOS);
        GEN_CASE(BOT_CHECKATTACKATPOS);
        GEN_CASE(GAME_MESSAGERECEIVED);
        GEN_CASE(GAME_DEMOSTATECHANGED);
        GEN_CASE(GAME_SNAPSHOT_CALLBACK_EXT);
    default:
        return "unknown";
    }
}
