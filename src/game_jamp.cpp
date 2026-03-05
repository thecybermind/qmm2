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
// QMM-specific JAMP header
#include "game_jamp.h"
#include "main.h"
#include "util.h"

GEN_QMM_MSGS(JAMP);
GEN_EXTS(JAMP);

// store whether or not this mod was loaded as GetGameAPI or not
static bool s_is_GetGameAPI = false;

// do not use macro since this game supports both dllEntry and GetGameAPI entry points
static const char* JAMP_eng_msg_names(intptr_t);
static const char* JAMP_mod_msg_names(intptr_t);
static bool JAMP_autodetect(bool, supportedgame*);
static intptr_t JAMP_syscall_legacy(intptr_t cmd, ...);
static intptr_t JAMP_vmMain_legacy(intptr_t cmd, ...);
static void JAMP_dllEntry(eng_syscall);
static intptr_t JAMP_syscall_GGA(intptr_t cmd, ...);
static intptr_t JAMP_vmMain_GGA(intptr_t cmd, ...);
static void* JAMP_GetGameAPI(void*, void*);
static bool JAMP_mod_load(void*);
static void JAMP_mod_unload();
supportedgame_funcs JAMP_funcs = {
    JAMP_qmm_eng_msgs,
    JAMP_qmm_mod_msgs,
    JAMP_eng_msg_names,
    JAMP_mod_msg_names,
    JAMP_autodetect,
    nullptr,    // JAMP_qvmsyscall
    JAMP_dllEntry,
    JAMP_GetGameAPI,
    JAMP_mod_load, JAMP_mod_unload
};


// auto-detection logic for JAMP
static bool JAMP_autodetect(bool is_GetGameAPI, supportedgame* game) {
    // QMM filename must match default or an OpenJK temp filename (if DLL was pulled from .pk3)
    if (!str_striequal(g_gameinfo.qmm_file, game->dllname)
        && !str_striequal(g_gameinfo.qmm_file.substr(0, 3), "ojk")
        && !str_striequal(path_baseext(g_gameinfo.qmm_file), "tmp"))
        return false;

    if (!str_stristr(g_gameinfo.exe_file, "jamp") && !str_stristr(g_gameinfo.exe_file, "openjk.") && !str_stristr(g_gameinfo.exe_file, "openjkded"))
        return false;

    // store how we were loaded
    s_is_GetGameAPI = is_GetGameAPI;

    return true;
}


// original syscall pointer that comes from the game engine
static eng_syscall orig_syscall_legacy = nullptr;

// pointer to vmMain that comes from the mod
static mod_vmMain orig_vmMain_legacy = nullptr;


// wrapper syscall function that calls actual engine func in orig_syscall_legacy
// this is how QMM and plugins will call into the engine
static intptr_t JAMP_syscall_legacy(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_syscall_legacy({} {}) called\n", JAMP_eng_msg_names(cmd), cmd);
#endif

    if (s_is_GetGameAPI)
        return 0;

    intptr_t ret = 0;

    switch (cmd) {
    // handle special cmds which QMM uses but JAMP doesn't have an analogue for
    case G_ARGS: {
        // quake2: char* (*args)(void);
        static std::string s;
        static char buf[MAX_STRING_CHARS];
        s = "";
        int i = 1;
        while (i < orig_syscall_legacy(G_ARGC)) {
            orig_syscall_legacy(G_ARGV, buf, sizeof(buf));
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
        ret = orig_syscall_legacy(cmd, QMM_PUT_SYSCALL_ARGS());
    }

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_syscall_legacy({} {}) returning {}\n", JAMP_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func in orig_vmMain_legacy
// this is how QMM and plugins will call into the mod
static intptr_t JAMP_vmMain_legacy(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_vmMain_legacy({} {}) called\n", JAMP_mod_msg_names(cmd), cmd);
#endif

    if (s_is_GetGameAPI)
        return 0;

    if (!orig_vmMain_legacy)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    // all normal mod functions go to mod
    ret = orig_vmMain_legacy(cmd, QMM_PUT_VMMAIN_ARGS());

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_vmMain_legacy({} {}) returning {}\n", JAMP_mod_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


static void JAMP_dllEntry(eng_syscall syscall) {
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_dllEntry({}) called\n", (void*)syscall);

    // store how we were loaded
    s_is_GetGameAPI = false;

    // store original syscall from engine
    orig_syscall_legacy = syscall;

    // pointer to wrapper vmMain function that calls actual mod vmMain func orig_vmMain
    g_gameinfo.pfnvmMain = JAMP_vmMain_legacy;

    // pointer to wrapper syscall function that calls actual engine syscall func
    g_gameinfo.pfnsyscall = JAMP_syscall_legacy;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_dllEntry({}) returning\n", (void*)syscall);
}


// these variables are all for the GGA implementation for OpenJK, but can't add _GGA suffix
// to all because ROUTE_IMPORT/ROUTE_EXPORT macros use these names

// a copy of the apiversion int that comes from the game engine
static intptr_t orig_apiversion_GGA = 0;

// a copy of the original import struct that comes from the game engine
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod
static game_export_t* orig_export = nullptr;

// struct with lambdas that call QMM's syscall function. this is given to the mod
static game_import_t qmm_import = {
    GEN_IMPORT(Print, G_PRINT),
    // todo finish
};

// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
    GEN_EXPORT(InitGame, GAME_INIT),
    // todo finish
};


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
static intptr_t JAMP_syscall_GGA(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_syscall_GGA({} {}) called\n", JAMP_eng_msg_names(cmd), cmd);
#endif

    if (!s_is_GetGameAPI)
        return 0;

    intptr_t ret = 0;

    switch (cmd) {
        ROUTE_IMPORT(Print, G_PRINT);

    default:
        break;
    };

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_syscall_GGA({} {}) returning {}\n", JAMP_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
static intptr_t JAMP_vmMain_GGA(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_vmMain_GGA({} {}) called\n", JAMP_mod_msg_names(cmd), cmd);
#endif

    if (!s_is_GetGameAPI)
        return 0;

    if (!orig_export)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    switch (cmd) {
        ROUTE_EXPORT(InitGame, GAME_INIT);

    default:
        break;
    };

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_vmMain_GGA({} {}) returning {}\n", JAMP_mod_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


static void* JAMP_GetGameAPI(void* apiversion, void* import) {
    orig_apiversion_GGA = (intptr_t)apiversion;
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_GetGameAPI({}, {}) called\n", orig_apiversion_GGA, import);

    // store how we were loaded
    s_is_GetGameAPI = true;

    // original import struct from engine
    // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
    game_import_t* gi = (game_import_t*)import;
    orig_import = *gi;

    // fill in variables of our hooked import struct to pass to the mod

    // pointer to wrapper vmMain function that calls actual mod func from orig_export
    g_gameinfo.pfnvmMain = JAMP_vmMain_GGA;

    // pointer to wrapper syscall function that calls actual engine func from orig_import
    g_gameinfo.pfnsyscall = JAMP_syscall_GGA;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("JAMP_GetGameAPI({}, {}) returning {}\n", orig_apiversion_GGA, import, (void*)&qmm_export);

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // the only thing in this struct the engine uses before calling Init is the apiversion
    return &qmm_export;

}


static bool JAMP_mod_load(void* entry) {
    if (s_is_GetGameAPI) {
        mod_GetGameAPI pfnGGA = (mod_GetGameAPI)entry;
        // api version gets passed before import pointer
        orig_export = (game_export_t*)pfnGGA((void*)orig_apiversion_GGA, &qmm_import);

        return !!orig_export;
    }
    else {
        orig_vmMain_legacy = (mod_vmMain)entry;
        return !!orig_vmMain_legacy;
    }
}


static void JAMP_mod_unload() {
    orig_export = nullptr;
    orig_vmMain_legacy = nullptr;
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

        // polyfills
        GEN_CASE(G_ARGS);

    default:
        return "unknown";
    }
}
