/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifdef QMM_FEATURE_SOF2SP

//#include <sof2sp/game/q_shared.h>
//#include <sof2sp/game/g_public.h>

#include "game_api.h"
#include "log.h"
// QMM-specific SOF2SP header
#include "game_sof2sp.h"
#include "main.h"
#include "util.h"

GEN_QMM_MSGS(SOF2SP);
GEN_EXTS(SOF2SP);

// a copy of the apiversion int that comes from the game engine
static intptr_t orig_apiversion = 0;

// a copy of the original import struct that comes from the game engine
static game_import_t orig_import;

// a copy of the original export struct pointer that comes from the mod
static game_export_t* orig_export = nullptr;

// struct with lambdas that call QMM's syscall function. this is given to the mod
static game_import_t qmm_import = {
    GEN_IMPORT(Printf, G_PRINTF),
    GEN_IMPORT(ptr1, 1),
    GEN_IMPORT(ptr2, 2),
    GEN_IMPORT(ptr3, 3),
    GEN_IMPORT(ptr4, 4),
    GEN_IMPORT(ptr5, 5),
    GEN_IMPORT(ptr6, 6),
    GEN_IMPORT(ptr7, 7),
    GEN_IMPORT(ptr8, 8),
    GEN_IMPORT(ptr9, 9),
    GEN_IMPORT(ptr10, 10),
    GEN_IMPORT(ptr11, 11),
    GEN_IMPORT(ptr12, 12),
    GEN_IMPORT(ptr13, 13),
    GEN_IMPORT(ptr14, 14),
    GEN_IMPORT(ptr15, 15),
    GEN_IMPORT(ptr16, 16),
    GEN_IMPORT(ptr17, 17),
    GEN_IMPORT(ptr18, 18),
    GEN_IMPORT(ptr19, 19),
    GEN_IMPORT(ptr20, 20),
    GEN_IMPORT(ptr21, 21),
    GEN_IMPORT(ptr22, 22),
    GEN_IMPORT(ptr23, 23),
    GEN_IMPORT(ptr24, 24),
    GEN_IMPORT(ptr25, 25),
    GEN_IMPORT(ptr26, 26),
    GEN_IMPORT(ptr27, 27),
    GEN_IMPORT(ptr28, 28),
    GEN_IMPORT(ptr29, 29),
    GEN_IMPORT(ptr30, 30),
    GEN_IMPORT(ptr31, 31),
    GEN_IMPORT(ptr32, 32),
    GEN_IMPORT(ptr33, 33),
    GEN_IMPORT(ptr34, 34),
    GEN_IMPORT(ptr35, 35),
    GEN_IMPORT(ptr36, 36),
    GEN_IMPORT(ptr37, 37),
    GEN_IMPORT(ptr38, 38),
    GEN_IMPORT(ptr39, 39),
    GEN_IMPORT(ptr40, 40),
    GEN_IMPORT(ptr41, 41),
    GEN_IMPORT(ptr42, 42),
    GEN_IMPORT(ptr43, 43),
    GEN_IMPORT(ptr44, 44),
    GEN_IMPORT(ptr45, 45),
    GEN_IMPORT(ptr46, 46),
    GEN_IMPORT(ptr47, 47),
    GEN_IMPORT(ptr48, 48),
    GEN_IMPORT(ptr49, 49),
    GEN_IMPORT(ptr50, 50),
    GEN_IMPORT(ptr51, 51),
    GEN_IMPORT(ptr52, 52),
    GEN_IMPORT(ptr53, 53),
    GEN_IMPORT(ptr54, 54),
    GEN_IMPORT(ptr55, 55),
    GEN_IMPORT(ptr56, 56),
    GEN_IMPORT(ptr57, 57),
    GEN_IMPORT(ptr58, 58),
    GEN_IMPORT(ptr59, 59),
    GEN_IMPORT(ptr60, 60),
    GEN_IMPORT(ptr61, 61),
    GEN_IMPORT(ptr62, 62),
    GEN_IMPORT(ptr63, 63),
    GEN_IMPORT(ptr64, 64),
    GEN_IMPORT(ptr65, 65),
    GEN_IMPORT(ptr66, 66),
    GEN_IMPORT(ptr67, 67),
    GEN_IMPORT(ptr68, 68),
    GEN_IMPORT(ptr69, 69),
    GEN_IMPORT(ptr70, 70),
    GEN_IMPORT(ptr71, 71),
    GEN_IMPORT(ptr72, 72),
    GEN_IMPORT(ptr73, 73),
    GEN_IMPORT(ptr74, 74),
    GEN_IMPORT(ptr75, 75),
    GEN_IMPORT(ptr76, 76),
    GEN_IMPORT(ptr77, 77),
    GEN_IMPORT(ptr78, 78),
    GEN_IMPORT(ptr79, 79),
    GEN_IMPORT(ptr80, 80),
    GEN_IMPORT(ptr81, 81),
    GEN_IMPORT(ptr82, 82),
    GEN_IMPORT(ptr83, 83),
    GEN_IMPORT(ptr84, 84),
    GEN_IMPORT(ptr85, 85),
    GEN_IMPORT(ptr86, 86),
    GEN_IMPORT(ptr87, 87),
    GEN_IMPORT(ptr88, 88),
    GEN_IMPORT(ptr89, 89),
    GEN_IMPORT(ptr90, 90),
    GEN_IMPORT(ptr91, 91),
    GEN_IMPORT(ptr92, 92),
    GEN_IMPORT(ptr93, 93),
    GEN_IMPORT(ptr94, 94),
    GEN_IMPORT(ptr95, 95),
    GEN_IMPORT(ptr96, 96),
    GEN_IMPORT(ptr97, 97),
    GEN_IMPORT(ptr98, 98),
    GEN_IMPORT(ptr99, 99),
    GEN_IMPORT(ptr100, 100),
    GEN_IMPORT(ptr101, 101),
    GEN_IMPORT(ptr102, 102),
    GEN_IMPORT(ptr103, 103),
    GEN_IMPORT(ptr104, 104),
    GEN_IMPORT(ptr105, 105),
    GEN_IMPORT(ptr106, 106),
    GEN_IMPORT(ptr107, 107),
    GEN_IMPORT(ptr108, 108),
    GEN_IMPORT(ptr109, 109),
    GEN_IMPORT(ptr110, 110),
    GEN_IMPORT(ptr111, 111),
    GEN_IMPORT(ptr112, 112),
};


// these are "pre" hooks for storing some data for polyfills.
// we need these to be called BEFORE plugins' prehooks get called so they have to be done in the qmm_export table

// track entstrings for our G_GET_ENTITY_TOKEN syscall
/*
static std::vector<std::string> s_entity_tokens;
static size_t s_tokencount = 0;
static void SOF2SP_Init(const char* mapname, const char* spawntarget, int checkSum, const char* entstring, int levelTime, int randomSeed, int globalTime, SavedGameJustLoaded_e eSavedGameJustLoaded, qboolean qbLoadTransition) {
    if (entstring) {
        s_entity_tokens = util_parse_entstring(entstring);
        s_tokencount = 0;
    }
    cgame_is_QMM_vmMain_call = true;
    vmMain(GAME_INIT, mapname, spawntarget, checkSum, entstring, levelTime, randomSeed, globalTime, eSavedGameJustLoaded, qbLoadTransition);
}
*/

// struct with lambdas that call QMM's vmMain function. this is given to the game engine
static game_export_t qmm_export = {
    GEN_EXPORT(Init, GAME_INIT),
    GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
    GEN_EXPORT(ClientConnect, GAME_CLIENT_CONNECT),
    GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
    GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
    GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
    GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
    GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
    GEN_EXPORT(return2030F65D, GAME_RETURN_2030F65D),
    GEN_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND),
    0,
    GEN_EXPORT(SpawnRMGEntity, GAME_SPAWN_RMG_ENTITY),
    GEN_EXPORT(arioche, GAME_ARIOCHE),
    GEN_EXPORT(EntityList, GAME_ENTITY_LIST),
    GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
    0,
    GEN_EXPORT(unknown16, GAME_UNKNOWN16),
    GEN_EXPORT(Save, GAME_SAVE),
    GEN_EXPORT(GameAllowedToSaveHere, GAME_GAMEALLOWEDTOSAVEHERE),
    GEN_EXPORT(CanPlayCinematic, GAME_CAN_PLAY_CINEMATIC),
    GEN_EXPORT(unknown20, GAME_UNKNOWN20),
    GEN_EXPORT(unknown21, GAME_UNKNOWN21),
    GEN_EXPORT(unknown22, GAME_UNKNOWN22),
    GEN_EXPORT(unknown23, GAME_UNKNOWN23),
    GEN_EXPORT(unknown24, GAME_UNKNOWN24),
    GEN_EXPORT(unknown25, GAME_UNKNOWN25),
};


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t SOF2SP_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("SOF2SP_syscall({} {}) called\n", SOF2SP_eng_msg_names(cmd), cmd);
#endif

    intptr_t ret = 0;

    if (cmd < 200) {
        switch (cmd) {
            ROUTE_IMPORT(Printf, G_PRINTF);
        default: {
            pfn_call_t* p = (pfn_call_t*)&orig_import.Printf;
            ret = (p[cmd])(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
            break;
        }
        };
    }

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_TRACE, "QMM") << fmt::format("SOF2SP_syscall({} {}) returning {}\n", SOF2SP_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t SOF2SP_vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_vmMain({} {}) called\n", SOF2SP_mod_msg_names(cmd), cmd);

    if (!orig_export)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    if (cmd == GAME_INIT) {
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Init({}, {}, {})\n", args[0], args[1], args[2]);
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Init({}, {}, {})\n", (void*)args[0], (void*)args[1], (void*)args[2]);
    }

    switch (cmd) {
        ROUTE_EXPORT(Init, GAME_INIT);
        ROUTE_EXPORT(Shutdown, GAME_SHUTDOWN);
        ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
        ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
        ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
        ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
        ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
        ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
        ROUTE_EXPORT(return2030F65D, GAME_RETURN_2030F65D);
        ROUTE_EXPORT(ConsoleCommand, GAME_CONSOLE_COMMAND);
        ROUTE_EXPORT(SpawnRMGEntity, GAME_SPAWN_RMG_ENTITY);
        ROUTE_EXPORT(arioche, GAME_ARIOCHE);
        ROUTE_EXPORT(EntityList, GAME_ENTITY_LIST);
        ROUTE_EXPORT(WriteLevel, GAME_WRITE_LEVEL);
        ROUTE_EXPORT(unknown16, GAME_UNKNOWN16);
        ROUTE_EXPORT(Save, GAME_SAVE);
        ROUTE_EXPORT(GameAllowedToSaveHere, GAME_GAMEALLOWEDTOSAVEHERE);
        ROUTE_EXPORT(CanPlayCinematic, GAME_CAN_PLAY_CINEMATIC);
        ROUTE_EXPORT(unknown20, GAME_UNKNOWN20);
        ROUTE_EXPORT(unknown21, GAME_UNKNOWN21);
        ROUTE_EXPORT(unknown22, GAME_UNKNOWN22);
        ROUTE_EXPORT(unknown23, GAME_UNKNOWN23);
        ROUTE_EXPORT(unknown24, GAME_UNKNOWN24);
        ROUTE_EXPORT(unknown25, GAME_UNKNOWN25);

        // handle cmds for variables, this is how a plugin would get these values if needed
        ROUTE_EXPORT_VAR(unknown10, GAMEV_UNKNOWN10);
        ROUTE_EXPORT_VAR(unknown15, GAMEV_UNKNOWN15);
    default:
        break;
    };

    // after the mod is called into by the engine, some of the variables in the mod's exports may have changed (num_entities and errorMessage in particular)
    // and these changes need to be available to the engine, so copy those values again now before returning from the mod
    qmm_export.unknown10 = orig_export->unknown10;
    qmm_export.unknown15 = orig_export->unknown15;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_vmMain({} {}) returning {}\n", SOF2SP_mod_msg_names(cmd), cmd, ret);

    return ret;
}


void* SOF2SP_GetGameAPI(void* apiversion, void* import) {
    orig_apiversion = (intptr_t)apiversion;
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_GetGameAPI({}, {}) called\n", orig_apiversion, import);

    // original import struct from engine
    // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
    game_import_t* gi = (game_import_t*)import;
    orig_import = *gi;

    // fill in variables of our hooked import struct to pass to the mod
    // qmm_import.unknown = orig_import.unknown;

    //memcpy(&qmm_import, &orig_import, sizeof(qmm_import));

    // pointer to wrapper vmMain function that calls actual mod func from orig_export
    g_gameinfo.pfnvmMain = SOF2SP_vmMain;

    // pointer to wrapper syscall function that calls actual engine func from orig_import
    g_gameinfo.pfnsyscall = SOF2SP_syscall;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_GetGameAPI({}, {}) returning {}\n", orig_apiversion, import, (void*)&qmm_export);

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // the only thing in this struct the engine uses before calling Init is the apiversion
    return &qmm_export;
}


bool SOF2SP_mod_load(void* entry) {
    mod_GetGameAPI_t mod_GetGameAPI = (mod_GetGameAPI_t)entry;
    // api version gets passed before import pointer
    orig_export = (game_export_t*)mod_GetGameAPI((void*)orig_apiversion, &qmm_import);

    return !!orig_export;
}


void SOF2SP_mod_unload() {
    orig_export = nullptr;
}


const char* SOF2SP_eng_msg_names(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_PRINTF);
        /*
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
*/

    default:
        return "unknown";
    }
}


const char* SOF2SP_mod_msg_names(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(GAME_INIT);
        GEN_CASE(GAME_SHUTDOWN);
        GEN_CASE(GAME_CLIENT_CONNECT);
        GEN_CASE(GAME_CLIENT_BEGIN);
        GEN_CASE(GAME_CLIENT_DISCONNECT);
        GEN_CASE(GAME_CLIENT_COMMAND);
        GEN_CASE(GAME_CLIENT_THINK);
        GEN_CASE(GAME_RUN_FRAME);
        GEN_CASE(GAME_RETURN_2030F65D);
        GEN_CASE(GAME_CONSOLE_COMMAND);
        GEN_CASE(GAMEV_UNKNOWN10);
        GEN_CASE(GAME_SPAWN_RMG_ENTITY);
        GEN_CASE(GAME_ARIOCHE);
        GEN_CASE(GAME_ENTITY_LIST);
        GEN_CASE(GAME_WRITE_LEVEL);
        GEN_CASE(GAMEV_UNKNOWN15);
        GEN_CASE(GAME_UNKNOWN16);
        GEN_CASE(GAME_SAVE);
        GEN_CASE(GAME_GAMEALLOWEDTOSAVEHERE);
        GEN_CASE(GAME_CAN_PLAY_CINEMATIC);
        GEN_CASE(GAME_UNKNOWN20);
        GEN_CASE(GAME_UNKNOWN21);
        GEN_CASE(GAME_UNKNOWN22);
        GEN_CASE(GAME_UNKNOWN23);
        GEN_CASE(GAME_UNKNOWN24);
        GEN_CASE(GAME_UNKNOWN25);

    default:
        return "unknown";
    }
}
#endif // QMM_FEATURE_SOF2SP
