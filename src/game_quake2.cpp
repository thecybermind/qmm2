/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <quake2/game/q_shared.h>
#include <quake2/game/game.h>
#include "game_api.hpp"
#include "log.hpp"
#include "format.hpp"
// QMM-specific QUAKE2 header
#include "game_quake2.h"
#include "gameinfo.hpp"
#include "main.hpp"
#include "util.hpp"

struct QUAKE2_GameSupport : public GameSupport {
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
    virtual const char* DefaultModDir() { return "baseq2"; }
    virtual const char* ModCvar() { return "game"; }
    virtual const char* GameName() { return "Quake 2"; }
    virtual const char* GameCode() { return "QUAKE2"; }

private:
    // update the export variables from orig_export
    static void update_exports();

    // track configstrings for our G_GET_CONFIGSTRING syscall
    static std::map<int, std::string> configstrings;
    static void configstring(int num, char* configstring);

    // track userinfo for our G_GET_USERINFO syscall
    static std::map<intptr_t, std::string> userinfos;
    static qboolean ClientConnect(edict_t* ent, char* userinfo);
    static void ClientUserinfoChanged(edict_t* ent, char* userinfo);

    // track entstrings for our G_GET_ENTITY_TOKEN syscall
    static std::vector<std::string> entity_tokens;
    static size_t token_counter;
    static void SpawnEntities(char* mapname, char* entstring, char* spawnpoint);

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

GEN_GAME_OBJ(QUAKE2);


// auto-detection logic for QUAKE2
bool QUAKE2_GameSupport::AutoDetect(APIType engineapi) {
    if (engineapi != QMM_API_GETGAMEAPI)
        return false;

    if (!str_striequal(gameinfo.qmm_file, DefaultDLLName()))
        return false;

    if (!str_stristr(gameinfo.exe_file, "quake2") && !str_stristr(gameinfo.exe_file, "q2ded"))
        return false;

    return true;
}


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t QUAKE2_GameSupport::syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_GameSupport::syscall({} {}) called\n", EngMsgName(cmd), cmd);
#endif

    // update export vars before calling into the engine
    update_exports();

    intptr_t ret = 0; 

    switch (cmd) {
        ROUTE_IMPORT(bprintf, G_BPRINTF);
        ROUTE_IMPORT(dprintf, G_DPRINTF);
        ROUTE_IMPORT(cprintf, G_CPRINTF);
        ROUTE_IMPORT(centerprintf, G_CENTERPRINTF);
        ROUTE_IMPORT_6_V(sound, G_SOUND, (edict_t*), (int), (int), *(float*)&, *(float*)&, *(float*)&);
        ROUTE_IMPORT_7_V(positioned_sound, G_POSITIONED_SOUND, (float*), (edict_t*), (int), (int), *(float*)&, *(float*)&, *(float*)&);
        ROUTE_IMPORT(configstring, G_CONFIGSTRING);
        ROUTE_IMPORT(error, G_ERROR);
        ROUTE_IMPORT(modelindex, G_MODELINDEX);
        ROUTE_IMPORT(soundindex, G_SOUNDINDEX);
        ROUTE_IMPORT(imageindex, G_IMAGEINDEX);
        ROUTE_IMPORT(setmodel, G_SETMODEL);
        ROUTE_IMPORT(trace, G_TRACE);
        ROUTE_IMPORT(pointcontents, G_POINT_CONTENTS);
        ROUTE_IMPORT(inPVS, G_IN_PVS);
        ROUTE_IMPORT(inPHS, G_IN_PHS);
        ROUTE_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE);
        ROUTE_IMPORT(AreasConnected, G_AREAS_CONNECTED);
        ROUTE_IMPORT(linkentity, G_LINKENTITY);
        ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
        ROUTE_IMPORT(BoxEdicts, G_BOXEDICTS);
        ROUTE_IMPORT(Pmove, G_PMOVE);
        ROUTE_IMPORT(multicast, G_MULTICAST);
        ROUTE_IMPORT(unicast, G_UNICAST);
        ROUTE_IMPORT(WriteChar, G_MSG_WRITECHAR);
        ROUTE_IMPORT(WriteByte, G_MSG_WRITEBYTE);
        ROUTE_IMPORT(WriteShort, G_MSG_WRITESHORT);
        ROUTE_IMPORT(WriteLong, G_MSG_WRITELONG);
        ROUTE_IMPORT_1_V(WriteFloat, G_MSG_WRITEFLOAT, *(float*)&);
        ROUTE_IMPORT(WriteString, G_MSG_WRITESTRING);
        ROUTE_IMPORT(WritePosition, G_MSG_WRITEPOSITION);
        ROUTE_IMPORT(WriteDir, G_MSG_WRITEDIR);
        ROUTE_IMPORT_1_V(WriteAngle, G_MSG_WRITEANGLE, *(float*)&);
        ROUTE_IMPORT(TagMalloc, G_TAGMALLOC);
        ROUTE_IMPORT(TagFree, G_TAGFREE);
        ROUTE_IMPORT(FreeTags, G_FREETAGS);
        ROUTE_IMPORT(cvar, G_CVAR);
        ROUTE_IMPORT(cvar_set, G_CVAR_SET);
        ROUTE_IMPORT(cvar_forceset, G_CVAR_FORCESET);
        ROUTE_IMPORT(argc, G_ARGC);
        ROUTE_IMPORT(argv, G_ARGV);
        ROUTE_IMPORT(args, G_ARGS);
        ROUTE_IMPORT(AddCommandString, G_ADDCOMMANDSTRING);
        ROUTE_IMPORT_2_V(DebugGraph, G_DEBUGGRAPH, *(float*)&, (int));

        // handle cmds for variables, this is how a plugin would get these values if needed

    // handle special cmds which QMM uses but QUAKE2 doesn't have an analogue for
    case G_CVAR_REGISTER: {
        // quake2: cvar_t *(*cvar) (char *var_name, char *value, int flags);
        // q3a: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
        // qmm always passes NULL for vmCvar so don't worry about it
        char* var_name = (char*)(args[1]);
        char* value = (char*)(args[2]);
        int flags = (int)args[3];
        (void)orig_import.cvar(var_name, value, flags);
        break;
    }
    case G_CVAR_VARIABLE_STRING_BUFFER: {
        // quake2: cvar_t *(*cvar) (char *var_name, char *value, int flags);
        // q3a: void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
        char* var_name = (char*)(args[0]);
        char* buffer = (char*)(args[1]);
        intptr_t bufsize = args[2];
        *buffer = '\0';
        cvar_t* cvar = orig_import.cvar(var_name, (char*)"", 0);
        if (cvar)
            strncpyz(buffer, cvar->string, (size_t)bufsize);
        break;
    }
    case G_CVAR_VARIABLE_INTEGER_VALUE: {
        // quake2: cvar_t *(*cvar) (char *var_name, char *value, int flags);
        // q3a: int trap_Cvar_VariableIntegerValue(const char* var_name)
        char* var_name = (char*)(args[0]);
        cvar_t* cvar = orig_import.cvar(var_name, (char*)"", 0);
        if (cvar)
            ret = (int)cvar->value;
        break;
    }
    case G_SEND_CONSOLE_COMMAND: {
        // quake2: void (*AddCommandString)(char *text);
        // qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
        char* text = (char*)(args[1]);
        orig_import.AddCommandString(text);
        break;
    }
    case G_PRINT: {
        // quake2: void	(*bprintf) (int printlevel, char *fmt, ...);
        // qmm: void trap_Printf( const char *fmt );
        char* text = (char*)args[0];
        orig_import.bprintf(PRINT_HIGH, text);
        break;
    }
    case G_FS_FOPEN_FILE: {
        // provide these G_FS_ functions to plugins just so the most basic file functions all work. use FILE* for these
        // int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
        const char* qpath = (const char*)args[0];
        fileHandle_t* f = (fileHandle_t*)args[1];
        intptr_t mode = args[2];

        const char* str_mode = "rb";
        if (mode == FS_WRITE)
            str_mode = "wb";
        else if (mode == FS_APPEND)
            str_mode = "ab";
        std::string path = fmt::format("{}/{}", gameinfo.qmm_dir, qpath);
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
    case G_FS_READ: {
        // void trap_FS_Read(void* buffer, int len, fileHandle_t f);
        char* buffer = (char*)args[0];
        size_t len = (size_t)args[1];
        fileHandle_t f = (fileHandle_t)args[2];
        size_t total = 0;
        FILE* fp = (FILE*)f;
        for (int i = 0; i < 50; i++) {	// prevent infinite loops trying to read
            total += fread(buffer + total, 1, len - total, fp);
            if (total >= len || ferror(fp) || feof(fp))
                break;
        }
        break;
    }
    case G_FS_WRITE: {
        // void trap_FS_Write(const void* buffer, int len, fileHandle_t f);
        char* buffer = (char*)args[0];
        size_t len = (size_t)args[1];
        fileHandle_t f = (fileHandle_t)args[2];
        size_t total = 0;
        FILE* fp = (FILE*)f;
        for (int i = 0; i < 50; i++) {	// prevent infinite loops trying to write
            total += fwrite(buffer + total, 1, len - total, fp);
            if (total >= len || ferror(fp))
                break;
        }
        break;
    }
    case G_FS_FCLOSE_FILE: {
        // void trap_FS_FCloseFile(fileHandle_t f);
        fileHandle_t f = (fileHandle_t)args[0];
        FILE* fp = (FILE*)f;
        fclose(fp);
        break;
    }
    case G_LOCATE_GAME_DATA: {
        // help plugins not need separate logic for entity/client pointers
        // void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient);
        // this is just to be hooked by plugins, so ignore everything
        break;
    }
    case G_DROP_CLIENT: {
        // void trap_DropClient(int clientNum, const char *reason);
        intptr_t clientnum = args[0];
        orig_import.AddCommandString((char*)fmt::format("kick {}\n", clientnum).c_str());
        break;
    }
    case G_GET_USERINFO: {
        // void trap_GetUserinfo(int num, char *buffer, int bufferSize);
        intptr_t num = args[0];
        char* buffer = (char*)args[1];
        intptr_t bufferSize = args[2];
        *buffer = '\0';
        if (userinfos.count(num))
            strncpyz(buffer, userinfos[num].c_str(), (size_t)bufferSize);
        break;
    }
    case G_GET_ENTITY_TOKEN: {
        // bool trap_GetEntityToken(char *buffer, int bufferSize);
        if (token_counter >= entity_tokens.size()) {
            ret = false;
            break;
        }

        char* buffer = (char*)args[0];
        intptr_t bufferSize = args[1];

        strncpyz(buffer, entity_tokens[token_counter++].c_str(), (size_t)bufferSize);
        ret = true;
        break;
    }
    case G_GET_CONFIGSTRING: {
        // const char* (*get_configstring)(int num);
        intptr_t num = args[0];

        if (configstrings.count(num))
            ret = (intptr_t)configstrings[num].c_str();

        break;
    }
    case G_MILLISECONDS: {
        ret = util_get_milliseconds();
        break;
    }
    default:
        break;
    };

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_GameSupport::syscall({} {}) returning {}\n", EngMsgName(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t QUAKE2_GameSupport::vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_GameSupport::vmMain({} {}) called\n", ModMsgName(cmd), cmd);
#endif

    if (!orig_export)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    switch (cmd) {
        ROUTE_EXPORT(Init, GAME_INIT);
        ROUTE_EXPORT(Shutdown, GAME_SHUTDOWN);
        ROUTE_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES);
        ROUTE_EXPORT(WriteGame, GAME_WRITE_GAME);
        ROUTE_EXPORT(ReadGame, GAME_READ_GAME);
        ROUTE_EXPORT(WriteLevel, GAME_WRITE_LEVEL);
        ROUTE_EXPORT(ReadLevel, GAME_READ_LEVEL);
        ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
        ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
        ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
        ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
        ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
        ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
        ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
        ROUTE_EXPORT(ServerCommand, GAME_SERVER_COMMAND);

        // handle cmds for variables, this is how a plugin would get these values if needed
        ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
        ROUTE_EXPORT_VAR(edicts, GAMEVP_EDICTS);
        ROUTE_EXPORT_VAR(edict_size, GAMEV_EDICT_SIZE);
        ROUTE_EXPORT_VAR(num_edicts, GAMEV_NUM_EDICTS);
        ROUTE_EXPORT_VAR(max_edicts, GAMEV_MAX_EDICTS);

    default:
        break;
    };

    // update export vars after returning from the mod
    update_exports();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_GameSupport::vmMain({} {}) returning {}\n", ModMsgName(cmd), cmd, ret);
#endif

    return ret;
}


void* QUAKE2_GameSupport::Entry(void* import, void*, APIType) {
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_GameSupport::Entry({}) called\n", import);

    // original import struct from engine
    // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
    game_import_t* gi = (game_import_t*)import;
    orig_import = *gi;

    // fill in variables of our hooked import struct to pass to the mod
    // qmm_import.x = orig_import.x;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("QUAKE2_GameSupport::Entry({}) returning {}\n", import, fmt::ptr(&qmm_export));

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // the only thing in this struct the engine uses before calling Init is the apiversion
    return &qmm_export;
}


bool QUAKE2_GameSupport::ModLoad(void* entry, APIType modapi) {
    if (modapi != QMM_API_GETGAMEAPI)
        return false;

    mod_GetGameAPI pfnGGA = (mod_GetGameAPI)entry;
    orig_export = (game_export_t*)pfnGGA(&qmm_import, nullptr);

    return !!orig_export;
}


void QUAKE2_GameSupport::ModUnload() {
    orig_export = nullptr;
}


const char* QUAKE2_GameSupport::EngMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_BPRINTF);
        GEN_CASE(G_DPRINTF);
        GEN_CASE(G_CPRINTF);
        GEN_CASE(G_CENTERPRINTF);
        GEN_CASE(G_SOUND);
        GEN_CASE(G_POSITIONED_SOUND);
        GEN_CASE(G_CONFIGSTRING);
        GEN_CASE(G_ERROR);
        GEN_CASE(G_MODELINDEX);
        GEN_CASE(G_SOUNDINDEX);
        GEN_CASE(G_IMAGEINDEX);
        GEN_CASE(G_SETMODEL);
        GEN_CASE(G_TRACE);
        GEN_CASE(G_POINT_CONTENTS);
        GEN_CASE(G_IN_PVS);
        GEN_CASE(G_IN_PHS);
        GEN_CASE(G_SETAREAPORTALSTATE);
        GEN_CASE(G_AREAS_CONNECTED);
        GEN_CASE(G_LINKENTITY);
        GEN_CASE(G_UNLINKENTITY);
        GEN_CASE(G_BOXEDICTS);
        GEN_CASE(G_PMOVE);
        GEN_CASE(G_MULTICAST);
        GEN_CASE(G_UNICAST);
        GEN_CASE(G_MSG_WRITECHAR);
        GEN_CASE(G_MSG_WRITEBYTE);
        GEN_CASE(G_MSG_WRITESHORT);
        GEN_CASE(G_MSG_WRITELONG);
        GEN_CASE(G_MSG_WRITEFLOAT);
        GEN_CASE(G_MSG_WRITESTRING);
        GEN_CASE(G_MSG_WRITEPOSITION);
        GEN_CASE(G_MSG_WRITEDIR);
        GEN_CASE(G_MSG_WRITEANGLE);
        GEN_CASE(G_TAGMALLOC);
        GEN_CASE(G_TAGFREE);
        GEN_CASE(G_FREETAGS);
        GEN_CASE(G_CVAR);
        GEN_CASE(G_CVAR_SET);
        GEN_CASE(G_CVAR_FORCESET);
        GEN_CASE(G_ARGC);
        GEN_CASE(G_ARGV);
        GEN_CASE(G_ARGS);
        GEN_CASE(G_ADDCOMMANDSTRING);
        GEN_CASE(G_DEBUGGRAPH);

        // polyfills
        GEN_CASE(G_CVAR_REGISTER);
        GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
        GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(G_SEND_CONSOLE_COMMAND);
        GEN_CASE(G_PRINT);

        GEN_CASE(G_FS_FOPEN_FILE);
        GEN_CASE(G_FS_READ);
        GEN_CASE(G_FS_WRITE);
        GEN_CASE(G_FS_FCLOSE_FILE);

        GEN_CASE(G_LOCATE_GAME_DATA);
        GEN_CASE(G_DROP_CLIENT);
        GEN_CASE(G_GET_USERINFO);
        GEN_CASE(G_GET_ENTITY_TOKEN);
        GEN_CASE(G_GET_CONFIGSTRING);

    default:
        return "unknown";
    }
}


const char* QUAKE2_GameSupport::ModMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(GAMEV_APIVERSION);
        GEN_CASE(GAME_INIT);
        GEN_CASE(GAME_SHUTDOWN);
        GEN_CASE(GAME_SPAWN_ENTITIES);
        GEN_CASE(GAME_WRITE_GAME);
        GEN_CASE(GAME_READ_GAME);
        GEN_CASE(GAME_WRITE_LEVEL);
        GEN_CASE(GAME_READ_LEVEL);
        GEN_CASE(GAME_CLIENT_CONNECT);
        GEN_CASE(GAME_CLIENT_BEGIN);
        GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
        GEN_CASE(GAME_CLIENT_DISCONNECT);
        GEN_CASE(GAME_CLIENT_COMMAND);
        GEN_CASE(GAME_CLIENT_THINK);
        GEN_CASE(GAME_RUN_FRAME);
        GEN_CASE(GAME_SERVER_COMMAND);
        GEN_CASE(GAMEVP_EDICTS);
        GEN_CASE(GAMEV_EDICT_SIZE);
        GEN_CASE(GAMEV_NUM_EDICTS);
        GEN_CASE(GAMEV_MAX_EDICTS);

    default:
        return "unknown";
    }
}


void QUAKE2_GameSupport::update_exports() {
    if (!orig_export)
        return;

    bool changed = false;

    // if entity data changed, we need to send a G_LOCATE_GAME_DATA so plugins can hook it
    if (qmm_export.edicts != orig_export->edicts
        || qmm_export.edict_size != orig_export->edict_size
        || qmm_export.num_edicts != orig_export->num_edicts
        ) {
        changed = true;
    }

    qmm_export.edicts = orig_export->edicts;
    qmm_export.edict_size = orig_export->edict_size;
    qmm_export.num_edicts = orig_export->num_edicts;
    qmm_export.max_edicts = orig_export->max_edicts;

    if (changed) {
        // this will trigger this message to be fired to plugins, and then it will be handled
        // by the empty "case G_LOCATE_GAME_DATA" in syscall
        qmm_syscall(G_LOCATE_GAME_DATA, (intptr_t)qmm_export.edicts, qmm_export.num_edicts, qmm_export.edict_size, nullptr, 0);
    }
}


game_import_t QUAKE2_GameSupport::orig_import = {};

// a copy of the original export struct pointer that comes from the mod
game_export_t* QUAKE2_GameSupport::orig_export = nullptr;


std::map<int, std::string> QUAKE2_GameSupport::configstrings;
void QUAKE2_GameSupport::configstring(int num, char* configstring) {
    // if configstring is null, remove entry in map. otherwise store in map
    if (!configstring)
        configstrings.erase(num);
    else
        configstrings[num] = configstring;
    qmm_syscall(G_CONFIGSTRING, num, configstring);
}


game_import_t QUAKE2_GameSupport::qmm_import = {
    GEN_IMPORT(bprintf, G_BPRINTF),
    GEN_IMPORT(dprintf, G_DPRINTF),
    GEN_IMPORT(cprintf, G_CPRINTF),
    GEN_IMPORT(centerprintf, G_CENTERPRINTF),
    GEN_IMPORT_6(sound, G_SOUND, void, edict_t*, int, int, float, float, float),
    GEN_IMPORT_7(positioned_sound, G_POSITIONED_SOUND, void, vec3_t, edict_t*, int, int, float, float, float),
    QUAKE2_GameSupport::configstring,
    GEN_IMPORT(error, G_ERROR),
    GEN_IMPORT(modelindex, G_MODELINDEX),
    GEN_IMPORT(soundindex, G_SOUNDINDEX),
    GEN_IMPORT(imageindex, G_IMAGEINDEX),
    GEN_IMPORT(setmodel, G_SETMODEL),
    GEN_IMPORT(trace, G_TRACE),
    GEN_IMPORT(pointcontents, G_POINT_CONTENTS),
    GEN_IMPORT(inPVS, G_IN_PVS),
    GEN_IMPORT(inPHS, G_IN_PHS),
    GEN_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE),
    GEN_IMPORT(AreasConnected, G_AREAS_CONNECTED),
    GEN_IMPORT(linkentity, G_LINKENTITY),
    GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
    GEN_IMPORT(BoxEdicts, G_BOXEDICTS),
    GEN_IMPORT(Pmove, G_PMOVE),
    GEN_IMPORT(multicast, G_MULTICAST),
    GEN_IMPORT(unicast, G_UNICAST),
    GEN_IMPORT(WriteChar, G_MSG_WRITECHAR),
    GEN_IMPORT(WriteByte, G_MSG_WRITEBYTE),
    GEN_IMPORT(WriteShort, G_MSG_WRITESHORT),
    GEN_IMPORT(WriteLong, G_MSG_WRITELONG),
    GEN_IMPORT_1(WriteFloat, G_MSG_WRITEFLOAT, void, float),
    GEN_IMPORT(WriteString, G_MSG_WRITESTRING),
    GEN_IMPORT(WritePosition, G_MSG_WRITEPOSITION),
    GEN_IMPORT(WriteDir, G_MSG_WRITEDIR),
    GEN_IMPORT_1(WriteAngle, G_MSG_WRITEANGLE, void, float),
    GEN_IMPORT(TagMalloc, G_TAGMALLOC),
    GEN_IMPORT(TagFree, G_TAGFREE),
    GEN_IMPORT(FreeTags, G_FREETAGS),
    GEN_IMPORT(cvar, G_CVAR),
    GEN_IMPORT(cvar_set, G_CVAR_SET),
    GEN_IMPORT(cvar_forceset, G_CVAR_FORCESET),
    GEN_IMPORT(argc, G_ARGC),
    GEN_IMPORT(argv, G_ARGV),
    GEN_IMPORT(args, G_ARGS),
    GEN_IMPORT(AddCommandString, G_ADDCOMMANDSTRING),
    GEN_IMPORT_2(DebugGraph, G_DEBUGGRAPH, void, float, int),
};


std::map<intptr_t, std::string> QUAKE2_GameSupport::userinfos;
qboolean QUAKE2_GameSupport::ClientConnect(edict_t* ent, char* userinfo) {
    // get client number (ent->s.number is not set until CLIENT_BEGIN, so calculate based on edict_t*)
    if (orig_export && orig_export->edicts && orig_export->edict_size) {
        intptr_t entnum = ((intptr_t)ent - (intptr_t)orig_export->edicts) / orig_export->edict_size;
        intptr_t clientnum = entnum - 1;
        // if userinfo is null, remove entry in map. otherwise store in map
        if (!userinfo)
            userinfos.erase(clientnum);
        else
            userinfos[clientnum] = userinfo;
    }
    cgameinfo.is_from_QMM = true;
    return ::vmMain(GAME_CLIENT_CONNECT, ent, userinfo);
}


void QUAKE2_GameSupport::ClientUserinfoChanged(edict_t* ent, char* userinfo) {
    // get client number (ent->s.number is not set until CLIENT_BEGIN, so calculate based on edict_t*)
    if (orig_export && orig_export->edicts && orig_export->edict_size) {
        intptr_t entnum = ((intptr_t)ent - (intptr_t)orig_export->edicts) / orig_export->edict_size;
        intptr_t clientnum = entnum - 1;
        // if userinfo is null, remove entry in map. otherwise store in map
        if (!userinfo)
            userinfos.erase(clientnum);
        else
            userinfos[clientnum] = userinfo;
    }
    cgameinfo.is_from_QMM = true;
    (void)::vmMain(GAME_CLIENT_USERINFO_CHANGED, ent, userinfo);
}


// track entstrings for our G_GET_ENTITY_TOKEN syscall
std::vector<std::string> QUAKE2_GameSupport::entity_tokens;
size_t QUAKE2_GameSupport::token_counter = 0;
void QUAKE2_GameSupport::SpawnEntities(char* mapname, char* entstring, char* spawnpoint) {
    if (entstring) {
        entity_tokens = util_parse_entstring(entstring);
        token_counter = 0;
    }
    cgameinfo.is_from_QMM = true;
    (void)::vmMain(GAME_SPAWN_ENTITIES, mapname, entstring, spawnpoint);
}


game_export_t QUAKE2_GameSupport::qmm_export = {
    GAME_API_VERSION,	// apiversion
    GEN_EXPORT(Init, GAME_INIT),
    GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
    QUAKE2_GameSupport::SpawnEntities,
    GEN_EXPORT(WriteGame, GAME_WRITE_GAME),
    GEN_EXPORT(ReadGame, GAME_READ_GAME),
    GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
    GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
    QUAKE2_GameSupport::ClientConnect,
    GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
    QUAKE2_GameSupport::ClientUserinfoChanged,
    GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
    GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
    GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
    GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
    GEN_EXPORT(ServerCommand, GAME_SERVER_COMMAND),
    // the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
    nullptr,	// edicts
    0,			// edict_size
    0,			// num_edicts
    0,			// max_edicts
};
