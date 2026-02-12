/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <sof2sp/g_public.h>

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
    GEN_IMPORT(FS_Write, G_FS_WRITE),
    GEN_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE),
    GEN_IMPORT(FS_ReadFile, G_FS_READFILE),
    GEN_IMPORT(FS_FreeFile, G_FS_FREEFILE),
    GEN_IMPORT(FS_FileAvailable, G_FS_FILEAVAILABLE),
    GEN_IMPORT(FS_ListFiles, G_FS_LISTFILES),
    GEN_IMPORT(FS_FreeFileList, G_FS_FREEFILELIST),
    GEN_IMPORT(unknown14, G_UNKNOWN14),
    GEN_IMPORT(Milliseconds, G_MILLISECONDS),
    GEN_IMPORT(unknown16, G_UNKNOWN16),
    GEN_IMPORT(unknown17, G_UNKNOWN17),
    GEN_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND),
    GEN_IMPORT(ExecuteConsoleCommand, G_EXECUTE_CONSOLE_COMMAND),
    GEN_IMPORT(Argc, G_ARGC),
    GEN_IMPORT(Argv, G_ARGV),
    GEN_IMPORT(Args, G_ARGS),
    GEN_IMPORT(Cvar_IsModified, G_CVAR_ISMODIFIED),
    GEN_IMPORT(Cvar_Register, G_CVAR_REGISTER),
    GEN_IMPORT(Cvar_Update, G_CVAR_UPDATE),
    GEN_IMPORT(Cvar_Set, G_CVAR_SET),
    GEN_IMPORT(Cvar_Get, G_CVAR_GET),
    GEN_IMPORT(Cvar_SetValue, G_CVAR_SETVALUE),
    GEN_IMPORT(Cvar_VariableIntegerValue, G_CVAR_VARIABLE_INTEGER_VALUE),
    GEN_IMPORT(Cvar_VariableFloatValue, G_CVAR_VARIABLE_FLOAT_VALUE),
    GEN_IMPORT(Cvar_VariableStringBuffer, G_CVAR_VARIABLE_STRING_BUFFER),
    GEN_IMPORT(Malloc, G_MALLOC),
    GEN_IMPORT(Free, G_FREE),
    GEN_IMPORT(unknown34, G_UNKNOWN34),
    GEN_IMPORT(CM_RegisterTerrain, G_CM_REGISTERTERRAIN),
    GEN_IMPORT(unknown36, G_UNKNOWN36),
    GEN_IMPORT(unknown37, G_UNKNOWN37),
    GEN_IMPORT(unknown38, G_UNKNOWN38),
    GEN_IMPORT(unknown39, G_UNKNOWN39),
    GEN_IMPORT(unknown40, G_UNKNOWN40),
    GEN_IMPORT(unknown41, G_UNKNOWN41),
    GEN_IMPORT(unknown42, G_UNKNOWN42),
    GEN_IMPORT(unknown43, G_UNKNOWN43),
    GEN_IMPORT(unknown44, G_UNKNOWN44),
    GEN_IMPORT(unknown45, G_UNKNOWN45),
    GEN_IMPORT(unknown46, G_UNKNOWN46),
    GEN_IMPORT(unknown47, G_UNKNOWN47),
    GEN_IMPORT(unknown48, G_UNKNOWN48),
    GEN_IMPORT(unknown49, G_UNKNOWN49),
    GEN_IMPORT(unknown50, G_UNKNOWN50),
    GEN_IMPORT(Trace, G_TRACE),
    GEN_IMPORT(unknown52, G_UNKNOWN52),
    GEN_IMPORT(unknown53, G_UNKNOWN53),
    GEN_IMPORT(LocateGameData, G_LOCATE_GAME_DATA),
    GEN_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND),
    GEN_IMPORT(unknown56, G_UNKNOWN56),
    GEN_IMPORT(unknown57, G_UNKNOWN57),
    GEN_IMPORT(unknown58, G_UNKNOWN58),
    GEN_IMPORT(unknown59, G_UNKNOWN59),
    GEN_IMPORT(unknown60, G_UNKNOWN60),
    GEN_IMPORT(unknown61, G_UNKNOWN61),
    GEN_IMPORT(unknown62, G_UNKNOWN62),
    GEN_IMPORT(unknown63, G_UNKNOWN63),
    GEN_IMPORT(unknown64, G_UNKNOWN64),
    GEN_IMPORT(unknown65, G_UNKNOWN65),
    GEN_IMPORT(unknown66, G_UNKNOWN66),
    0, // unknown67
    0, // unknown68
    GEN_IMPORT(unknown69, G_UNKNOWN69),
    GEN_IMPORT(unknown70, G_UNKNOWN70),
    GEN_IMPORT(PointContents, G_POINT_CONTENTS),
    GEN_IMPORT(unknown72, G_UNKNOWN72),
    GEN_IMPORT(SetBrushModel, G_SET_BRUSH_MODEL),
    GEN_IMPORT(SetActiveSubBSP, G_SET_ACTIVE_SUBBSP),
    GEN_IMPORT(unknown75, G_UNKNOWN75),
    GEN_IMPORT(unknown76, G_UNKNOWN76),
    GEN_IMPORT(SetConfigstring, G_SET_CONFIGSTRING),
    GEN_IMPORT(GetConfigstring, G_GET_CONFIGSTRING),
    GEN_IMPORT(GetServerInfo, G_GET_SERVERINFO),
    GEN_IMPORT(AdjustAreaPortalState, G_ADJUSTAREAPORTALSTATE),
    GEN_IMPORT(unknown81, G_UNKNOWN81),
    GEN_IMPORT(unknown82, G_UNKNOWN82),
    GEN_IMPORT(unknown83, G_UNKNOWN83),
    GEN_IMPORT(unknown84, G_UNKNOWN84),
    GEN_IMPORT(TIKI_RegisterModel, G_TIKI_REGISTERMODEL),
    GEN_IMPORT(unknown86, G_UNKNOWN86),
    GEN_IMPORT(unknown87, G_UNKNOWN87),
    GEN_IMPORT(unknown88, G_UNKNOWN88),
    GEN_IMPORT(unknown89, G_UNKNOWN89),
    GEN_IMPORT(unknown90, G_UNKNOWN90),
    GEN_IMPORT(unknown91, G_UNKNOWN91),
    GEN_IMPORT(unknown92, G_UNKNOWN92),
    GEN_IMPORT(unknown93, G_UNKNOWN93),
    GEN_IMPORT(unknown94, G_UNKNOWN94),
    GEN_IMPORT(unknown95, G_UNKNOWN95),
    GEN_IMPORT(GetEntityToken, G_GET_ENTITY_TOKEN),
    GEN_IMPORT(unknown97, G_UNKNOWN97),
    GEN_IMPORT(unknown98, G_UNKNOWN98),
    GEN_IMPORT(unknown99, G_UNKNOWN99),
    GEN_IMPORT(unknown100, G_UNKNOWN100),
    GEN_IMPORT(unknown101, G_UNKNOWN101),
    GEN_IMPORT(unknown102, G_UNKNOWN102),
    GEN_IMPORT(unknown103, G_UNKNOWN103),
    GEN_IMPORT(unknown104, G_UNKNOWN104),
    GEN_IMPORT(unknown105, G_UNKNOWN105),
    GEN_IMPORT(unknown106, G_UNKNOWN106),
    GEN_IMPORT(unknown107, G_UNKNOWN107),
    GEN_IMPORT(unknown108, G_UNKNOWN108),
    GEN_IMPORT(unknown109, G_UNKNOWN109),
    GEN_IMPORT(unknown110, G_UNKNOWN110),
    GEN_IMPORT(CM_TM_Upload, G_CM_TM_UPLOAD),
    GEN_IMPORT(SaveTerrainImageToDisk, G_SAVETERRAINIMAGETODISK),
};


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
    GEN_EXPORT(IsClientActive, GAME_IS_CLIENT_ACTIVE),
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
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_syscall({} {}) called\n", SOF2SP_eng_msg_names(cmd), cmd);
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
        ROUTE_IMPORT(FS_Write, G_FS_WRITE);
        ROUTE_IMPORT(FS_FCloseFile, G_FS_FCLOSE_FILE);
        ROUTE_IMPORT(FS_ReadFile, G_FS_READFILE);
        ROUTE_IMPORT(FS_FreeFile, G_FS_FREEFILE);
        ROUTE_IMPORT(FS_FileAvailable, G_FS_FILEAVAILABLE);
        ROUTE_IMPORT(FS_ListFiles, G_FS_LISTFILES);
        ROUTE_IMPORT(FS_FreeFileList, G_FS_FREEFILELIST);
        ROUTE_IMPORT(unknown14, G_UNKNOWN14);
        ROUTE_IMPORT(Milliseconds, G_MILLISECONDS);
        ROUTE_IMPORT(unknown16, G_UNKNOWN16);
        ROUTE_IMPORT(unknown17, G_UNKNOWN17);
        // handled below since we do special handling to deal with the "when" argument
        //ROUTE_IMPORT(SendConsoleCommand, G_SEND_CONSOLE_COMMAND);
        //ROUTE_IMPORT(ExecuteConsoleCommand, G_EXECUTE_CONSOLE_COMMAND);
        ROUTE_IMPORT(Argc, G_ARGC);
        ROUTE_IMPORT(Argv, G_ARGV);
        ROUTE_IMPORT(Args, G_ARGS);
        ROUTE_IMPORT(Cvar_IsModified, G_CVAR_ISMODIFIED);
        ROUTE_IMPORT(Cvar_Register, G_CVAR_REGISTER);
        ROUTE_IMPORT(Cvar_Update, G_CVAR_UPDATE);
        ROUTE_IMPORT(Cvar_Set, G_CVAR_SET);
        ROUTE_IMPORT(Cvar_Get, G_CVAR_GET);
        ROUTE_IMPORT(Cvar_SetValue, G_CVAR_SETVALUE);
        ROUTE_IMPORT(Cvar_VariableIntegerValue, G_CVAR_VARIABLE_INTEGER_VALUE);
        ROUTE_IMPORT(Cvar_VariableFloatValue, G_CVAR_VARIABLE_FLOAT_VALUE);
        ROUTE_IMPORT(Cvar_VariableStringBuffer, G_CVAR_VARIABLE_STRING_BUFFER);
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
        ROUTE_IMPORT(Trace, G_TRACE);
        ROUTE_IMPORT(unknown52, G_UNKNOWN52);
        ROUTE_IMPORT(unknown53, G_UNKNOWN53);
        ROUTE_IMPORT(LocateGameData, G_LOCATE_GAME_DATA);
        ROUTE_IMPORT(SendServerCommand, G_SEND_SERVER_COMMAND);
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
        ROUTE_IMPORT(unknown69, G_UNKNOWN69);
        ROUTE_IMPORT(unknown70, G_UNKNOWN70);
        ROUTE_IMPORT(PointContents, G_POINT_CONTENTS);
        ROUTE_IMPORT(unknown72, G_UNKNOWN72);
        ROUTE_IMPORT(SetBrushModel, G_SET_BRUSH_MODEL);
        ROUTE_IMPORT(SetActiveSubBSP, G_SET_ACTIVE_SUBBSP);
        ROUTE_IMPORT(unknown75, G_UNKNOWN75);
        ROUTE_IMPORT(unknown76, G_UNKNOWN76);
        ROUTE_IMPORT(SetConfigstring, G_SET_CONFIGSTRING);
        ROUTE_IMPORT(GetConfigstring, G_GET_CONFIGSTRING);
        ROUTE_IMPORT(GetServerInfo, G_GET_SERVERINFO);
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
        ROUTE_IMPORT(GetEntityToken, G_GET_ENTITY_TOKEN);
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

        // handle cmds for variables, this is how a plugin would get these values if needed
        ROUTE_IMPORT_VAR(unknown67, GV_UNKNOWN67);
        ROUTE_IMPORT_VAR(unknown68, GV_UNKNOWN68);

    // handle special cmds which QMM uses but SOF2SP doesn't have an analogue for
    case G_ERROR: {
        // sof2sp: void(*ErrorF)(int code, const char* fmt, ...);
        // q3a: void trap_Error(const char* fmt);
        const char* fmt = (const char*)(args[1]);
        (void)orig_import.ErrorF(0, fmt);
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
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_syscall({} {}) returning {}\n", SOF2SP_eng_msg_names(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t SOF2SP_vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_vmMain({} {}) called\n", SOF2SP_mod_msg_names(cmd), cmd);
#endif

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
        ROUTE_EXPORT(IsClientActive, GAME_IS_CLIENT_ACTIVE);
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

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("SOF2SP_vmMain({} {}) returning {}\n", SOF2SP_mod_msg_names(cmd), cmd, ret);
#endif

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
        GEN_CASE(G_FS_WRITE);
        GEN_CASE(G_FS_FCLOSE_FILE);
        GEN_CASE(G_FS_READFILE);
        GEN_CASE(G_FS_FREEFILE);
        GEN_CASE(G_FS_FILEAVAILABLE);
        GEN_CASE(G_FS_LISTFILES);
        GEN_CASE(G_FS_FREEFILELIST);
        GEN_CASE(G_UNKNOWN14);
        GEN_CASE(G_MILLISECONDS);
        GEN_CASE(G_UNKNOWN16);
        GEN_CASE(G_UNKNOWN17);
        GEN_CASE(G_SEND_CONSOLE_COMMAND);
        GEN_CASE(G_EXECUTE_CONSOLE_COMMAND);
        GEN_CASE(G_ARGC);
        GEN_CASE(G_ARGV);
        GEN_CASE(G_ARGS);
        GEN_CASE(G_CVAR_ISMODIFIED);
        GEN_CASE(G_CVAR_REGISTER);
        GEN_CASE(G_CVAR_UPDATE);
        GEN_CASE(G_CVAR_SET);
        GEN_CASE(G_CVAR_GET);
        GEN_CASE(G_CVAR_SETVALUE);
        GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(G_CVAR_VARIABLE_FLOAT_VALUE);
        GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
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
        GEN_CASE(G_TRACE);
        GEN_CASE(G_UNKNOWN52);
        GEN_CASE(G_UNKNOWN53);
        GEN_CASE(G_LOCATE_GAME_DATA);
        GEN_CASE(G_SEND_SERVER_COMMAND);
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
        GEN_CASE(GV_UNKNOWN67);
        GEN_CASE(GV_UNKNOWN68);
        GEN_CASE(G_UNKNOWN69);
        GEN_CASE(G_UNKNOWN70);
        GEN_CASE(G_POINT_CONTENTS);
        GEN_CASE(G_UNKNOWN72);
        GEN_CASE(G_SET_BRUSH_MODEL);
        GEN_CASE(G_SET_ACTIVE_SUBBSP);
        GEN_CASE(G_UNKNOWN75);
        GEN_CASE(G_UNKNOWN76);
        GEN_CASE(G_SET_CONFIGSTRING);
        GEN_CASE(G_GET_CONFIGSTRING);
        GEN_CASE(G_GET_SERVERINFO);
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
        GEN_CASE(G_GET_ENTITY_TOKEN);
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
        GEN_CASE(GAME_IS_CLIENT_ACTIVE);
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
