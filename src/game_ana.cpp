/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "osdef.h"

#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_32)

#define _CRT_SECURE_NO_WARNINGS 1
#include <cstdio>
#include <map>
#include <vector>
#include <string>
// #include <quake2/game/q_shared.h>
// #include <quake2/game/game.h>
#include "gameapi.hpp"
#include "log.hpp"
#include "format.hpp"
// QMM-specific ANA header
#include "game_ana.h"
#include "qmm.hpp"
#include "mod.hpp"
#include "util.hpp"

struct game_export_t;
struct game_import_t;
struct edict_t;
typedef int qboolean;

struct ANA_GameSupport : public GameSupport {
    virtual const char* EngMsgName(intptr_t msg);
    virtual const char* ModMsgName(intptr_t msg);
    virtual bool AutoDetect(APIType engine_api);
    virtual void* Entry(void* syscall, void*, APIType engine_api);
    virtual bool ModLoad(void* entry, APIType mod_api);
    virtual void ModUnload(APIType mod_api);
    virtual intptr_t QMMEngMsg(intptr_t msg) { return qmm_eng_msgs[msg]; }
    virtual intptr_t QMMModMsg(intptr_t msg) { return qmm_mod_msgs[msg]; }

    virtual intptr_t syscall_args(intptr_t, intptr_t* args);
    virtual intptr_t vmMain_args(intptr_t, intptr_t* args);

    virtual const char* DefaultDLLName() { return "game" MOD_DLL; }
    virtual const char* DefaultModDir() { return "."; }
    virtual const char* ModCvar() { return "game"; }
    virtual const char* GameName() { return "Anachronox"; }
    virtual const char* GameCode() { return "ANA"; }

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
    // static game_import_t orig_import;

    // a copy of the original export struct pointer that comes from the mod
    static game_export_t* orig_export;

    // struct with lambdas that call QMM's syscall function. this is given to the mod
    // static game_import_t qmm_import;

    // struct with lambdas that call QMM's vmMain function. this is given to the game engine
    // static game_export_t qmm_export;

    const intptr_t qmm_eng_msgs[QMM_ENGINE_MSG_COUNT] = { 0 };// = GEN_GAME_QMM_ENG_MSGS();
    const intptr_t qmm_mod_msgs[QMM_MOD_MSG_COUNT] = { 0 };// = GEN_GAME_QMM_MOD_MSGS();

};

typedef FARPROC(WINAPI *pfnGPA)(HMODULE hModule, LPCSTR lpProcName);
static pfnGPA old_GetProcAddress;
static FARPROC WINAPI GetProcAddress_Hook(HMODULE hModule, LPCSTR lpProcName);
static void* install_hook(HMODULE target_module, const char* dll_name, const char* function_name, void* function_hook);

GEN_GAME_OBJ(ANA);


// auto-detection logic for ANA
bool ANA_GameSupport::AutoDetect(APIType engine_api) {
    if (engine_api != QMM_API_GETGAMEAPI)
        return false;

    if (!Util::str_striequal(QMM::qmm_file, DefaultDLLName()))
        return false;

    if (!Util::str_stristr(QMM::exe_file, "anox"))
        return false;

    return true;
}


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t ANA_GameSupport::syscall_args(intptr_t cmd, intptr_t* args) {
    (void)args;

    if (cmd != G_PRINT)
        QMMLOG(QMM_LOG_TRACE, "QMM") << "ANA_GameSupport::syscall(" << EngMsgName(cmd) << "(" << cmd << ")) called\n";

    // update export vars before calling into the engine
    if (cmd != G_PRINT)
        update_exports();

    intptr_t ret = 0; 
#if 0
    if (orig_import.bprintf) {
        switch (cmd) {
            ROUTE_IMPORT(bprintf, G_BPRINTF);
            ROUTE_IMPORT(dprintf, G_DPRINTF);
            ROUTE_IMPORT(cprintf, G_CPRINTF);
            ROUTE_IMPORT(centerprintf, G_CENTERPRINTF);
            ROUTE_IMPORT_6_V(sound, G_SOUND, edict_t*, int, int, FLOAT_CAST, FLOAT_CAST, FLOAT_CAST);
            ROUTE_IMPORT_7_V(positioned_sound, G_POSITIONED_SOUND, float*, edict_t*, int, int, FLOAT_CAST, FLOAT_CAST, FLOAT_CAST);
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
            ROUTE_IMPORT_1_V(WriteFloat, G_MSG_WRITEFLOAT, FLOAT_CAST);
            ROUTE_IMPORT(WriteString, G_MSG_WRITESTRING);
            ROUTE_IMPORT(WritePosition, G_MSG_WRITEPOSITION);
            ROUTE_IMPORT(WriteDir, G_MSG_WRITEDIR);
            ROUTE_IMPORT_1_V(WriteAngle, G_MSG_WRITEANGLE, FLOAT_CAST);
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
            ROUTE_IMPORT_2_V(DebugGraph, G_DEBUGGRAPH, FLOAT_CAST, int);

            // handle cmds for variables, this is how a plugin would get these values if needed

            // handle special cmds which QMM uses but ANA doesn't have an analogue for
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
                    Util::strncpyz(buffer, cvar->string, (size_t)bufsize);
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
                // quake2: void (*bprintf) (int printlevel, char *fmt, ...);
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
                std::string path = fmt::format("{}/{}", QMM::qmm_dir, qpath);
                if (mode != FS_READ)
                    Util::path_mkdir(Util::path_dirname(path));
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
                for (int i = 0; i < 50; i++) {  // prevent infinite loops trying to read
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
                for (int i = 0; i < 50; i++) {  // prevent infinite loops trying to write
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
                    Util::strncpyz(buffer, userinfos[num].c_str(), (size_t)bufferSize);
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

                Util::strncpyz(buffer, entity_tokens[token_counter++].c_str(), (size_t)bufferSize);
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
            case G_MILLISECONDS:
                ret = Util::util_get_milliseconds();
                break;

            default:
                break;
        };

        // do anything that needs to be done after function call here
    }
#endif

    if (cmd != G_PRINT)
        QMMLOG(QMM_LOG_TRACE, "QMM") << "ANA_GameSupport::syscall(" << EngMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t ANA_GameSupport::vmMain_args(intptr_t cmd, intptr_t* args) {
    (void)args;

    QMMLOG(QMM_LOG_TRACE, "QMM") << "ANA_GameSupport::vmMain(" << ModMsgName(cmd) << "(" << cmd << ")) called\n";

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

#if 0
    if (orig_export) {
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
    }
#endif

    QMMLOG(QMM_LOG_TRACE, "QMM") << "ANA_GameSupport::vmMain(" << ModMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";

    return ret;
}


void* ANA_GameSupport::Entry(void* import, void*, APIType engine_api) {
    QMMLOG(QMM_LOG_DEBUG, "QMM") << "ANA_GameSupport::Entry(" << import << ") called\n";

    if (engine_api == QMM_API_GETGAMEAPI) {
        // original import struct from engine
        // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
        game_import_t* gi = (game_import_t*)import;
        (void)gi;
        // orig_import = *gi;

        // fill in variables of our hooked import struct to pass to the mod
        // qmm_import.x = orig_import.x;
    }

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "ANA_GameSupport::Entry(" << import << ") returning " << nullptr/*&qmm_export*/ << "\n";

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // the only thing in this struct the engine uses before calling Init is the apiversion
    return nullptr; // &qmm_export;
}


bool ANA_GameSupport::ModLoad(void* entry, APIType mod_api) {
    if (mod_api != QMM_API_GETGAMEAPI)
        return false;

    mod_GetGameAPI pfnGGA = (mod_GetGameAPI)entry;
    orig_export = (game_export_t*)pfnGGA(nullptr/*&qmm_import*/, nullptr);

    return !!orig_export;
}


void ANA_GameSupport::ModUnload(APIType) {
    orig_export = nullptr;
    configstrings.clear();
    entity_tokens.clear();
    token_counter = 0;
    userinfos.clear();
}


const char* ANA_GameSupport::EngMsgName(intptr_t msg) {
    switch (msg) {
#if 0
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
#endif
        case 0: return "unknown";
        default:
            return "unknown";
    }
}


const char* ANA_GameSupport::ModMsgName(intptr_t msg) {
    switch (msg) {
#if 0
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
#endif

        case 0: return "unknown";
        default:
            return "unknown";
    }
}

void ANA_GameSupport::update_exports() {
    if (!orig_export)
        return;

#if 0
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
        intptr_t args[] = { (intptr_t)qmm_export.edicts, qmm_export.num_edicts, (intptr_t)qmm_export.edict_size, (intptr_t)nullptr, 0 };
        (void)QMM::syscall_args(G_LOCATE_GAME_DATA, args);
    }
#endif 
}


// game_import_t ANA_GameSupport::orig_import = {};

// a copy of the original export struct pointer that comes from the mod
game_export_t* ANA_GameSupport::orig_export = nullptr;

std::map<int, std::string> ANA_GameSupport::configstrings;
void ANA_GameSupport::configstring(int num, char* configstring) {
    (void)num; (void)configstring;
#if 0
    // if configstring is null, remove entry in map. otherwise store in map
    if (!configstring)
        configstrings.erase(num);
    else
        configstrings[num] = configstring;
    intptr_t args[] = { num, (intptr_t)configstring };
    (void)QMM::syscall_args(G_CONFIGSTRING, args);
#endif
}


#if 0
game_import_t ANA_GameSupport::qmm_import = {
    GEN_IMPORT(bprintf, G_BPRINTF),
    GEN_IMPORT(dprintf, G_DPRINTF),
    GEN_IMPORT(cprintf, G_CPRINTF),
    GEN_IMPORT(centerprintf, G_CENTERPRINTF),
    GEN_IMPORT_6(sound, G_SOUND, void, edict_t*, int, int, float, float, float),
    GEN_IMPORT_7(positioned_sound, G_POSITIONED_SOUND, void, vec3_t, edict_t*, int, int, float, float, float),
    ANA_GameSupport::configstring,
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
#endif


std::map<intptr_t, std::string> ANA_GameSupport::userinfos;
qboolean ANA_GameSupport::ClientConnect(edict_t* ent, char* userinfo) {
    return ent && userinfo;
#if 0
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
    intptr_t args[] = { (intptr_t)ent, (intptr_t)userinfo };
    return QMM::vmMain_args(GAME_CLIENT_CONNECT, args);
#endif
}


void ANA_GameSupport::ClientUserinfoChanged(edict_t* ent, char* userinfo) {
    (void)ent; (void)userinfo;
#if 0
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
    intptr_t args[] = { (intptr_t)ent, (intptr_t)userinfo };
    (void)QMM::vmMain_args(GAME_CLIENT_USERINFO_CHANGED, args);
#endif
}


// track entstrings for our G_GET_ENTITY_TOKEN syscall
std::vector<std::string> ANA_GameSupport::entity_tokens;
size_t ANA_GameSupport::token_counter = 0;
void ANA_GameSupport::SpawnEntities(char* mapname, char* entstring, char* spawnpoint) {
    (void)mapname; (void)entstring; (void)spawnpoint;
#if 0
    if (entstring) {
        entity_tokens = Util::util_parse_entstring(entstring);
        token_counter = 0;
    }
    intptr_t args[] = { (intptr_t)mapname, (intptr_t)entstring, (intptr_t)spawnpoint };
    (void)QMM::vmMain_args(GAME_SPAWN_ENTITIES, args);
#endif
}


#if 0
game_export_t ANA_GameSupport::qmm_export = {
    GAME_API_VERSION,    // apiversion
    GEN_EXPORT(Init, GAME_INIT),
    GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
    ANA_GameSupport::SpawnEntities,
    GEN_EXPORT(WriteGame, GAME_WRITE_GAME),
    GEN_EXPORT(ReadGame, GAME_READ_GAME),
    GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
    GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
    ANA_GameSupport::ClientConnect,
    GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
    ANA_GameSupport::ClientUserinfoChanged,
    GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
    GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
    GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
    GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
    GEN_EXPORT(ServerCommand, GAME_SERVER_COMMAND),
    // the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
    nullptr,    // edicts
    0,          // edict_size
    0,          // num_edicts
    0,          // max_edicts
};
#endif

C_DLLEXPORT void NPServer_Vid_Change() {
    old_GetProcAddress = (pfnGPA)install_hook(GetModuleHandle(NULL), "kernel32.dll", "GetProcAddress", GetProcAddress_Hook);
}


// generate stub functions (these may need to also pass-through depending on how the game stores the pointers
// if they get re-read then the GetProcAddress hook should take care of this
#define STUB_FUNC(func) C_DLLEXPORT void func() { }
STUB_FUNC(GAME_GetContext)
STUB_FUNC(GAME_SearchContext_va)
STUB_FUNC(NAServer_Attach)
STUB_FUNC(NPServer_GetStopList)
STUB_FUNC(SPAWN_InhibitEntitiesByField)
STUB_FUNC(game_CallTouchFunc)
STUB_FUNC(game_CallUseFunc)
STUB_FUNC(NPServer_StopAll)
STUB_FUNC(GAME_PushContext)
STUB_FUNC(GAME_PopContext)
STUB_FUNC(NPServer_Start)
STUB_FUNC(NAServer_DetachAll)
STUB_FUNC(NAServer_Detach)
STUB_FUNC(game_CallThinkFunc)
STUB_FUNC(NPServer_FastForward)
STUB_FUNC(SPAWN_UninhibitEntitiesByField)
STUB_FUNC(NPServer_Stop)
STUB_FUNC(SPAWN_InhibitEntity)

// these all get grabbed by the GetProcAddress hook
/*
STUB_FUNC(GAME_GetPreviousContext)
STUB_FUNC(GAME_PopAllContext)
STUB_FUNC(G_GetGameLevelName)
STUB_FUNC(QDllEntry)
STUB_FUNC(SPAWN_IsInhibited)
STUB_FUNC(evaluate_spawn_condition)
STUB_FUNC(game_CallBlockedFunc)
STUB_FUNC(BLOCKED_door_blocked)
STUB_FUNC(BLOCKED_plat_blocked)
STUB_FUNC(BLOCKED_rotating_blocked)
STUB_FUNC(BLOCKED_train_blocked)
STUB_FUNC(THINK_anglemove_begin)
STUB_FUNC(THINK_anglemove_done)
STUB_FUNC(THINK_anglemove_final)
STUB_FUNC(THINK_button_done)
STUB_FUNC(THINK_button_return)
STUB_FUNC(THINK_button_wait)
STUB_FUNC(THINK_delay)
STUB_FUNC(THINK_door_check_clearance)
STUB_FUNC(THINK_door_go_down)
STUB_FUNC(THINK_door_hit_bottom)
STUB_FUNC(THINK_door_hit_top)
STUB_FUNC(THINK_edict_free)
STUB_FUNC(THINK_elevator_init)
STUB_FUNC(THINK_flymonster_start_go)
STUB_FUNC(THINK_monster_spawn)
STUB_FUNC(THINK_monster_think)
STUB_FUNC(THINK_move_accel)
STUB_FUNC(THINK_move_begin)
STUB_FUNC(THINK_move_calc_speed)
STUB_FUNC(THINK_move_done)
STUB_FUNC(THINK_move_final)
STUB_FUNC(THINK_multi_wait)
STUB_FUNC(THINK_plat_go_down)
STUB_FUNC(THINK_plat_go_up)
STUB_FUNC(THINK_plat_hit_bottom)
STUB_FUNC(THINK_plat_hit_top)
STUB_FUNC(THINK_ridebox_findowner)
STUB_FUNC(THINK_ridebox_think)
STUB_FUNC(THINK_sprite_think)
STUB_FUNC(THINK_target_lightramp_think)
STUB_FUNC(THINK_target_speaker_think)
STUB_FUNC(THINK_train_find)
STUB_FUNC(THINK_train_next)
STUB_FUNC(THINK_train_wait)
STUB_FUNC(THINK_trash_generate)
STUB_FUNC(THINK_trash_think)
STUB_FUNC(THINK_trigger_console_wait)
STUB_FUNC(THINK_wind_timer)
STUB_FUNC(TOUCH_button_touch)
STUB_FUNC(TOUCH_door_touch)
STUB_FUNC(TOUCH_doortrigger_touch)
STUB_FUNC(TOUCH_multi_touch)
STUB_FUNC(TOUCH_path_corner_touch)
STUB_FUNC(TOUCH_plat_center_touch)
STUB_FUNC(TOUCH_rotating_touch)
STUB_FUNC(TOUCH_trigger_battle_touch)
STUB_FUNC(TOUCH_trigger_changelevel_touch)
STUB_FUNC(TOUCH_trigger_console_once_touch)
STUB_FUNC(TOUCH_trigger_console_touch)
STUB_FUNC(TOUCH_trigger_push_touch)
STUB_FUNC(TOUCH_trigger_watercurrent_touch)
STUB_FUNC(USE_areaportal_use)
STUB_FUNC(USE_button_use)
STUB_FUNC(USE_conveyor_use)
STUB_FUNC(USE_door_use)
STUB_FUNC(USE_elevator_use)
STUB_FUNC(USE_light_use)
STUB_FUNC(USE_lightent_use)
STUB_FUNC(USE_monster_spawn_use)
STUB_FUNC(USE_multi_use)
STUB_FUNC(USE_particle_use)
STUB_FUNC(USE_plat_use)
STUB_FUNC(USE_rotating_use)
STUB_FUNC(USE_spew_use)
STUB_FUNC(USE_target_lightramp_use)
STUB_FUNC(USE_target_speaker_use)
STUB_FUNC(USE_train_use)
STUB_FUNC(USE_trigger_counter_use)
STUB_FUNC(USE_trigger_enable)
STUB_FUNC(USE_trigger_relay_use)
STUB_FUNC(USE_wall_use)
*/

// This should redirect unknown QMM functions towards the mod DLL
FARPROC WINAPI GetProcAddress_Hook(HMODULE hModule, LPCSTR lpProcName) {
    FARPROC old_ret = old_GetProcAddress(hModule, lpProcName);

    if (!old_ret && (void*)hModule == QMM::qmm_module_ptr) {
        return GetProcAddress((HMODULE)Mod::dll, lpProcName);
    }

    return old_ret;
};


static void* install_hook(HMODULE target_module, const char* dll_name, const char* function_name, void* function_hook) {
    char* module_base = (char*)target_module;   // module base as a char* for adding RVAs
    void* ret = nullptr;                        // store and return old function address
    MEMORY_BASIC_INFORMATION thunkMemInfo;
    DWORD oldProtect, _;
    PIMAGE_DOS_HEADER dosHeaders = (PIMAGE_DOS_HEADER)module_base;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(module_base + dosHeaders->e_lfanew);
    PIMAGE_DATA_DIRECTORY importDescriptorTable = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(module_base + importDescriptorTable->VirtualAddress);

    // loop through each descriptor entry looking for a matching DLL name (NULL = check every DLL)
    for (; importDescriptor->Name != 0; importDescriptor++) {
        PIMAGE_THUNK_DATA thunk_name = (PIMAGE_THUNK_DATA)(module_base + importDescriptor->OriginalFirstThunk);
        PIMAGE_THUNK_DATA thunk_addr = (PIMAGE_THUNK_DATA)(module_base + importDescriptor->FirstThunk);

        // if this dll isn't the one we're looking for, continue 
        if (dll_name && _stricmp(module_base + importDescriptor->Name, dll_name) != 0)
            continue;

        // loop as long as RVA of imported name (AddressOfData) is not 0
        for (; thunk_addr->u1.Function != 0; thunk_name++, thunk_addr++) {
            // skip if the function was imported by ordinal number
            if ((thunk_name->u1.Ordinal & IMAGE_ORDINAL_FLAG) == IMAGE_ORDINAL_FLAG)
                continue;

            // if this function's name matches what we want
            if (!strcmp(function_name, ((PIMAGE_IMPORT_BY_NAME)(module_base + thunk_name->u1.AddressOfData))->Name)) {
                // get old function address
                ret = (void*)thunk_addr->u1.Function;

                // get memory page info for our desired thunk
                if (!VirtualQuery(&thunk_addr->u1.Function, &thunkMemInfo, sizeof(thunkMemInfo)))
                    return nullptr;

                // allow writing to the thunk page
                if (!VirtualProtect(thunkMemInfo.BaseAddress, thunkMemInfo.RegionSize, PAGE_READWRITE, &oldProtect))
                    return nullptr;

                // replace function with the hook
                thunk_addr->u1.Function = (intptr_t)function_hook;

                // restore previous permissions to thunk page
                VirtualProtect(thunkMemInfo.BaseAddress, thunkMemInfo.RegionSize, oldProtect, &_);

                // return old function address
                return ret;
            }
        } // thunk loop
    } // importDescriptor loop

    return nullptr;
}

#endif // QMM_OS_WINDOWS && QMM_ARCH_32

