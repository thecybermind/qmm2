/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "osdef.h"

#if defined(QMM_ARCH_32)

#define _CRT_SECURE_NO_WARNINGS 1
#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <sof/gamecpp/q_shared.h>
#include <sof/gamecpp/game.h>

#include "gameapi.hpp"
#include "log.hpp"
#include "format.hpp"
// QMM-specific SOF header
#include "game_sof.h"
#include "qmm.hpp"
#include "util.hpp"

struct SOF_GameSupport : public GameSupport {
    virtual const char* EngMsgName(intptr_t msg);
    virtual const char* ModMsgName(intptr_t msg);
    virtual bool AutoDetect(APIType engine_api);
    virtual void* Entry(void* syscall, void*, APIType engine_api);
    virtual bool ModLoad(void* entry, APIType mod_api);
    virtual void ModUnload(APIType mod_api);
    virtual int QMMEngMsg(int msg) { return qmm_eng_msgs[msg]; }
    virtual int QMMModMsg(int msg) { return qmm_mod_msgs[msg]; }

    virtual intptr_t syscall_args(intptr_t, intptr_t* args);
    virtual intptr_t vmMain_args(intptr_t, intptr_t* args);

    virtual const char* DefaultDLLName() { return "game" MOD_DLL; }
    virtual const char* DefaultModDir() { return "base"; }
    virtual const char* ModCvar() { return "game"; }
    virtual const char* GameName() { return "Soldier of Fortune"; }
    virtual const char* GameCode() { return "SOF"; }

private:
    // update the export variables from orig_export
    static void update_exports();

    // callback for registered cvar commands
    static void cvar_cmd_callback(cvar_t* cvar);

    // track args for "qmm" cvar to be retrieved by G_ARGC/G_ARGV/G_ARGS
    static std::vector<std::string> command_args;
    static void store_args(std::string cvarname, std::string cvarvalue);

    // track file info for FS_LoadFile
    static std::map<fileHandle_t, intptr_t> file_lengths;
    static std::map<fileHandle_t, intptr_t> file_pos;

    // track configstrings for our G_GET_CONFIGSTRING syscall
    static std::map<int, std::string> configstrings;
    static void configstring(int num, const char* configstring);

    // track userinfo for our G_GET_USERINFO syscall
    static std::map<intptr_t, std::string> userinfos;
    static qboolean ClientConnect(edict_t* ent, char* userinfo);
    static void ClientUserinfoChanged(edict_t* ent, char* userinfo, bool not_first_time);

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

GEN_GAME_OBJ(SOF);


// auto-detection logic for SOF
bool SOF_GameSupport::AutoDetect(APIType engine_api) {
    if (engine_api != QMM_API_GETGAMEAPI)
        return false;

    if (!Util::str_striequal(QMM::qmm_file, DefaultDLLName()))
        return false;

    if (!Util::str_stristr(QMM::exe_file, "sof"))
        return false;

    return true;
}


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t SOF_GameSupport::syscall_args(intptr_t cmd, intptr_t* args) {
    if (cmd != G_PRINT)
        QMMLOG(QMM_LOG_TRACE, "QMM") << "SOF_GameSupport::syscall(" << EngMsgName(cmd) << "(" << cmd << ")) called\n";

    // update export vars before calling into the engine
    update_exports();

    intptr_t ret = 0;

    float fret; // used to get float return values
    if (orig_import.modelindex) {
        switch (cmd) {
            ROUTE_IMPORT(modelindex, G_MODELINDEX);
            ROUTE_IMPORT(soundindex, G_SOUNDINDEX);
            ROUTE_IMPORT(effectindex, G_EFFECTINDEX);
            ROUTE_IMPORT(imageindex, G_IMAGEINDEX);
            ROUTE_IMPORT(unload_sound, G_UNLOAD_SOUND);
            ROUTE_IMPORT(FilterPacket, G_FILTERPACKET);
            ROUTE_IMPORT(CreateGhoulConfigStrings, G_CREATEGHOULCONFIGSTRINGS);
            ROUTE_IMPORT(setmodel, G_SETMODEL);
            ROUTE_IMPORT(setrendermodel, G_SETRENDERMODEL);
            // handled later to allow for obtaining GAME_CONSOLE_COMMAND args
            // ROUTE_IMPORT(argc, G_ARGC);
            // ROUTE_IMPORT(argv, G_ARGV);
            // ROUTE_IMPORT(args, G_ARGS);
            ROUTE_IMPORT(bprintf, G_BPRINTF);
            ROUTE_IMPORT(dprintf, G_DPRINTF);
            ROUTE_IMPORT(cprintf, G_CPRINTF);
            ROUTE_IMPORT(clprintf, G_CLPRINTF);
            ROUTE_IMPORT(welcomeprint, G_WELCOMEPRINT);
            ROUTE_IMPORT(centerprintf, G_CENTERPRINTF);
            ROUTE_IMPORT(cinprintf, G_CINPRINTF);
            ROUTE_IMPORT(bcaption, G_BCAPTION);
            ROUTE_IMPORT(captionprintf, G_CAPTIONPRINTF);
            ROUTE_IMPORT(Con_ClearNotify, G_CON_CLEARNOTIFY);
            ROUTE_IMPORT_7_V(sound, G_SOUND, edict_t*, int, int, FLOAT_CAST, FLOAT_CAST, FLOAT_CAST, int);
            ROUTE_IMPORT_8_V(positioned_sound, G_POSITIONED_SOUND, float*, edict_t*, int, int, FLOAT_CAST, FLOAT_CAST, FLOAT_CAST, int);
            ROUTE_IMPORT_2_V(DebugGraph, G_DEBUGGRAPH, FLOAT_CAST, int);
            ROUTE_IMPORT(DamageTexture, G_DAMAGETEXTURE);
            ROUTE_IMPORT(SurfaceTypeList, G_SURFACETYPELIST);
            ROUTE_IMPORT_2_V(Update, G_UPDATE, FLOAT_CAST, bool);
            ROUTE_IMPORT(multicast, G_MULTICAST);
            ROUTE_IMPORT(multicastignore, G_MULTICASTIGNORE);
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
            ROUTE_IMPORT(WriteByteSizebuf, G_WRITEBYTE_SIZEBUF);
            ROUTE_IMPORT(WriteShortSizebuf, G_WRITESHORT_SIZEBUF);
            ROUTE_IMPORT(WriteLongSizebuf, G_WRITELONG_SIZEBUF);
            ROUTE_IMPORT(ReliableWriteByteToClient, G_RELIABLE_WRITEBYTE_TOCLIENT);
            ROUTE_IMPORT(ReliableWriteDataToClient, G_RELIABLE_WRITEDATA_TOCLIENT);
            ROUTE_IMPORT(GetNearestByteNormal, G_GETNEARESTBYTENORMAL);
            ROUTE_IMPORT(sendPlayernameColors, G_SENDPLAYERNAMECOLORS);
            ROUTE_IMPORT(SP_Register, G_SP_REGISTER);
            ROUTE_IMPORT(SP_Print, G_SP_PRINT);
            ROUTE_IMPORT(SP_Print_Obit, G_SP_PRINT_OBIT);
            ROUTE_IMPORT(SP_SPrint, G_SP_SPRINT);
            ROUTE_IMPORT(SP_GetStringText, G_SP_GETSTRINGTEXT);
            ROUTE_IMPORT(trace, G_TRACE);
            ROUTE_IMPORT(polyTrace, G_POLYTRACE);
            ROUTE_IMPORT(pointcontents, G_POINT_CONTENTS);
            ROUTE_IMPORT(RegionDistance, G_REGIONDISTANCE);
            ROUTE_IMPORT(inPVS, G_IN_PVS);
            ROUTE_IMPORT(inPHS, G_IN_PHS);
            ROUTE_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE);
            ROUTE_IMPORT(AreasConnected, G_AREAS_CONNECTED);
            ROUTE_IMPORT(GetGhoul, G_GETGHOUL);
            ROUTE_IMPORT(NewPlayerModelInfo, G_NEWPLAYERMODELINFO);
            ROUTE_IMPORT(FindGSQFile, G_FINDGSQFILE);
            ROUTE_IMPORT(ReadGsqEntry, G_READGSQENTRY);
            ROUTE_IMPORT(PrecacheGSQFile, G_PRECACHEGSQFILE);
            ROUTE_IMPORT(RegisterGSQSequences, G_REGISTERGSQSEQUENCES);
            ROUTE_IMPORT(TurnOffPartsFromGSQFile, G_TURNOFFPARTSFROMGSQFILE);
            ROUTE_IMPORT(configstring, G_CONFIGSTRING);
            ROUTE_IMPORT(SZ_Init, G_SZ_INIT);
            ROUTE_IMPORT(SZ_Clear, G_SZ_CLEAR);
            ROUTE_IMPORT(SZ_Write, G_SZ_WRITE);
            ROUTE_IMPORT(error, G_ERROR);
            ROUTE_IMPORT(Sys_ConsoleOutput, G_SYS_CONSOLEOUTPUT);
            ROUTE_IMPORT(Sys_GetPlayerAPI, G_SYS_GETPLAYERAPI);
            ROUTE_IMPORT(Sys_UnloadPlayer, G_SYS_UNLOADPLAYER);
            ROUTE_IMPORT_2_F(flrand, G_FLRAND, FLOAT_CAST, FLOAT_CAST);
            ROUTE_IMPORT(irand, G_IRAND);
            ROUTE_IMPORT(linkentity, G_LINKENTITY);
            ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
            ROUTE_IMPORT(BoxEdicts, G_BOXEDICTS);
            ROUTE_IMPORT(Pmove, G_PMOVE);
            ROUTE_IMPORT(TagMalloc, G_TAGMALLOC);
            ROUTE_IMPORT(TagFree, G_TAGFREE);
            ROUTE_IMPORT(FreeTags, G_FREETAGS);
            ROUTE_IMPORT(AppendToSavegame, G_APPENDTOSAVEGAME);
            ROUTE_IMPORT(ReadFromSavegame, G_READFROMSAVEGAME);
            ROUTE_IMPORT(cvar, G_CVAR);
            ROUTE_IMPORT(cvar_set, G_CVAR_SET);
            ROUTE_IMPORT_2_V(cvar_setvalue, G_CVAR_SETVALUE, const char*, FLOAT_CAST);
            ROUTE_IMPORT(cvar_forceset, G_CVAR_FORCESET);
            ROUTE_IMPORT(cvar_info, G_CVAR_INFO);
            ROUTE_IMPORT_1_F(cvar_variablevalue, G_CVAR_VARIABLEVALUE, const char*);
            ROUTE_IMPORT(FS_LoadFile, G_FS_LOADFILE);
            ROUTE_IMPORT(FS_FreeFile, G_FS_FREEFILE);
            ROUTE_IMPORT(FS_Userdir, G_FS_USERDIR);
            ROUTE_IMPORT(FS_CreatePath, G_FS_CREATEPATH);
            ROUTE_IMPORT(FS_FileExists, G_FS_FILEEXISTS);
            ROUTE_IMPORT(AddCommandString, G_ADDCOMMANDSTRING);

            // handle cmds for variables, this is how a plugin would get these values if needed
            ROUTE_IMPORT_VAR(isClient, GVP_ISCLIENT);

            // if command_args has values (set before and cleared after GAME_CONSOLE_COMMAND), grab values from it.
            // otherwise pass to engine
            case G_ARGC:
                ret = command_args.empty() ? orig_import.argc() : (intptr_t)command_args.size();
                break;
            case G_ARGV:
                if (command_args.empty())
                    ret = (intptr_t)orig_import.argv(args[0]);
                else if ((intptr_t)command_args.size() > args[0])
                    ret = (intptr_t)command_args[args[0]].c_str();
                break;
            case G_ARGS: {
                // sof: char* (*args)(void);
                if (command_args.empty()) {
                    ret = (intptr_t)orig_import.args();
                    break;
                }
                static std::string s;
                s = "";
                bool first = true;
                for (std::string& a : command_args) {
                    // skip first arg since G_ARGS doesn't include it
                    if (first) {
                        first = false;
                        continue;
                    }
                    if (!s.empty())
                        s += "";
                    s += a;
                }
                ret = (intptr_t)s.c_str();
                break;
            }
            case G_QMM_REGISTER_CONSOLE_COMMAND:
                // void (const char *cmd)
                // create the given cvar name with its command callback sending a GAME_CONSOLE_COMMAND signal
                (void)orig_import.cvar((const char*)args[0], "", 0, cvar_cmd_callback);
                break;
            // handle special cmds which QMM uses but SOF doesn't have an analogue for
            case G_CVAR_REGISTER: {
                // sof: cvar_t *(*cvar) (char *var_name, char *value, int flags);
                // q3a: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
                // qmm always passes NULL for vmCvar so don't worry about it
                const char* var_name = (const char*)(args[1]);
                const char* value = (const char*)(args[2]);
                int flags = (int)args[3];
                (void)orig_import.cvar(var_name, value, flags, nullptr);
                break;
            }
            case G_CVAR_VARIABLE_STRING_BUFFER: {
                // sof: cvar_t *(*cvar) (char *var_name, char *value, int flags);
                // q3a: void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
                const char* var_name = (const char*)(args[0]);
                char* buffer = (char*)(args[1]);
                intptr_t bufsize = args[2];
                *buffer = '\0';
                cvar_t* cvar = orig_import.cvar(var_name, "", 0, nullptr);
                if (cvar)
                    Util::strncpyz(buffer, cvar->string, (size_t)bufsize);
                break;
            }
            case G_CVAR_VARIABLE_INTEGER_VALUE: {
                // sof: cvar_t *(*cvar) (char *var_name, char *value, int flags);
                // q3a: int trap_Cvar_VariableIntegerValue(const char* var_name)
                const char* var_name = (const char*)(args[0]);
                cvar_t* cvar = orig_import.cvar(var_name, "", 0, nullptr);
                if (cvar)
                    ret = (int)cvar->value;
                break;
            }
            case G_SEND_CONSOLE_COMMAND: {
                // sof: void (*AddCommandString)(char *text);
                // qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
                char* text = (char*)(args[1]);
                orig_import.AddCommandString(text);
                break;
            }
            case G_PRINT: {
                // sof: void (*bprintf) (int printlevel, char *fmt, ...);
                // qmm: void trap_Printf( const char *fmt );
                char* text = (char*)args[0];
                orig_import.bprintf(PRINT_HIGH, text);
                break;
            }
            case G_FS_FOPEN_FILE: {
                // provide these G_FS_ functions to plugins just so the most basic file functions all work.
                // use FILE* for writing, use G_FS_LOADFILE for reading
                // int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
                const char* qpath = (const char*)args[0];
                fileHandle_t* f = (fileHandle_t*)args[1];
                intptr_t mode = args[2];
                std::string path = fmt::format("{}/{}", QMM::qmm_dir, qpath);
                if (mode == FS_READ) {
                    ret = orig_import.FS_LoadFile((char*)path.c_str(), (void**)f, false);
                    if (ret == -1)
                        break;
                    file_lengths[*f] = ret;
                    file_pos[*f] = 0;
                    // add 1 bit to filehandle to flag that it's a G_FS_LOADFILE buffer for G_FS_FCLOSE_FILE
                    *f |= 1;
                    break;
                }
                const char* str_mode = (mode == FS_WRITE) ? "wb" : "ab";
                Util::path_mkdir(Util::path_dirname(path));
                FILE* fp = fopen(path.c_str(), str_mode);
                if (!fp) {
                    ret = -1;
                    break;
                }
                ret = (mode == FS_WRITE) ? 0 : ftell(fp);
                *f = (fileHandle_t)fp;
                break;
            }
            case G_FS_READ: {
                // void trap_FS_Read(void* buffer, int len, fileHandle_t f);
                char* buffer = (char*)args[0];
                size_t len = (size_t)args[1];
                fileHandle_t f = (fileHandle_t)args[2];
                // if this is a G_FS_LOADFILE buffer
                if (f & 1) {
                    f--;
                    // if this buffer is being tracked, read from it
                    // don't allow reading past the end of the buffer
                    if (file_lengths.count(f) && file_pos.count(f)) {
                        if (file_pos[f] + (intptr_t)len >= file_lengths[f])
                            len = file_lengths[f] - file_pos[f] - 1;
                        memcpy(buffer, (char*)f + file_pos[f], len);
                        file_pos[f] += len;
                    }
                    break;
                }
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
                // if this is somehow a G_FS_LOADFILE buffer
                if (f & 1) {
                    f--;
                    // if this buffer is being tracked, write to it
                    // don't allow writing past the end of the buffer
                    if (file_lengths.count(f) && file_pos.count(f)) {
                        if (file_pos[f] + (intptr_t)len >= file_lengths[f])
                            len = file_lengths[f] - file_pos[f] - 1;
                        memcpy((char*)f + file_pos[f], buffer, len);
                        file_pos[f] += len;
                    }
                    break;
                }
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
                // if this is a G_FS_LOADFILE buffer
                if (f & 1) {
                    f--;
                    orig_import.FS_FreeFile((void*)f);
                    file_lengths.erase(f);
                    file_pos.erase(f);
                    break;
                }
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

    if (cmd != G_PRINT)
        QMMLOG(QMM_LOG_TRACE, "QMM") << "SOF_GameSupport::syscall(" << EngMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";


    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t SOF_GameSupport::vmMain_args(intptr_t cmd, intptr_t* args) {
    QMMLOG(QMM_LOG_TRACE, "QMM") << "SOF_GameSupport::vmMain(" << ModMsgName(cmd) << "(" << cmd << ")) called\n";

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    if (orig_export) {
        switch (cmd) {
            // handled below to create "qmm" cvar to fake GAME_CONSOLE_COMMAND
            // ROUTE_EXPORT(Init, GAME_INIT);
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
            ROUTE_EXPORT(ResetCTFTeam, GAME_RESETCTFTEAM);
            ROUTE_EXPORT(GameAllowASave, GAME_GAMEALLOWASAVE);
            ROUTE_EXPORT(SavesLeft, GAME_SAVESLEFT);
            ROUTE_EXPORT(GetGameStats, GAME_GETGAMESTATS);
            ROUTE_EXPORT(UpdateInven, GAME_UPDATEINVEN);
            ROUTE_EXPORT(GetDMGameName, GAME_GETDMGAMENAME);
            ROUTE_EXPORT(GetCinematicFreeze, GAME_GETCINEMATICFREEZE);
            ROUTE_EXPORT(SetCinematicFreeze, GAME_SETCINEMATICFREEZE);
            ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);

            // handle cmds for variables, this is how a plugin would get these values if needed
            ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
            ROUTE_EXPORT_VAR(edicts, GAMEVP_EDICTS);
            ROUTE_EXPORT_VAR(edict_size, GAMEV_EDICT_SIZE);
            ROUTE_EXPORT_VAR(num_edicts, GAMEV_NUM_EDICTS);
            ROUTE_EXPORT_VAR(max_edicts, GAMEV_MAX_EDICTS);
            
            // register "qmm" cvar with a command callback to fake GAME_CONSOLE_COMMAND
            case GAME_INIT:
                orig_export->Init();
                // create a "qmm" cvar with its command callback sending a GAME_CONSOLE_COMMAND signal
                (void)orig_import.cvar("qmm", "", 0, cvar_cmd_callback);
                break;
            // handle special cmds which QMM uses but SOF doesn't have an analogue for
            case GAME_CONSOLE_COMMAND:
                // empty handler after plugins pre hooks called, don't send to game
                break;
            default:
                break;
        };

        // update export vars after returning from the mod
        update_exports();
    }

    QMMLOG(QMM_LOG_TRACE, "QMM") << "SOF_GameSupport::vmMain(" << ModMsgName(cmd) << "(" << cmd << ")) returning " << ret << "\n";

    return ret;
}


void* SOF_GameSupport::Entry(void* import, void*, APIType engine_api) {
    QMMLOG(QMM_LOG_DEBUG, "QMM") << "SOF_GameSupport::Entry(" << import << ") called\n";

    if (engine_api == QMM_API_GETGAMEAPI) {
        // original import struct from engine
        // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
        game_import_t* gi = (game_import_t*)import;
        orig_import = *gi;

        // fill in variables of our hooked import struct to pass to the mod
        qmm_import.isClient = orig_import.isClient;
    }

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "SOF_GameSupport::Entry(" << import << ") returning " << &qmm_export << "\n";

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // the only thing in this struct the engine uses before calling Init is the apiversion
    return &qmm_export;
}


bool SOF_GameSupport::ModLoad(void* entry, APIType mod_api) {
    if (mod_api != QMM_API_GETGAMEAPI)
        return false;

    mod_GetGameAPI pfnGGA = (mod_GetGameAPI)entry;
    orig_export = (game_export_t*)pfnGGA(&qmm_import, nullptr);

    return !!orig_export;
}


void SOF_GameSupport::ModUnload(APIType) {
    orig_export = nullptr;
    command_args.clear();
    configstrings.clear();
    entity_tokens.clear();
    token_counter = 0;
    userinfos.clear();
    file_lengths.clear();
    file_pos.clear();
}


const char* SOF_GameSupport::EngMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_MODELINDEX);
        GEN_CASE(G_SOUNDINDEX);
        GEN_CASE(G_EFFECTINDEX);
        GEN_CASE(G_IMAGEINDEX);
        GEN_CASE(G_UNLOAD_SOUND);
        GEN_CASE(G_FILTERPACKET);
        GEN_CASE(G_CREATEGHOULCONFIGSTRINGS);
        GEN_CASE(G_SETMODEL);
        GEN_CASE(G_SETRENDERMODEL);
        GEN_CASE(G_ARGC);
        GEN_CASE(G_ARGV);
        GEN_CASE(G_ARGS);
        GEN_CASE(G_BPRINTF);
        GEN_CASE(G_DPRINTF);
        GEN_CASE(G_CPRINTF);
        GEN_CASE(G_CLPRINTF);
        GEN_CASE(G_WELCOMEPRINT);
        GEN_CASE(G_CENTERPRINTF);
        GEN_CASE(G_CINPRINTF);
        GEN_CASE(G_BCAPTION);
        GEN_CASE(G_CAPTIONPRINTF);
        GEN_CASE(G_CON_CLEARNOTIFY);
        GEN_CASE(G_SOUND);
        GEN_CASE(G_POSITIONED_SOUND);
        GEN_CASE(G_DEBUGGRAPH);
        GEN_CASE(G_DAMAGETEXTURE);
        GEN_CASE(G_SURFACETYPELIST);
        GEN_CASE(G_UPDATE);
        GEN_CASE(G_MULTICAST);
        GEN_CASE(G_MULTICASTIGNORE);
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
        GEN_CASE(G_WRITEBYTE_SIZEBUF);
        GEN_CASE(G_WRITESHORT_SIZEBUF);
        GEN_CASE(G_WRITELONG_SIZEBUF);
        GEN_CASE(G_RELIABLE_WRITEBYTE_TOCLIENT);
        GEN_CASE(G_RELIABLE_WRITEDATA_TOCLIENT);
        GEN_CASE(G_GETNEARESTBYTENORMAL);
        GEN_CASE(G_SENDPLAYERNAMECOLORS);
        GEN_CASE(G_SP_REGISTER);
        GEN_CASE(G_SP_PRINT);
        GEN_CASE(G_SP_PRINT_OBIT);
        GEN_CASE(G_SP_SPRINT);
        GEN_CASE(G_SP_GETSTRINGTEXT);
        GEN_CASE(G_TRACE);
        GEN_CASE(G_POLYTRACE);
        GEN_CASE(G_POINT_CONTENTS);
        GEN_CASE(G_REGIONDISTANCE);
        GEN_CASE(G_IN_PVS);
        GEN_CASE(G_IN_PHS);
        GEN_CASE(G_SETAREAPORTALSTATE);
        GEN_CASE(G_AREAS_CONNECTED);
        GEN_CASE(G_GETGHOUL);
        GEN_CASE(G_NEWPLAYERMODELINFO);
        GEN_CASE(G_FINDGSQFILE);
        GEN_CASE(G_READGSQENTRY);
        GEN_CASE(G_PRECACHEGSQFILE);
        GEN_CASE(G_REGISTERGSQSEQUENCES);
        GEN_CASE(G_TURNOFFPARTSFROMGSQFILE);
        GEN_CASE(GVP_ISCLIENT);
        GEN_CASE(G_CONFIGSTRING);
        GEN_CASE(G_SZ_INIT);
        GEN_CASE(G_SZ_CLEAR);
        GEN_CASE(G_SZ_WRITE);
        GEN_CASE(G_ERROR);
        GEN_CASE(G_SYS_CONSOLEOUTPUT);
        GEN_CASE(G_SYS_GETPLAYERAPI);
        GEN_CASE(G_SYS_UNLOADPLAYER);
        GEN_CASE(G_FLRAND);
        GEN_CASE(G_IRAND);
        GEN_CASE(G_LINKENTITY);
        GEN_CASE(G_UNLINKENTITY);
        GEN_CASE(G_BOXEDICTS);
        GEN_CASE(G_PMOVE);
        GEN_CASE(G_TAGMALLOC);
        GEN_CASE(G_TAGFREE);
        GEN_CASE(G_FREETAGS);
        GEN_CASE(G_APPENDTOSAVEGAME);
        GEN_CASE(G_READFROMSAVEGAME);
        GEN_CASE(G_CVAR);
        GEN_CASE(G_CVAR_SET);
        GEN_CASE(G_CVAR_SETVALUE);
        GEN_CASE(G_CVAR_FORCESET);
        GEN_CASE(G_CVAR_INFO);
        GEN_CASE(G_CVAR_VARIABLEVALUE);
        GEN_CASE(G_FS_LOADFILE);
        GEN_CASE(G_FS_FREEFILE);
        GEN_CASE(G_FS_USERDIR);
        GEN_CASE(G_FS_CREATEPATH);
        GEN_CASE(G_FS_FILEEXISTS);
        GEN_CASE(G_ADDCOMMANDSTRING);

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
        GEN_CASE(G_MILLISECONDS);

        default:
            return "unknown";
    }
}


const char* SOF_GameSupport::ModMsgName(intptr_t cmd) {
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
        GEN_CASE(GAME_RESETCTFTEAM);
        GEN_CASE(GAME_GAMEALLOWASAVE);
        GEN_CASE(GAME_SAVESLEFT);
        GEN_CASE(GAME_GETGAMESTATS);
        GEN_CASE(GAME_UPDATEINVEN);
        GEN_CASE(GAME_GETDMGAMENAME);
        GEN_CASE(GAME_GETCINEMATICFREEZE);
        GEN_CASE(GAME_SETCINEMATICFREEZE);
        GEN_CASE(GAME_RUN_FRAME);

        GEN_CASE(GAMEVP_EDICTS);
        GEN_CASE(GAMEV_EDICT_SIZE);
        GEN_CASE(GAMEV_NUM_EDICTS);
        GEN_CASE(GAMEV_MAX_EDICTS);

        default:
            return "unknown";
    }
}


void SOF_GameSupport::update_exports() {
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
        intptr_t args[] = { (intptr_t)qmm_export.edicts, qmm_export.num_edicts, qmm_export.edict_size, (intptr_t)nullptr, 0 };
        (void)QMM::syscall_args(G_LOCATE_GAME_DATA, args);
    }
}


void SOF_GameSupport::cvar_cmd_callback(cvar_t* cvar) {
    store_args(cvar->name, cvar->string);
    (void)QMM::vmMain_args(GAME_CONSOLE_COMMAND, nullptr);
    command_args.clear();
    orig_import.cvar_set(cvar->name, "");
}


// track args for "qmm" cvar to be retrieved by G_ARGC/G_ARGV/G_ARGS
std::vector<std::string> SOF_GameSupport::command_args;
void SOF_GameSupport::store_args(std::string cvarname, std::string cvarvalue) {
    command_args.push_back(cvarname);

    std::string build = "";

    bool in_quote = false;
    for (auto& c : cvarvalue) {
        if (!in_quote && c == ' ') {
            command_args.push_back(build);
            build.clear();
        }
        else if (!in_quote && c == '\"') {
            in_quote = true;
        }
        else if (in_quote && c == '\"') {
            in_quote = false;
            command_args.push_back(build);
            build.clear();
        }
        else {
            build += c;
        }
    }

    // push remaining arg
    if (!build.empty()) {
        command_args.push_back(build);
        build.clear();
    }
}


// track file info for FS_LoadFile
std::map<fileHandle_t, intptr_t> SOF_GameSupport::file_lengths;
std::map<fileHandle_t, intptr_t> SOF_GameSupport::file_pos;

game_import_t SOF_GameSupport::orig_import;

game_export_t* SOF_GameSupport::orig_export = nullptr;


std::map<int, std::string> SOF_GameSupport::configstrings;
void SOF_GameSupport::configstring(int num, const char* configstring) {
    // if configstring is null, remove entry in map. otherwise store in map
    if (!configstring)
        configstrings.erase(num);
    else
        configstrings[num] = configstring;
    intptr_t args[] = { num, (intptr_t)configstring };
    (void)QMM::syscall_args(G_CONFIGSTRING, args);
}


game_import_t SOF_GameSupport::qmm_import = {
    GEN_IMPORT(modelindex, G_MODELINDEX),
    GEN_IMPORT(soundindex, G_SOUNDINDEX),
    GEN_IMPORT(effectindex, G_EFFECTINDEX),
    GEN_IMPORT(imageindex, G_IMAGEINDEX),
    GEN_IMPORT(unload_sound, G_UNLOAD_SOUND),
    GEN_IMPORT(FilterPacket, G_FILTERPACKET),
    GEN_IMPORT(CreateGhoulConfigStrings, G_CREATEGHOULCONFIGSTRINGS),
    GEN_IMPORT(setmodel, G_SETMODEL),
    GEN_IMPORT(setrendermodel, G_SETRENDERMODEL),
    GEN_IMPORT(argc, G_ARGC),
    GEN_IMPORT(argv, G_ARGV),
    GEN_IMPORT(args, G_ARGS),
    GEN_IMPORT(bprintf, G_BPRINTF),
    GEN_IMPORT(dprintf, G_DPRINTF),
    GEN_IMPORT(cprintf, G_CPRINTF),
    GEN_IMPORT(clprintf, G_CLPRINTF),
    GEN_IMPORT(welcomeprint, G_WELCOMEPRINT),
    GEN_IMPORT(centerprintf, G_CENTERPRINTF),
    GEN_IMPORT(cinprintf, G_CINPRINTF),
    GEN_IMPORT(bcaption, G_BCAPTION),
    GEN_IMPORT(captionprintf, G_CAPTIONPRINTF),
    GEN_IMPORT(Con_ClearNotify, G_CON_CLEARNOTIFY),
    GEN_IMPORT_7(sound, G_SOUND, void, edict_t*, int, int, float, float, float, int),
    GEN_IMPORT_8(positioned_sound, G_POSITIONED_SOUND, void, float*, edict_t*, int, int, float, float, float, int),
    GEN_IMPORT_2(DebugGraph, G_DEBUGGRAPH, void, float, int),
    GEN_IMPORT(DamageTexture, G_DAMAGETEXTURE),
    GEN_IMPORT(SurfaceTypeList, G_SURFACETYPELIST),
    GEN_IMPORT_2(Update, G_UPDATE, void, float, bool),
    GEN_IMPORT(multicast, G_MULTICAST),
    GEN_IMPORT(multicastignore, G_MULTICASTIGNORE),
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
    GEN_IMPORT(WriteByteSizebuf, G_WRITEBYTE_SIZEBUF),
    GEN_IMPORT(WriteShortSizebuf, G_WRITESHORT_SIZEBUF),
    GEN_IMPORT(WriteLongSizebuf, G_WRITELONG_SIZEBUF),
    GEN_IMPORT(ReliableWriteByteToClient, G_RELIABLE_WRITEBYTE_TOCLIENT),
    GEN_IMPORT(ReliableWriteDataToClient, G_RELIABLE_WRITEDATA_TOCLIENT),
    GEN_IMPORT(GetNearestByteNormal, G_GETNEARESTBYTENORMAL),
    GEN_IMPORT(sendPlayernameColors, G_SENDPLAYERNAMECOLORS),
    GEN_IMPORT(SP_Register, G_SP_REGISTER),
    GEN_IMPORT(SP_Print, G_SP_PRINT),
    GEN_IMPORT(SP_Print_Obit, G_SP_PRINT_OBIT),
    GEN_IMPORT(SP_SPrint, G_SP_SPRINT),
    GEN_IMPORT(SP_GetStringText, G_SP_GETSTRINGTEXT),
    GEN_IMPORT(trace, G_TRACE),
    GEN_IMPORT(polyTrace, G_POLYTRACE),
    GEN_IMPORT(pointcontents, G_POINT_CONTENTS),
    GEN_IMPORT(RegionDistance, G_REGIONDISTANCE),
    GEN_IMPORT(inPVS, G_IN_PVS),
    GEN_IMPORT(inPHS, G_IN_PHS),
    GEN_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE),
    GEN_IMPORT(AreasConnected, G_AREAS_CONNECTED),
    GEN_IMPORT(GetGhoul, G_GETGHOUL),
    GEN_IMPORT(NewPlayerModelInfo, G_NEWPLAYERMODELINFO),
    GEN_IMPORT(FindGSQFile, G_FINDGSQFILE),
    GEN_IMPORT(ReadGsqEntry, G_READGSQENTRY),
    GEN_IMPORT(PrecacheGSQFile, G_PRECACHEGSQFILE),
    GEN_IMPORT(RegisterGSQSequences, G_REGISTERGSQSEQUENCES),
    GEN_IMPORT(TurnOffPartsFromGSQFile, G_TURNOFFPARTSFROMGSQFILE),
    nullptr,    // isClient
    GEN_IMPORT(configstring, G_CONFIGSTRING),
    GEN_IMPORT(SZ_Init, G_SZ_INIT),
    GEN_IMPORT(SZ_Clear, G_SZ_CLEAR),
    GEN_IMPORT(SZ_Write, G_SZ_WRITE),
    GEN_IMPORT(error, G_ERROR),
    GEN_IMPORT(Sys_ConsoleOutput, G_SYS_CONSOLEOUTPUT),
    GEN_IMPORT(Sys_GetPlayerAPI, G_SYS_GETPLAYERAPI),
    GEN_IMPORT(Sys_UnloadPlayer, G_SYS_UNLOADPLAYER),
    GEN_IMPORT_2(flrand, G_FLRAND, float, float, float),
    GEN_IMPORT(irand, G_IRAND),
    GEN_IMPORT(linkentity, G_LINKENTITY),
    GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
    GEN_IMPORT(BoxEdicts, G_BOXEDICTS),
    GEN_IMPORT(Pmove, G_PMOVE),
    GEN_IMPORT(TagMalloc, G_TAGMALLOC),
    GEN_IMPORT(TagFree, G_TAGFREE),
    GEN_IMPORT(FreeTags, G_FREETAGS),
    GEN_IMPORT(AppendToSavegame, G_APPENDTOSAVEGAME),
    GEN_IMPORT(ReadFromSavegame, G_READFROMSAVEGAME),
    GEN_IMPORT(cvar, G_CVAR),
    GEN_IMPORT(cvar_set, G_CVAR_SET),
    GEN_IMPORT_2(cvar_setvalue, G_CVAR_SETVALUE, void, const char*, float),
    GEN_IMPORT(cvar_forceset, G_CVAR_FORCESET),
    GEN_IMPORT(cvar_info, G_CVAR_INFO),
    GEN_IMPORT_1_F(cvar_variablevalue, G_CVAR_VARIABLEVALUE, const char*),
    GEN_IMPORT(FS_LoadFile, G_FS_LOADFILE),
    GEN_IMPORT(FS_FreeFile, G_FS_FREEFILE),
    GEN_IMPORT(FS_Userdir, G_FS_USERDIR),
    GEN_IMPORT(FS_CreatePath, G_FS_CREATEPATH),
    GEN_IMPORT(FS_FileExists, G_FS_FILEEXISTS),
    GEN_IMPORT(AddCommandString, G_ADDCOMMANDSTRING),
};


// track userinfo for our G_GET_USERINFO syscall
std::map<intptr_t, std::string> SOF_GameSupport::userinfos;
qboolean SOF_GameSupport::ClientConnect(edict_t* ent, char* userinfo) {
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
}


void SOF_GameSupport::ClientUserinfoChanged(edict_t* ent, char* userinfo, bool not_first_time) {
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
    intptr_t args[] = { (intptr_t)ent, (intptr_t)userinfo, not_first_time };
    (void)QMM::vmMain_args(GAME_CLIENT_USERINFO_CHANGED, args);
}


// track entstrings for our G_GET_ENTITY_TOKEN syscall
std::vector<std::string> SOF_GameSupport::entity_tokens;
size_t SOF_GameSupport::token_counter = 0;
void SOF_GameSupport::SpawnEntities(char* mapname, char* entstring, char* spawnpoint) {
    if (entstring) {
        entity_tokens = Util::util_parse_entstring(entstring);
        token_counter = 0;
    }
    intptr_t args[] = { (intptr_t)mapname, (intptr_t)entstring, (intptr_t)spawnpoint };
    (void)QMM::vmMain_args(GAME_SPAWN_ENTITIES, args);
}


// struct with lambdas that call QMM's vmMain function. this is given to the game engine
game_export_t SOF_GameSupport::qmm_export = {
    GAME_API_VERSION,    // apiversion
    GEN_EXPORT(Init, GAME_INIT),
    GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
    SOF_GameSupport::SpawnEntities,
    GEN_EXPORT(WriteGame, GAME_WRITE_GAME),
    GEN_EXPORT(ReadGame, GAME_READ_GAME),
    GEN_EXPORT(WriteLevel, GAME_WRITE_LEVEL),
    GEN_EXPORT(ReadLevel, GAME_READ_LEVEL),
    SOF_GameSupport::ClientConnect,
    GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
    SOF_GameSupport::ClientUserinfoChanged,
    GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
    GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
    GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
    GEN_EXPORT(ResetCTFTeam, GAME_RESETCTFTEAM),
    GEN_EXPORT(GameAllowASave, GAME_GAMEALLOWASAVE),
    GEN_EXPORT(SavesLeft, GAME_SAVESLEFT),
    GEN_EXPORT(GetGameStats, GAME_GETGAMESTATS),
    GEN_EXPORT(UpdateInven, GAME_UPDATEINVEN),
    GEN_EXPORT(GetDMGameName, GAME_GETDMGAMENAME),
    GEN_EXPORT(GetCinematicFreeze, GAME_GETCINEMATICFREEZE),
    GEN_EXPORT(SetCinematicFreeze, GAME_SETCINEMATICFREEZE),
    GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
    // the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
    nullptr,    // edicts
    0,          // edict_size
    0,          // num_edicts
    0,          // max_edicts

};

#endif // QMM_ARCH_32
