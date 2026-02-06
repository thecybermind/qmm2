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
    GEN_IMPORT(DPrintf, G_DPRINTF),
    GEN_IMPORT(DPrintf2, G_DPRINTF2),
    GEN_IMPORT(snprintf, G_SNPRINTF),
    GEN_IMPORT(ErrorF, G_ERRORF),
    GEN_IMPORT(FS_FOpenFile, G_FS_FOPEN_FILE),
    GEN_IMPORT(FS_Read, G_FS_READ),
    GEN_IMPORT(FS_FileForHandle, G_FS_FILE_FOR_HANDLE),
    GEN_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE),
    GEN_IMPORT(FS_ReadFile, G_FS_READFILE),
    GEN_IMPORT(FS_FreeFile, G_FS_FREEFILE),
    GEN_IMPORT(FS_FileAvailable, G_FS_FILEAVAILABLE),
    GEN_IMPORT(FS_BuildOSPath, G_FS_BUILDOSPATH),
    GEN_IMPORT(unknown13, G_UNKNOWN13),
    GEN_IMPORT(unknown14, G_UNKNOWN14),
    GEN_IMPORT(Milliseconds, G_MILLISECONDS),
    GEN_IMPORT(unknown16, G_UNKNOWN16),
    GEN_IMPORT(unknown17, G_UNKNOWN17),
    GEN_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND),
    GEN_IMPORT(ExecuteConsoleCommand, G_EXECUTE_CONSOLE_COMMAND),
    GEN_IMPORT(unknown20, G_UNKNOWN20),
    GEN_IMPORT(unknown21, G_UNKNOWN21),
    GEN_IMPORT(unknown22, G_UNKNOWN22),
    GEN_IMPORT(Cvar_Get, G_CVAR_GET),
    GEN_IMPORT(Cvar_Register, G_CVAR_REGISTER),
    GEN_IMPORT(Cvar_Update, G_CVAR_UPDATE),
    GEN_IMPORT(Cvar_Set2, G_CVAR_SET2),
    GEN_IMPORT(unknown27, G_UNKNOWN27),
    GEN_IMPORT(Cvar_Set, G_CVAR_SET),
    GEN_IMPORT(unknown29, G_UNKNOWN29),
    GEN_IMPORT(unknown30, G_UNKNOWN30),
    GEN_IMPORT(unknown31, G_UNKNOWN31),
    GEN_IMPORT(Malloc, G_MALLOC),
    GEN_IMPORT(Free, G_FREE),
    GEN_IMPORT(unknown34, G_UNKNOWN34),
    GEN_IMPORT(CM_RegisterTerrain, G_CM_REGISTERTERRAIN),
    GEN_IMPORT(unknown36, 36),
    GEN_IMPORT(unknown37, 37),
    GEN_IMPORT(unknown38, 38),
    GEN_IMPORT(unknown39, 39),
    GEN_IMPORT(unknown40, 40),
    GEN_IMPORT(unknown41, 41),
    GEN_IMPORT(unknown42, 42),
    GEN_IMPORT(unknown43, 43),
    GEN_IMPORT(unknown44, 44),
    GEN_IMPORT(unknown45, 45),
    GEN_IMPORT(unknown46, 46),
    GEN_IMPORT(unknown47, 47),
    GEN_IMPORT(unknown48, 48),
    GEN_IMPORT(unknown49, 49),
    GEN_IMPORT(unknown50, 50),
    GEN_IMPORT(unknown51, 51),
    GEN_IMPORT(unknown52, 52),
    GEN_IMPORT(unknown53, 53),
    GEN_IMPORT(unknown54, 54),
    GEN_IMPORT(unknown55, 55),
    GEN_IMPORT(unknown56, 56),
    GEN_IMPORT(unknown57, 57),
    GEN_IMPORT(unknown58, 58),
    GEN_IMPORT(unknown59, 59),
    GEN_IMPORT(unknown60, 60),
    GEN_IMPORT(unknown61, 61),
    GEN_IMPORT(unknown62, 62),
    GEN_IMPORT(unknown63, 63),
    GEN_IMPORT(unknown64, 64),
    GEN_IMPORT(unknown65, 65),
    GEN_IMPORT(unknown66, 66),
    GEN_IMPORT(unknown67, 67),
    GEN_IMPORT(unknown68, 68),
    GEN_IMPORT(unknown69, 69),
    GEN_IMPORT(unknown70, 70),
    GEN_IMPORT(unknown71, 71),
    GEN_IMPORT(unknown72, 72),
    GEN_IMPORT(unknown73, 73),
    GEN_IMPORT(unknown74, 74),
    GEN_IMPORT(unknown75, 75),
    GEN_IMPORT(unknown76, 76),
    GEN_IMPORT(unknown77, 77),
    GEN_IMPORT(unknown78, 78),
    GEN_IMPORT(unknown79, 79),
    GEN_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE),
    GEN_IMPORT(unknown81, 81),
    GEN_IMPORT(unknown82, 82),
    GEN_IMPORT(unknown83, 83),
    GEN_IMPORT(unknown84, 84),
    GEN_IMPORT(TIKI_RegisterModel, G_TIKI_REGISTERMODEL),
    GEN_IMPORT(unknown86, 86),
    GEN_IMPORT(unknown87, 87),
    GEN_IMPORT(unknown88, 88),
    GEN_IMPORT(unknown89, 89),
    GEN_IMPORT(unknown90, 90),
    GEN_IMPORT(unknown91, 91),
    GEN_IMPORT(unknown92, 92),
    GEN_IMPORT(unknown93, 93),
    GEN_IMPORT(unknown94, 94),
    GEN_IMPORT(unknown95, 95),
    GEN_IMPORT(unknown96, 96),
    GEN_IMPORT(unknown97, 97),
    GEN_IMPORT(unknown98, 98),
    GEN_IMPORT(unknown99, 99),
    GEN_IMPORT(unknown100, 100),
    GEN_IMPORT(unknown101, 101),
    GEN_IMPORT(unknown102, 102),
    GEN_IMPORT(unknown103, 103),
    GEN_IMPORT(unknown104, 104),
    GEN_IMPORT(unknown105, 105),
    GEN_IMPORT(unknown106, 106),
    GEN_IMPORT(unknown107, 107),
    GEN_IMPORT(unknown108, 108),
    GEN_IMPORT(unknown109, 109),
    GEN_IMPORT(unknown110, 110),
    GEN_IMPORT(CM_TM_Upload, G_CM_TM_UPLOAD),
    GEN_IMPORT(SaveTerrainImageToDisk, G_SAVETERRAINIMAGETODISK),
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

    switch (cmd) {
        ROUTE_IMPORT(Printf, G_PRINTF);
        ROUTE_IMPORT(DPrintf, G_DPRINTF);
        ROUTE_IMPORT(DPrintf2, G_DPRINTF2);
        ROUTE_IMPORT(snprintf, G_SNPRINTF);
        ROUTE_IMPORT(ErrorF, G_ERRORF);
        ROUTE_IMPORT(FS_FOpenFile, G_FS_FOPEN_FILE);
        ROUTE_IMPORT(FS_Read, G_FS_READ);
        ROUTE_IMPORT(FS_FileForHandle, G_FS_FILE_FOR_HANDLE);
        ROUTE_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE);
        ROUTE_IMPORT(FS_ReadFile, G_FS_READFILE);
        ROUTE_IMPORT(FS_FreeFile, G_FS_FREEFILE);
        ROUTE_IMPORT(FS_FileAvailable, G_FS_FILEAVAILABLE);
        ROUTE_IMPORT(FS_BuildOSPath, G_FS_BUILDOSPATH);
        ROUTE_IMPORT(unknown13, G_UNKNOWN13);
        ROUTE_IMPORT(unknown14, G_UNKNOWN14);
        ROUTE_IMPORT(Milliseconds, G_MILLISECONDS);
        ROUTE_IMPORT(unknown16, G_UNKNOWN16);
        ROUTE_IMPORT(unknown17, G_UNKNOWN17);
        // handled below since we do special handling to deal with the "when" argument
        //ROUTE_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND);
        //ROUTE_IMPORT(ExecuteConsoleCommand, G_EXECUTE_CONSOLE_COMMAND);
        ROUTE_IMPORT(unknown20, G_UNKNOWN20);
        ROUTE_IMPORT(unknown21, G_UNKNOWN21);
        ROUTE_IMPORT(unknown22, G_UNKNOWN22);
        ROUTE_IMPORT(Cvar_Get, G_CVAR_GET);
        ROUTE_IMPORT(Cvar_Register, G_CVAR_REGISTER);
        ROUTE_IMPORT(Cvar_Update, G_CVAR_UPDATE);
        ROUTE_IMPORT(Cvar_Set2, G_CVAR_SET2);
        ROUTE_IMPORT(unknown27, G_UNKNOWN27);
        ROUTE_IMPORT(Cvar_Set, G_CVAR_SET);
        ROUTE_IMPORT(unknown29, G_UNKNOWN29);
        ROUTE_IMPORT(unknown30, G_UNKNOWN30);
        ROUTE_IMPORT(unknown31, G_UNKNOWN31);
        ROUTE_IMPORT(Malloc, G_MALLOC);
        ROUTE_IMPORT(Free, G_FREE);
        ROUTE_IMPORT(unknown34, G_UNKNOWN34);
        ROUTE_IMPORT(CM_RegisterTerrain, G_CM_REGISTERTERRAIN);
        ROUTE_IMPORT(unknown36, G_UNKNOWN36);
        ROUTE_IMPORT(unknown37, G_UNKNOWN37);
        ROUTE_IMPORT(unknown38, G_UNKNOWN38);
        ROUTE_IMPORT(unknown39, G_UNKNOWN39);
        ROUTE_IMPORT(unknown40, G_UNKNOWN40);
        ROUTE_IMPORT(unknown41, G_UNKNOWN41);
        ROUTE_IMPORT(unknown42, G_UNKNOWN42);
        ROUTE_IMPORT(unknown43, G_UNKNOWN43);
        ROUTE_IMPORT(unknown44, G_UNKNOWN44);
        ROUTE_IMPORT(unknown45, G_UNKNOWN45);
        ROUTE_IMPORT(unknown46, G_UNKNOWN46);
        ROUTE_IMPORT(unknown47, G_UNKNOWN47);
        ROUTE_IMPORT(unknown48, G_UNKNOWN48);
        ROUTE_IMPORT(unknown49, G_UNKNOWN49);
        ROUTE_IMPORT(unknown50, G_UNKNOWN50);
        ROUTE_IMPORT(unknown51, G_UNKNOWN51);
        ROUTE_IMPORT(unknown52, G_UNKNOWN52);
        ROUTE_IMPORT(unknown53, G_UNKNOWN53);
        ROUTE_IMPORT(unknown54, G_UNKNOWN54);
        ROUTE_IMPORT(unknown55, G_UNKNOWN55);
        ROUTE_IMPORT(unknown56, G_UNKNOWN56);
        ROUTE_IMPORT(unknown57, G_UNKNOWN57);
        ROUTE_IMPORT(unknown58, G_UNKNOWN58);
        ROUTE_IMPORT(unknown59, G_UNKNOWN59);
        ROUTE_IMPORT(unknown60, G_UNKNOWN60);
        ROUTE_IMPORT(unknown61, G_UNKNOWN61);
        ROUTE_IMPORT(unknown62, G_UNKNOWN62);
        ROUTE_IMPORT(unknown63, G_UNKNOWN63);
        ROUTE_IMPORT(unknown64, G_UNKNOWN64);
        ROUTE_IMPORT(unknown65, G_UNKNOWN65);
        ROUTE_IMPORT(unknown66, G_UNKNOWN66);
        ROUTE_IMPORT(unknown67, G_UNKNOWN67);
        ROUTE_IMPORT(unknown68, G_UNKNOWN68);
        ROUTE_IMPORT(unknown69, G_UNKNOWN69);
        ROUTE_IMPORT(unknown70, G_UNKNOWN70);
        ROUTE_IMPORT(unknown71, G_UNKNOWN71);
        ROUTE_IMPORT(unknown72, G_UNKNOWN72);
        ROUTE_IMPORT(unknown73, G_UNKNOWN73);
        ROUTE_IMPORT(unknown74, G_UNKNOWN74);
        ROUTE_IMPORT(unknown75, G_UNKNOWN75);
        ROUTE_IMPORT(unknown76, G_UNKNOWN76);
        ROUTE_IMPORT(unknown77, G_UNKNOWN77);
        ROUTE_IMPORT(unknown78, G_UNKNOWN78);
        ROUTE_IMPORT(unknown79, G_UNKNOWN79);
        ROUTE_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE);
        ROUTE_IMPORT(unknown81, G_UNKNOWN81);
        ROUTE_IMPORT(unknown82, G_UNKNOWN82);
        ROUTE_IMPORT(unknown83, G_UNKNOWN83);
        ROUTE_IMPORT(unknown84, G_UNKNOWN84);
        ROUTE_IMPORT(TIKI_RegisterModel, G_TIKI_REGISTERMODEL);
        ROUTE_IMPORT(unknown86, G_UNKNOWN86);
        ROUTE_IMPORT(unknown87, G_UNKNOWN87);
        ROUTE_IMPORT(unknown88, G_UNKNOWN88);
        ROUTE_IMPORT(unknown89, G_UNKNOWN89);
        ROUTE_IMPORT(unknown90, G_UNKNOWN90);
        ROUTE_IMPORT(unknown91, G_UNKNOWN91);
        ROUTE_IMPORT(unknown92, G_UNKNOWN92);
        ROUTE_IMPORT(unknown93, G_UNKNOWN93);
        ROUTE_IMPORT(unknown94, G_UNKNOWN94);
        ROUTE_IMPORT(unknown95, G_UNKNOWN95);
        ROUTE_IMPORT(unknown96, G_UNKNOWN96);
        ROUTE_IMPORT(unknown97, G_UNKNOWN97);
        ROUTE_IMPORT(unknown98, G_UNKNOWN98);
        ROUTE_IMPORT(unknown99, G_UNKNOWN99);
        ROUTE_IMPORT(unknown100, G_UNKNOWN100);
        ROUTE_IMPORT(unknown101, G_UNKNOWN101);
        ROUTE_IMPORT(unknown102, G_UNKNOWN102);
        ROUTE_IMPORT(unknown103, G_UNKNOWN103);
        ROUTE_IMPORT(unknown104, G_UNKNOWN104);
        ROUTE_IMPORT(unknown105, G_UNKNOWN105);
        ROUTE_IMPORT(unknown106, G_UNKNOWN106);
        ROUTE_IMPORT(unknown107, G_UNKNOWN107);
        ROUTE_IMPORT(unknown108, G_UNKNOWN108);
        ROUTE_IMPORT(unknown109, G_UNKNOWN109);
        ROUTE_IMPORT(unknown110, G_UNKNOWN110);
        ROUTE_IMPORT(CM_TM_Upload, G_CM_TM_UPLOAD);
        ROUTE_IMPORT(SaveTerrainImageToDisk, G_SAVETERRAINIMAGETODISK);

    // handle special cmds which QMM uses but SOF2SP doesn't have an analogue for
    case G_ERROR: {
        // sof2sp: void(*ErrorF)(int code, const char* fmt, ...);
        // q3a: void trap_Error(const char* fmt, ...);
        const char* fmt = (const char*)(args[1]);
        (void)orig_import.ErrorF(0, fmt, args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
        break;
    }
    case G_CVAR_VARIABLE_STRING_BUFFER: {
        // sof2sp: cvar_t *(*Cvar_Get)(const char *varName, const char *varValue, int varFlags)
        // q3a: void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
        const char* varName = (const char*)(args[0]);
        char* buffer = (char*)(args[1]);
        intptr_t bufsize = args[2];
        *buffer = '\0';
        cvar_t* cvar = orig_import.Cvar_Get(varName, "", 0);
        if (cvar)
            strncpyz(buffer, cvar->string, (size_t)bufsize);
        break;
    }
    case G_CVAR_VARIABLE_INTEGER_VALUE: {
        // sof2sp: cvar_t *(*Cvar_Get)(const char *varName, const char *varValue, int varFlags)
        // q3a: int trap_Cvar_VariableIntegerValue(const char* var_name)
        const char* varName = (const char*)(args[0]);
        cvar_t* cvar = orig_import.Cvar_Get(varName, "", 0);
        if (cvar)
            ret = cvar->integer;
        break;
    }
    case G_EXECUTE_CONSOLE_COMMAND:
    case G_SEND_CONSOLE_COMMAND: {
        // SOF2SP: void (*SendConsoleCommand)(const char *text);
        // SOF2SP: void (*ExecuteConsoleCommand)(int exec_when, const char *text);
        // qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
        // first arg may be exec_when, like EXEC_APPEND
        intptr_t when = args[0];
        const char* text = (const char*)(args[1]);
        // EXEC_APPEND is the highest flag in all known games at 2, but go with 100 to be safe
        if (when > 100) {
            text = (const char*)when;
            orig_import.SendConsoleCommand(text);
            break;
        }
        orig_import.ExecuteConsoleCommand((int)when, text);
        break;
    }

    default: {
        break;
    }

    };

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

    // pointer to wrapper vmMain function that calls actual mod func from orig_export
    g_gameinfo.pfnvmMain = SOF2SP_vmMain;

    // pointer to wrapper syscall function that calls actual engine func from orig_import
    g_gameinfo.pfnsyscall = SOF2SP_syscall;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_GetGameAPI({}, {}) returning {}\n", orig_apiversion, import, (void*)&qmm_export);

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // nothing in this struct is used by the engine before calling Init
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
        GEN_CASE(G_DPRINTF);
        GEN_CASE(G_DPRINTF2);
        GEN_CASE(G_SNPRINTF);
        GEN_CASE(G_ERRORF);
        GEN_CASE(G_FS_FOPEN_FILE);
        GEN_CASE(G_FS_READ);
        GEN_CASE(G_FS_FILE_FOR_HANDLE);
        GEN_CASE(G_FS_FCLOSE_FILE);
        GEN_CASE(G_FS_READFILE);
        GEN_CASE(G_FS_FREEFILE);
        GEN_CASE(G_FS_FILEAVAILABLE);
        GEN_CASE(G_FS_BUILDOSPATH);
        GEN_CASE(G_UNKNOWN13);
        GEN_CASE(G_UNKNOWN14);
        GEN_CASE(G_MILLISECONDS);
        GEN_CASE(G_UNKNOWN16);
        GEN_CASE(G_UNKNOWN17);
        GEN_CASE(G_SEND_CONSOLE_COMMAND);
        GEN_CASE(G_EXECUTE_CONSOLE_COMMAND);
        GEN_CASE(G_UNKNOWN20);
        GEN_CASE(G_UNKNOWN21);
        GEN_CASE(G_UNKNOWN22);
        GEN_CASE(G_CVAR_GET);
        GEN_CASE(G_CVAR_REGISTER);
        GEN_CASE(G_CVAR_UPDATE);
        GEN_CASE(G_CVAR_SET2);
        GEN_CASE(G_UNKNOWN27);
        GEN_CASE(G_CVAR_SET);
        GEN_CASE(G_UNKNOWN29);
        GEN_CASE(G_UNKNOWN30);
        GEN_CASE(G_UNKNOWN31);
        GEN_CASE(G_MALLOC);
        GEN_CASE(G_FREE);
        GEN_CASE(G_UNKNOWN34);
        GEN_CASE(G_CM_REGISTERTERRAIN);
        GEN_CASE(G_UNKNOWN36);
        GEN_CASE(G_UNKNOWN37);
        GEN_CASE(G_UNKNOWN38);
        GEN_CASE(G_UNKNOWN39);
        GEN_CASE(G_UNKNOWN40);
        GEN_CASE(G_UNKNOWN41);
        GEN_CASE(G_UNKNOWN42);
        GEN_CASE(G_UNKNOWN43);
        GEN_CASE(G_UNKNOWN44);
        GEN_CASE(G_UNKNOWN45);
        GEN_CASE(G_UNKNOWN46);
        GEN_CASE(G_UNKNOWN47);
        GEN_CASE(G_UNKNOWN48);
        GEN_CASE(G_UNKNOWN49);
        GEN_CASE(G_UNKNOWN50);
        GEN_CASE(G_UNKNOWN51);
        GEN_CASE(G_UNKNOWN52);
        GEN_CASE(G_UNKNOWN53);
        GEN_CASE(G_UNKNOWN54);
        GEN_CASE(G_UNKNOWN55);
        GEN_CASE(G_UNKNOWN56);
        GEN_CASE(G_UNKNOWN57);
        GEN_CASE(G_UNKNOWN58);
        GEN_CASE(G_UNKNOWN59);
        GEN_CASE(G_UNKNOWN60);
        GEN_CASE(G_UNKNOWN61);
        GEN_CASE(G_UNKNOWN62);
        GEN_CASE(G_UNKNOWN63);
        GEN_CASE(G_UNKNOWN64);
        GEN_CASE(G_UNKNOWN65);
        GEN_CASE(G_UNKNOWN66);
        GEN_CASE(G_UNKNOWN67);
        GEN_CASE(G_UNKNOWN68);
        GEN_CASE(G_UNKNOWN69);
        GEN_CASE(G_UNKNOWN70);
        GEN_CASE(G_UNKNOWN71);
        GEN_CASE(G_UNKNOWN72);
        GEN_CASE(G_UNKNOWN73);
        GEN_CASE(G_UNKNOWN74);
        GEN_CASE(G_UNKNOWN75);
        GEN_CASE(G_UNKNOWN76);
        GEN_CASE(G_UNKNOWN77);
        GEN_CASE(G_UNKNOWN78);
        GEN_CASE(G_UNKNOWN79);
        GEN_CASE(G_ADJUSTAREAPORTALSTATE);
        GEN_CASE(G_UNKNOWN81);
        GEN_CASE(G_UNKNOWN82);
        GEN_CASE(G_UNKNOWN83);
        GEN_CASE(G_UNKNOWN84);
        GEN_CASE(G_TIKI_REGISTERMODEL);
        GEN_CASE(G_UNKNOWN86);
        GEN_CASE(G_UNKNOWN87);
        GEN_CASE(G_UNKNOWN88);
        GEN_CASE(G_UNKNOWN89);
        GEN_CASE(G_UNKNOWN90);
        GEN_CASE(G_UNKNOWN91);
        GEN_CASE(G_UNKNOWN92);
        GEN_CASE(G_UNKNOWN93);
        GEN_CASE(G_UNKNOWN94);
        GEN_CASE(G_UNKNOWN95);
        GEN_CASE(G_UNKNOWN96);
        GEN_CASE(G_UNKNOWN97);
        GEN_CASE(G_UNKNOWN98);
        GEN_CASE(G_UNKNOWN99);
        GEN_CASE(G_UNKNOWN100);
        GEN_CASE(G_UNKNOWN101);
        GEN_CASE(G_UNKNOWN102);
        GEN_CASE(G_UNKNOWN103);
        GEN_CASE(G_UNKNOWN104);
        GEN_CASE(G_UNKNOWN105);
        GEN_CASE(G_UNKNOWN106);
        GEN_CASE(G_UNKNOWN107);
        GEN_CASE(G_UNKNOWN108);
        GEN_CASE(G_UNKNOWN109);
        GEN_CASE(G_UNKNOWN110);
        GEN_CASE(G_CM_TM_UPLOAD);
        GEN_CASE(G_SAVETERRAINIMAGETODISK);

        // polyfills
        GEN_CASE(G_ERROR);
        GEN_CASE(G_ARGV);
        GEN_CASE(G_ARGC);
        GEN_CASE(G_ARGS);
        GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
        GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(G_FS_WRITE);
        GEN_CASE(G_GET_CONFIGSTRING);

        GEN_CASE(G_LOCATE_GAME_DATA);
        GEN_CASE(G_GET_ENTITY_TOKEN);

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
