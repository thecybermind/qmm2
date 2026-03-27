/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "version.h"

// AFAIK Quake 2 Remastered is only available on 64-bit Windows, so skip the whole file otherwise.
// The game entry in game_api is similarly conditionally compiled.
#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)

#define _CRT_SECURE_NO_WARNINGS 1
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <q2r/rerelease/game.h>
#include "game_api.hpp"
#include "log.hpp"
#include "format.hpp"
// QMM-specific Q2R header
#include "game_q2r.h"
#include "gameinfo.hpp"
#include "main.hpp"
#include "util.hpp"

struct Q2R_GameSupport : public GameSupport {
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

    virtual const char* DefaultDLLName() { return "game_" X64_DLL; }
    virtual const char* DefaultModDir() { return "baseq2"; }
    virtual const char* ModCvar() { return "game"; }
    virtual const char* GameName() { return "Quake 2 Remastered"; }
    virtual const char* GameCode() { return "Q2R"; }

private:
    // update the export variables from orig_export
    static void update_exports();

    // track configstrings for our G_GET_CONFIGSTRING syscall
    static std::map<int, std::string> configstrings;
    static void configstring(int num, const char* configstring);

    // track userinfo for our G_GET_USERINFO syscall
    static std::map<intptr_t, std::string> userinfos;
    static bool ClientConnect(edict_t* ent, char* userinfo, const char* social_id, bool isBot);
    static void ClientUserinfoChanged(edict_t* ent, const char* userinfo);

    // track entstrings for our G_GET_ENTITY_TOKEN syscall
    static std::vector<std::string> entity_tokens;
    static size_t token_counter;
    static void SpawnEntities(const char* mapname, const char* entstring, const char* spawnpoint);

    // a copy of the original import struct that comes from the game engine
    static game_import_t orig_import;

    // a copy of the original export struct pointer that comes from the mod
    static game_export_t* orig_export;

    // struct with lambdas that call QMM's syscall function. this is given to the mod
    static game_import_t qmm_import;

    // struct with lambdas that call QMM's vmMain function. this is given to the game engine
    static game_export_t qmm_export;

    const int qmm_eng_msgs[QMM_ENGINE_MSG_COUNT] = GEN_GAME_QMM_ENG_MSGS();
    // GAME_PREINIT gets called first, which is when QMM has to perform mod/plugin loading, but we
    // don't want to make plugins have to use separate code to handle the actual GAME_INIT message
    const int qmm_mod_msgs[QMM_MOD_MSG_COUNT] = { GAME_PREINIT, GAME_SHUTDOWN, GAME_CONSOLE_COMMAND, };
};

GEN_GAME_OBJ(Q2R);


// auto-detection logic for Q2R
bool Q2R_GameSupport::AutoDetect(APIType engineapi) {
    if (engineapi != QMM_API_GETGAMEAPI)
        return false;

    if (!str_striequal(gameinfo.qmm_file, DefaultDLLName()))
        return false;

    if (!str_stristr(gameinfo.exe_file, "quake2ex"))
        return false;

    return true;
}


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t Q2R_GameSupport::syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q2R_GameSupport::syscall({} {}) called\n", EngMsgName(cmd), cmd);
#endif

    // update export vars before calling into the engine
    update_exports();

    intptr_t ret = 0;

    switch (cmd) {
        ROUTE_IMPORT(Broadcast_Print, G_BROADCAST_PRINT);
        ROUTE_IMPORT(Com_Print, G_COM_PRINT);
        ROUTE_IMPORT(Client_Print, G_CLIENT_PRINT);
        ROUTE_IMPORT(Center_Print, G_CENTERPRINT);
        ROUTE_IMPORT_6_V(sound, G_SOUND, (edict_t*), (soundchan_t), (int), *(float*)&, *(float*)&, *(float*)&);
        ROUTE_IMPORT_7_V(positioned_sound, G_POSITIONED_SOUND, (float*), (edict_t*), (soundchan_t), (int), *(float*)&, *(float*)&, *(float*)&);
        ROUTE_IMPORT_9_V(local_sound, G_LOCAL_SOUND, (edict_t*), (float*), (edict_t*), (soundchan_t), (int), *(float*)&, *(float*)&, *(float*)&, (uint32_t));
        ROUTE_IMPORT(configstring, G_CONFIGSTRING);
        ROUTE_IMPORT(get_configstring, G_GET_CONFIGSTRING);
        ROUTE_IMPORT(Com_Error, G_COM_ERROR);
        ROUTE_IMPORT(modelindex, G_MODELINDEX);
        ROUTE_IMPORT(soundindex, G_SOUNDINDEX);
        ROUTE_IMPORT(imageindex, G_IMAGEINDEX);
        ROUTE_IMPORT(setmodel, G_SETMODEL);
        ROUTE_IMPORT(trace, G_TRACE);
        ROUTE_IMPORT(clip, G_CLIP);
        ROUTE_IMPORT(pointcontents, G_POINT_CONTENTS);
        ROUTE_IMPORT(inPVS, G_IN_PVS);
        ROUTE_IMPORT(inPHS, G_IN_PHS);
        ROUTE_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE);
        ROUTE_IMPORT(AreasConnected, G_AREAS_CONNECTED);
        ROUTE_IMPORT(linkentity, G_LINKENTITY);
        ROUTE_IMPORT(unlinkentity, G_UNLINKENTITY);
        ROUTE_IMPORT(BoxEdicts, G_BOXEDICTS);
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
        ROUTE_IMPORT(WriteEntity, G_MSG_WRITEENTITY);
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
        ROUTE_IMPORT(GetExtension, G_GET_EXTENSION);
        ROUTE_IMPORT(Bot_RegisterEdict, G_BOT_REGISTEREDICT);
        ROUTE_IMPORT(Bot_UnRegisterEdict, G_BOT_UNREGISTEREDICT);
        ROUTE_IMPORT_3(Bot_MoveToPoint, G_BOT_MOVETOPOINT, (const edict_t*), (float*), *(float*)&);
        ROUTE_IMPORT(Bot_FollowActor, G_BOT_FOLLOWACTOR);
        ROUTE_IMPORT(GetPathToGoal, G_GETPATHTOGOAL);
        ROUTE_IMPORT(Loc_Print, G_LOC_PRINT);
        ROUTE_IMPORT_5_V(Draw_Line, G_DRAW_LINE, (float*), (float*), (const rgba_t&), *(float*)&, (bool));
        ROUTE_IMPORT_5_V(Draw_Point, G_DRAW_POINT, (float*), *(float*)&, (const rgba_t&), *(float*)&, (bool));
        ROUTE_IMPORT_5_V(Draw_Circle, G_DRAW_CIRCLE, (float*), *(float*)&, (const rgba_t&), *(float*)&, (bool));
        ROUTE_IMPORT_5_V(Draw_Bounds, G_DRAW_BOUNDS, (float*), (float*), (const rgba_t&), *(float*)&, (bool));
        ROUTE_IMPORT_5_V(Draw_Sphere, G_DRAW_SPHERE, (float*), *(float*)&, (const rgba_t&), *(float*)&, (bool));
        ROUTE_IMPORT_6_V(Draw_OrientedWorldText, G_DRAW_ORIENTEDWORLDTEXT, (float*), (const char*), (const rgba_t&), *(float*)&, *(float*)&, (bool));
        ROUTE_IMPORT_7_V(Draw_StaticWorldText, G_DRAW_STATICWORLDTEXT, (float*), (float*), (const char*), (const rgba_t&), *(float*)&, *(float*)&, (bool));
        ROUTE_IMPORT_6_V(Draw_Cylinder, G_DRAW_CYLINDER, (float*), *(float*)&, *(float*)&, (const rgba_t&), *(float*)&, (bool));
        ROUTE_IMPORT_7_V(Draw_Ray, G_DRAW_RAY, (float*), (float*), *(float*)&, *(float*)&, (const rgba_t&), *(float*)&, (bool));
        ROUTE_IMPORT_7_V(Draw_Arrow, G_DRAW_ARROW, (float*), (float*), *(float*)&, (const rgba_t&), (const rgba_t&), *(float*)&, (bool));
        ROUTE_IMPORT(ReportMatchDetails_Multicast, G_REPORTMATCHDETAILS_MULTICAST);
        ROUTE_IMPORT(ServerFrame, G_SERVER_FRAME);
        ROUTE_IMPORT(SendToClipBoard, G_SENDTOCLIPBOARD);
        ROUTE_IMPORT(Info_ValueForKey, G_INFO_VALUEFORKEY);
        ROUTE_IMPORT(Info_RemoveKey, G_INFO_REMOVEKEY);
        ROUTE_IMPORT(Info_SetValueForKey, G_INFO_SETVALUEFORKEY);

        // handle cmds for variables, this is how a plugin would get these values if needed
        ROUTE_IMPORT_VAR(tick_rate, GV_TICK_RATE);
        ROUTE_IMPORT_VAR(frame_time_s, GV_FRAME_TIME_S);
        ROUTE_IMPORT_VAR(frame_time_ms, GV_FRAME_TIME_MS);

        // handle special cmds which QMM uses but Q2R doesn't have an analogue for
    case G_CVAR_REGISTER: {
        // q2r: cvar_t *(*cvar) (char *var_name, char *value, int flags);
        // qmm: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
        // qmm always passes NULL for vmCvar so don't worry about it
        char* var_name = (char*)(args[1]);
        char* value = (char*)(args[2]);
        cvar_flags_t flags = (cvar_flags_t)args[3];
        (void)orig_import.cvar(var_name, value, flags);
        break;
    }
    case G_CVAR_VARIABLE_STRING_BUFFER: {
        // q2r: cvar_t *(*cvar) (char *var_name, char *value, int flags);
        // qmm: void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
        char* var_name = (char*)(args[0]);
        char* buffer = (char*)(args[1]);
        intptr_t bufsize = args[2];
        *buffer = '\0';
        cvar_t* cvar = orig_import.cvar(var_name, (char*)"", CVAR_NOFLAGS);
        if (cvar)
            strncpyz(buffer, cvar->string, (size_t)bufsize);
        break;
    }
    case G_CVAR_VARIABLE_INTEGER_VALUE: {
        // q2r: cvar_t *(*cvar) (char *var_name, char *value, int flags);
        // qmm: int trap_Cvar_VariableIntegerValue(const char* var_name)
        char* var_name = (char*)(args[0]);
        cvar_t* cvar = orig_import.cvar(var_name, (char*)"", CVAR_NOFLAGS);
        if (cvar)
            ret = cvar->integer;
        break;
    }
    case G_SEND_CONSOLE_COMMAND: {
        // Q2R: void (*AddCommandString)(const char *text);
        // qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
        const char* text = (const char*)(args[1]);
        orig_import.AddCommandString(text);
        break;
    }
    // provide these to plugins just so the most basic file functions all work. use FILE* for these
    case G_FS_FOPEN_FILE: {
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
    // help plugins not need separate logic for entity/client pointers
    case G_LOCATE_GAME_DATA: {
        // void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient);
        // this is just to be hooked by plugins, so ignore everything
        break;
    }
    case G_DROP_CLIENT: {
        // void trap_DropClient(int clientNum, const char *reason);
        intptr_t clientnum = args[0];
        orig_import.AddCommandString(fmt::format("kick {}\n", clientnum).c_str());
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

    default:
        break;
    };

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q2R_GameSupport::syscall({} {}) returning {}\n", EngMsgName(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t Q2R_GameSupport::vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q2R_GameSupport::vmMain({} {}) called\n", ModMsgName(cmd), cmd);
#endif

    if (!orig_export)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;
    switch (cmd) {
        ROUTE_EXPORT(PreInit, GAME_PREINIT);
        ROUTE_EXPORT(Init, GAME_INIT);
        ROUTE_EXPORT(Shutdown, GAME_SHUTDOWN);
        ROUTE_EXPORT(SpawnEntities, GAME_SPAWN_ENTITIES);
        ROUTE_EXPORT(WriteGameJson, GAME_WRITE_GAME);
        ROUTE_EXPORT(ReadGameJson, GAME_READ_GAME);
        ROUTE_EXPORT(WriteLevelJson, GAME_WRITE_LEVEL);
        ROUTE_EXPORT(ReadLevelJson, GAME_READ_LEVEL);
        ROUTE_EXPORT(CanSave, GAME_CAN_SAVE);
        ROUTE_EXPORT(ClientChooseSlot, GAME_CLIENT_CHOOSESLOT);
        ROUTE_EXPORT(ClientConnect, GAME_CLIENT_CONNECT);
        ROUTE_EXPORT(ClientBegin, GAME_CLIENT_BEGIN);
        ROUTE_EXPORT(ClientUserinfoChanged, GAME_CLIENT_USERINFO_CHANGED);
        ROUTE_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT);
        ROUTE_EXPORT(ClientCommand, GAME_CLIENT_COMMAND);
        ROUTE_EXPORT(ClientThink, GAME_CLIENT_THINK);
        ROUTE_EXPORT(RunFrame, GAME_RUN_FRAME);
        ROUTE_EXPORT(PrepFrame, GAME_PREP_FRAME);
        ROUTE_EXPORT(ServerCommand, GAME_SERVER_COMMAND);
        ROUTE_EXPORT(Pmove, GAME_PMOVE);
        ROUTE_EXPORT(GetExtension, GAME_GET_EXTENSION);
        ROUTE_EXPORT(Bot_SetWeapon, GAME_BOT_SETWEAPON);
        ROUTE_EXPORT(Bot_TriggerEdict, GAME_BOT_TRIGGEREDICT);
        ROUTE_EXPORT(Bot_UseItem, GAME_BOT_USEITEM);
        ROUTE_EXPORT(Bot_GetItemID, GAME_BOT_GETITEMID);
        ROUTE_EXPORT(Edict_ForceLookAtPoint, GAME_EDICT_FORCELOOKATPOINT);
        ROUTE_EXPORT(Bot_PickedUpItem, GAME_BOT_PICKEDUPITEM);
        ROUTE_EXPORT(Entity_IsVisibleToPlayer, GAME_ENTITY_ISVISIBLETOPLAYER);
        ROUTE_EXPORT(GetShadowLightData, GAME_GETSHADOWLIGHTDATA);

        // handle cmds for variables, this is how a plugin would get these values if needed
        ROUTE_EXPORT_VAR(apiversion, GAMEV_APIVERSION);
        ROUTE_EXPORT_VAR(edicts, GAMEVP_EDICTS);
        ROUTE_EXPORT_VAR(edict_size, GAMEV_EDICT_SIZE);
        ROUTE_EXPORT_VAR(num_edicts, GAMEV_NUM_EDICTS);
        ROUTE_EXPORT_VAR(max_edicts, GAMEV_MAX_EDICTS);
        ROUTE_EXPORT_VAR(server_flags, GAMEV_SERVER_FLAGS);

    default:
        break;
    };

    // update export vars after returning from the mod
    update_exports();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q2R_GameSupport::vmMain({} {}) returning {}\n", ModMsgName(cmd), cmd, ret);
#endif

    return ret;
}


void* Q2R_GameSupport::Entry(void* import, void*, APIType) {
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q2R_GameSupport::Entry({}) called\n", import);

    // original import struct from engine
    // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
    game_import_t* gi = (game_import_t*)import;
    orig_import = *gi;

    // fill in variables of our hooked import struct to pass to the mod
    qmm_import.tick_rate = orig_import.tick_rate;
    qmm_import.frame_time_s = orig_import.frame_time_s;
    qmm_import.frame_time_ms = orig_import.frame_time_ms;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Q2R_GameSupport::Entry({}) returning {}\n", import, fmt::ptr(&qmm_export));

    // struct full of export lambdas to QMM's vmMain
    // this gets returned to the game engine, but we haven't loaded the mod yet.
    // the only thing in this struct the engine uses before calling Init is the apiversion
    return &qmm_export;
}


bool Q2R_GameSupport::ModLoad(void* entry, APIType modapi) {
    if (modapi != QMM_API_GETGAMEAPI)
        return false;

    mod_GetGameAPI pfnGGA = (mod_GetGameAPI)entry;
    orig_export = (game_export_t*)pfnGGA(&qmm_import, nullptr);

    return !!orig_export;
}


void Q2R_GameSupport::ModUnload() {
    orig_export = nullptr;
}


const char* Q2R_GameSupport::EngMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_BROADCAST_PRINT);
        GEN_CASE(G_COM_PRINT);
        GEN_CASE(G_CLIENT_PRINT);
        GEN_CASE(G_CENTERPRINT);
        GEN_CASE(G_SOUND);
        GEN_CASE(G_POSITIONED_SOUND);
        GEN_CASE(G_LOCAL_SOUND);
        GEN_CASE(G_CONFIGSTRING);
        GEN_CASE(G_GET_CONFIGSTRING);
        GEN_CASE(G_COM_ERROR);
        GEN_CASE(G_MODELINDEX);
        GEN_CASE(G_SOUNDINDEX);
        GEN_CASE(G_IMAGEINDEX);
        GEN_CASE(G_SETMODEL);
        GEN_CASE(G_TRACE);
        GEN_CASE(G_CLIP);
        GEN_CASE(G_POINT_CONTENTS);
        GEN_CASE(G_IN_PVS);
        GEN_CASE(G_IN_PHS);
        GEN_CASE(G_SETAREAPORTALSTATE);
        GEN_CASE(G_AREAS_CONNECTED);
        GEN_CASE(G_LINKENTITY);
        GEN_CASE(G_UNLINKENTITY);
        GEN_CASE(G_BOXEDICTS);
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
        GEN_CASE(G_MSG_WRITEENTITY);
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
        GEN_CASE(G_GET_EXTENSION);
        GEN_CASE(G_BOT_REGISTEREDICT);
        GEN_CASE(G_BOT_UNREGISTEREDICT);
        GEN_CASE(G_BOT_MOVETOPOINT);
        GEN_CASE(G_BOT_FOLLOWACTOR);
        GEN_CASE(G_GETPATHTOGOAL);
        GEN_CASE(G_LOC_PRINT);
        GEN_CASE(G_DRAW_LINE);
        GEN_CASE(G_DRAW_POINT);
        GEN_CASE(G_DRAW_CIRCLE);
        GEN_CASE(G_DRAW_BOUNDS);
        GEN_CASE(G_DRAW_SPHERE);
        GEN_CASE(G_DRAW_ORIENTEDWORLDTEXT);
        GEN_CASE(G_DRAW_STATICWORLDTEXT);
        GEN_CASE(G_DRAW_CYLINDER);
        GEN_CASE(G_DRAW_RAY);
        GEN_CASE(G_DRAW_ARROW);
        GEN_CASE(G_REPORTMATCHDETAILS_MULTICAST);
        GEN_CASE(G_SERVER_FRAME);
        GEN_CASE(G_SENDTOCLIPBOARD);
        GEN_CASE(G_INFO_VALUEFORKEY);
        GEN_CASE(G_INFO_REMOVEKEY);
        GEN_CASE(G_INFO_SETVALUEFORKEY);

        // polyfills
        GEN_CASE(G_CVAR_REGISTER);
        GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
        GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(G_SEND_CONSOLE_COMMAND);

        GEN_CASE(G_FS_FOPEN_FILE);
        GEN_CASE(G_FS_READ);
        GEN_CASE(G_FS_WRITE);
        GEN_CASE(G_FS_FCLOSE_FILE);

        GEN_CASE(G_LOCATE_GAME_DATA);
        GEN_CASE(G_DROP_CLIENT);
        GEN_CASE(G_GET_USERINFO);
        GEN_CASE(G_GET_ENTITY_TOKEN);

    default:
        return "unknown";
    }
}


const char* Q2R_GameSupport::ModMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(GAMEV_APIVERSION);
        GEN_CASE(GAME_PREINIT);
        GEN_CASE(GAME_INIT);
        GEN_CASE(GAME_SHUTDOWN);
        GEN_CASE(GAME_SPAWN_ENTITIES);
        GEN_CASE(GAME_WRITE_GAME);
        GEN_CASE(GAME_READ_GAME);
        GEN_CASE(GAME_WRITE_LEVEL);
        GEN_CASE(GAME_READ_LEVEL);
        GEN_CASE(GAME_CAN_SAVE);
        GEN_CASE(GAME_CLIENT_CHOOSESLOT);
        GEN_CASE(GAME_CLIENT_CONNECT);
        GEN_CASE(GAME_CLIENT_BEGIN);
        GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
        GEN_CASE(GAME_CLIENT_DISCONNECT);
        GEN_CASE(GAME_CLIENT_COMMAND);
        GEN_CASE(GAME_CLIENT_THINK);
        GEN_CASE(GAME_RUN_FRAME);
        GEN_CASE(GAME_PREP_FRAME);
        GEN_CASE(GAME_SERVER_COMMAND);
        GEN_CASE(GAMEVP_EDICTS);
        GEN_CASE(GAMEV_EDICT_SIZE);
        GEN_CASE(GAMEV_NUM_EDICTS);
        GEN_CASE(GAMEV_MAX_EDICTS);
        GEN_CASE(GAMEV_SERVER_FLAGS);
        GEN_CASE(GAME_PMOVE);
        GEN_CASE(GAME_GET_EXTENSION);
        GEN_CASE(GAME_BOT_SETWEAPON);
        GEN_CASE(GAME_BOT_TRIGGEREDICT);
        GEN_CASE(GAME_BOT_USEITEM);
        GEN_CASE(GAME_BOT_GETITEMID);
        GEN_CASE(GAME_EDICT_FORCELOOKATPOINT);
        GEN_CASE(GAME_BOT_PICKEDUPITEM);
        GEN_CASE(GAME_ENTITY_ISVISIBLETOPLAYER);
        GEN_CASE(GAME_GETSHADOWLIGHTDATA);

    default:
        return "unknown";
    }
}


void Q2R_GameSupport::update_exports() {
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
    qmm_export.server_flags = orig_export->server_flags;

    if (changed) {
        // this will trigger this message to be fired to plugins, and then it will be handled
        // by the empty "case G_LOCATE_GAME_DATA" in MOHAA_syscall
        qmm_syscall(G_LOCATE_GAME_DATA, (intptr_t)qmm_export.edicts, qmm_export.num_edicts, qmm_export.edict_size, nullptr, 0);
    }
}


game_import_t Q2R_GameSupport::orig_import;


game_export_t* Q2R_GameSupport::orig_export = nullptr;


/*
   The sound() import gets messed up because the volume float arg doesn't get passed properly. Server-generated
   sound (i.e. anything from the world like item pickup or another player attacking) would be silent. After
   debugging, the volume argument would appear to come through as ~1e-19 or something exceptionally small.
   Assigning qmm_import.sound=orig_import.sound would resolve the issue but obviously not allow for plugin hooking.
   turns out the problem is the float arguments and the GEN_IMPORT macro using intptr_t.

   From: https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention#parameter-passing
   "Any floating-point and double-precision arguments in the first four parameters are passed in XMM0 - XMM3,
   depending on position. Floating-point values are only placed in the integer registers RCX, RDX, R8, and R9
   when there are varargs arguments."

   Since the Q2R engine is 64-bit and does not use varargs, floats in the first 4 args don't get put into the
   integer registers. Everything after 4 arguments are passed on the stack in 8-byte-aligned slots. So, we can
   pull the correct arguments with matching prototypes, then pass them to the varargs syscall where they get
   handled properly.
*/
// struct with lambdas that call QMM's syscall function. this is given to the mod
game_import_t Q2R_GameSupport::qmm_import = {
    0, // tick_rate
    0, // frame_time_s
    0, // frame_time_ms
    GEN_IMPORT(Broadcast_Print, G_BROADCAST_PRINT),
    GEN_IMPORT(Com_Print, G_COM_PRINT),
    GEN_IMPORT(Client_Print, G_CLIENT_PRINT),
    GEN_IMPORT(Center_Print, G_CENTERPRINT),
    GEN_IMPORT_6(sound, G_SOUND, void, edict_t*, soundchan_t, int, float, float, float),
    GEN_IMPORT_7(positioned_sound, G_POSITIONED_SOUND, void, gvec3_cref_t, edict_t*, soundchan_t, int, float, float, float),
    GEN_IMPORT_9(local_sound, G_LOCAL_SOUND, void, edict_t*, gvec3_cptr_t, edict_t*, soundchan_t, int, float, float, float, uint32_t),
    GEN_IMPORT(configstring, G_CONFIGSTRING),
    GEN_IMPORT(get_configstring, G_GET_CONFIGSTRING),
    GEN_IMPORT(Com_Error, G_COM_ERROR),
    GEN_IMPORT(modelindex, G_MODELINDEX),
    GEN_IMPORT(soundindex, G_SOUNDINDEX),
    GEN_IMPORT(imageindex, G_IMAGEINDEX),
    GEN_IMPORT(setmodel, G_SETMODEL),
    GEN_IMPORT(trace, G_TRACE),
    GEN_IMPORT(clip, G_CLIP),
    GEN_IMPORT(pointcontents, G_POINT_CONTENTS),
    GEN_IMPORT(inPVS, G_IN_PVS),
    GEN_IMPORT(inPHS, G_IN_PHS),
    GEN_IMPORT(SetAreaPortalState, G_SETAREAPORTALSTATE),
    GEN_IMPORT(AreasConnected, G_AREAS_CONNECTED),
    GEN_IMPORT(linkentity, G_LINKENTITY),
    GEN_IMPORT(unlinkentity, G_UNLINKENTITY),
    GEN_IMPORT(BoxEdicts, G_BOXEDICTS),
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
    GEN_IMPORT(WriteEntity, G_MSG_WRITEENTITY),
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
    GEN_IMPORT(GetExtension, G_GET_EXTENSION),
    GEN_IMPORT(Bot_RegisterEdict, G_BOT_REGISTEREDICT),
    GEN_IMPORT(Bot_UnRegisterEdict, G_BOT_UNREGISTEREDICT),
    GEN_IMPORT_3(Bot_MoveToPoint, G_BOT_MOVETOPOINT, GoalReturnCode, const edict_t*, gvec3_cref_t, const float),
    GEN_IMPORT(Bot_FollowActor, G_BOT_FOLLOWACTOR),
    GEN_IMPORT(GetPathToGoal, G_GETPATHTOGOAL),
    GEN_IMPORT(Loc_Print, G_LOC_PRINT),
    GEN_IMPORT_5(Draw_Line, G_DRAW_LINE, void, gvec3_cref_t, gvec3_cref_t, const rgba_t&, float, bool),
    GEN_IMPORT_5(Draw_Point, G_DRAW_POINT, void, gvec3_cref_t, float, const rgba_t&, float, bool),
    GEN_IMPORT_5(Draw_Circle, G_DRAW_CIRCLE, void, gvec3_cref_t, float, const rgba_t&, float, bool),
    GEN_IMPORT_5(Draw_Bounds, G_DRAW_BOUNDS, void, gvec3_cref_t, gvec3_cref_t, const rgba_t&, float, bool),
    GEN_IMPORT_5(Draw_Sphere, G_DRAW_SPHERE, void, gvec3_cref_t, float, const rgba_t&, float, bool),
    GEN_IMPORT_6(Draw_OrientedWorldText, G_DRAW_ORIENTEDWORLDTEXT, void, gvec3_cref_t, const char*, const rgba_t&, float, float, bool),
    GEN_IMPORT_7(Draw_StaticWorldText, G_DRAW_STATICWORLDTEXT, void, gvec3_cref_t, gvec3_cref_t, const char*, const rgba_t&, const float, const float, const bool),
    GEN_IMPORT_6(Draw_Cylinder, G_DRAW_CYLINDER, void, gvec3_cref_t, float, float, const rgba_t&, float, bool),
    GEN_IMPORT_7(Draw_Ray, G_DRAW_RAY, void, gvec3_cref_t, gvec3_cref_t, float, float, const rgba_t&, float, bool),
    GEN_IMPORT_7(Draw_Arrow, G_DRAW_ARROW, void, gvec3_cref_t, gvec3_cref_t, float, const rgba_t&, const rgba_t&, float, bool),
    GEN_IMPORT(ReportMatchDetails_Multicast, G_REPORTMATCHDETAILS_MULTICAST),
    GEN_IMPORT(ServerFrame, G_SERVER_FRAME),
    GEN_IMPORT(SendToClipBoard, G_SENDTOCLIPBOARD),
    GEN_IMPORT(Info_ValueForKey, G_INFO_VALUEFORKEY),
    GEN_IMPORT(Info_RemoveKey, G_INFO_REMOVEKEY),
    GEN_IMPORT(Info_SetValueForKey, G_INFO_SETVALUEFORKEY),
};


std::map<intptr_t, std::string> Q2R_GameSupport::userinfos;
bool Q2R_GameSupport::ClientConnect(edict_t* ent, char* userinfo, const char* social_id, bool isBot) {
    // get client number (ent->s.number is not set until CLIENT_BEGIN, so calculate based on edict_t*)
    if (orig_export && orig_export->edicts && orig_export->edict_size) {
        intptr_t entnum = (intptr_t)(((intptr_t)ent - (intptr_t)orig_export->edicts) / orig_export->edict_size);
        intptr_t clientnum = entnum - 1;
        // if userinfo is null, remove entry in map. otherwise store in map
        if (!userinfo)
            userinfos.erase(clientnum);
        else
            userinfos[clientnum] = userinfo;
    }
    cgameinfo.is_from_QMM = true;
    return (bool)::vmMain(GAME_CLIENT_CONNECT, ent, userinfo, social_id, isBot);
}


void Q2R_GameSupport::ClientUserinfoChanged(edict_t* ent, const char* userinfo) {
    // get client number (ent->s.number is not set until CLIENT_BEGIN, so calculate based on edict_t*)
    if (orig_export && orig_export->edicts && orig_export->edict_size) {
        intptr_t entnum = (intptr_t)(((intptr_t)ent - (intptr_t)orig_export->edicts) / orig_export->edict_size);
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


std::vector<std::string> Q2R_GameSupport::entity_tokens;
size_t Q2R_GameSupport::token_counter = 0;
void Q2R_GameSupport::SpawnEntities(const char* mapname, const char* entstring, const char* spawnpoint) {
    if (entstring) {
        entity_tokens = util_parse_entstring(entstring);
        token_counter = 0;
    }
    cgameinfo.is_from_QMM = true;
    (void)::vmMain(GAME_SPAWN_ENTITIES, mapname, entstring, spawnpoint);
}


// struct with lambdas that call QMM's vmMain function. this is given to the game engine
game_export_t Q2R_GameSupport::qmm_export = {
    GAME_API_VERSION,	// apiversion
    GEN_EXPORT(PreInit, GAME_PREINIT),
    GEN_EXPORT(Init, GAME_INIT),
    GEN_EXPORT(Shutdown, GAME_SHUTDOWN),
    Q2R_GameSupport::SpawnEntities,
    GEN_EXPORT(WriteGameJson, GAME_WRITE_GAME),
    GEN_EXPORT(ReadGameJson, GAME_READ_GAME),
    GEN_EXPORT(WriteLevelJson, GAME_WRITE_LEVEL),
    GEN_EXPORT(ReadLevelJson, GAME_READ_LEVEL),
    GEN_EXPORT(CanSave, GAME_CAN_SAVE),
    GEN_EXPORT(ClientChooseSlot, GAME_CLIENT_CHOOSESLOT),
    Q2R_GameSupport::ClientConnect,
    GEN_EXPORT(ClientBegin, GAME_CLIENT_BEGIN),
    Q2R_GameSupport::ClientUserinfoChanged,
    GEN_EXPORT(ClientDisconnect, GAME_CLIENT_DISCONNECT),
    GEN_EXPORT(ClientCommand, GAME_CLIENT_COMMAND),
    GEN_EXPORT(ClientThink, GAME_CLIENT_THINK),
    GEN_EXPORT(RunFrame, GAME_RUN_FRAME),
    GEN_EXPORT(PrepFrame, GAME_PREP_FRAME),
    GEN_EXPORT(ServerCommand, GAME_SERVER_COMMAND),
    // the engine won't use these until after Init, so we can fill these in after each call into the mod's export functions ("vmMain")
    nullptr,			// edicts
    0,					// edict_size
    0,					// num_edicts
    0,					// max_edicts
    SERVER_FLAGS_NONE,	// server_flags (0)
    GEN_EXPORT(Pmove, GAME_PMOVE),
    GEN_EXPORT(GetExtension, GAME_GET_EXTENSION),
    GEN_EXPORT(Bot_SetWeapon, GAME_BOT_SETWEAPON),
    GEN_EXPORT(Bot_TriggerEdict, GAME_BOT_TRIGGEREDICT),
    GEN_EXPORT(Bot_UseItem, GAME_BOT_USEITEM),
    GEN_EXPORT(Bot_GetItemID, GAME_BOT_GETITEMID),
    GEN_EXPORT(Edict_ForceLookAtPoint, GAME_EDICT_FORCELOOKATPOINT),
    GEN_EXPORT(Bot_PickedUpItem, GAME_BOT_PICKEDUPITEM),
    GEN_EXPORT(Entity_IsVisibleToPlayer, GAME_ENTITY_ISVISIBLETOPLAYER),
    GEN_EXPORT(GetShadowLightData, GAME_GETSHADOWLIGHTDATA),
};
#endif // QMM_OS_WINDOWS && QMM_ARCH_64
