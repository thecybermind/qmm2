/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "osdef.h"

// AFAIK Quake 2 Remastered is only available on 64-bit Windows, so skip the whole file otherwise.
// The game entry in game_api is similarly conditionally compiled.
#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)

#define _CRT_SECURE_NO_WARNINGS 1
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <q2r/rerelease/game.h>
#include "gameapi.hpp"
#include "log.hpp"
#include "format.hpp"
// QMM-specific Q2RC header
#include "game_q2rc.h"
#include "qmm.hpp"
#include "util.hpp"

struct Q2RC_GameSupport : public GameSupport {
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

    virtual const char* DefaultDLLName() { return "game_" X64_DLL; }
    virtual const char* DefaultModDir() { return "baseq2"; }
    virtual const char* ModCvar() { return "game"; }
    virtual const char* GameName() { return "Quake 2 Remastered (SP)"; }
    virtual const char* GameCode() { return "Q2RC"; }
    virtual bool IsHidden() { return true; }

private:
    // a copy of the original import struct that comes from the game engine
    static cgame_import_t orig_import;

    // a copy of the original export struct pointer that comes from the mod
    static cgame_export_t* orig_export;

    // struct with lambdas that call QMM's vmMain function. this is given to the game engine
    static cgame_export_t qmm_export;

    const int qmm_eng_msgs[QMM_ENGINE_MSG_COUNT] = { CG_PRINT, CG_ERROR, CG_ARGV, CG_ARGC, CG_SEND_CONSOLE_COMMAND, CG_GET_CONFIGSTRING, CG_CVAR_REGISTER, CG_CVAR_VARIABLE_STRING_BUFFER, CG_CVAR_VARIABLE_INTEGER_VALUE, CVAR_SERVERINFO, CVAR_ROM, CG_FS_FOPEN_FILE, CG_FS_READ, CG_FS_WRITE, CG_FS_FCLOSE_FILE, EXEC_APPEND, FS_READ, };
    const int qmm_mod_msgs[QMM_MOD_MSG_COUNT] = { CGAME_INIT, CGAME_SHUTDOWN, CGAME_CONSOLE_COMMAND, };
};

GEN_GAME_OBJ(Q2RC);


// auto-detection logic for Q2RC
bool Q2RC_GameSupport::AutoDetect(APIType engine_api) {
    if (engine_api != QMM_API_GETCGAMEAPI)
        return false;

    if (!Util::str_striequal(QMM::qmm_file, DefaultDLLName()))
        return false;

    if (!Util::str_stristr(QMM::exe_file, "quake2ex"))
        return false;

    return true;
}


// this syscall function is only used by QMM to call into the engine for basic functionality
// we only need to implement what QMM actually calls: print, error, console command, cvar register
intptr_t Q2RC_GameSupport::syscall_args(intptr_t cmd, intptr_t* args) {
    if (cmd != CG_PRINT)
        QMMLOG(QMM_LOG_TRACE, "QMM") << "Q2RC_GameSupport::syscall(" << EngMsgName(cmd) << "(" << cmd << ")) called\n";

    if (orig_import.Com_Print) {
        switch (cmd) {
            case CG_PRINT: {
                const char* msg = (const char*)(args[0]);
                orig_import.Com_Print(msg);
                break;
            }
            case CG_ERROR: {
                const char* msg = (const char*)(args[0]);
                orig_import.Com_Error(msg);
                break;
            }
            case CG_SEND_CONSOLE_COMMAND: {
                // Q2R: void (*AddCommandString)(const char *text);
                // qmm: void trap_SendConsoleCommand( int exec_when, const char *text );
                const char* text = (const char*)(args[1]);
                orig_import.AddCommandString(text);
                break;
            }
            case CG_CVAR_REGISTER: {
                // q2r: cvar_t *(*cvar) (const char *var_name, const char *value, cvar_flags_t flags);
                // qmm: void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
                // qmm always passes NULL for vmCvar so don't worry about it
                const char* var_name = (char*)(args[1]);
                const char* value = (char*)(args[2]);
                cvar_flags_t flags = (cvar_flags_t)args[3];
                (void)orig_import.cvar(var_name, value, flags);
                break;
            }
            // including for completeness
            case CG_ARGV:
            case CG_ARGC:
            case CG_CVAR_VARIABLE_STRING_BUFFER:
            case CG_CVAR_VARIABLE_INTEGER_VALUE:
            case CG_FS_FOPEN_FILE:
            case CG_GET_CONFIGSTRING:
            case CG_FS_READ:
            case CG_FS_WRITE:
            case CG_FS_FCLOSE_FILE:
                break;

            default:
                break;
        };
    }
    if (cmd != CG_PRINT)
        QMMLOG(QMM_LOG_TRACE, "QMM") << "Q2RC_GameSupport::syscall(" << EngMsgName(cmd) << "(" << cmd << ")) returning\n";

    return 0;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t Q2RC_GameSupport::vmMain_args(intptr_t cmd, intptr_t*) {
    QMMLOG(QMM_LOG_TRACE, "QMM") << "Q2RC_GameSupport::vmMain(" << ModMsgName(cmd) << "(" << cmd << ")) called\n";

    if (orig_export) {
        switch (cmd) {
            case CGAME_INIT:
                orig_export->Init();
                break;
            case CGAME_SHUTDOWN:
                orig_export->Shutdown();
                break;
            default:
                break;
        };
    }

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Q2RC_GameSupport::vmMain(" << ModMsgName(cmd) << "(" << cmd << ")) returning\n";

    return 0;
}


void* Q2RC_GameSupport::Entry(void* import, void*, APIType engine_api) {
    QMMLOG(QMM_LOG_DEBUG, "QMM") << "Q2RC_GameSupport::Entry(" << import << ") called\n";

    void* ret = nullptr;

    if (engine_api == QMM_API_GETCGAMEAPI) {
        // original import struct from engine
        // the struct given by the engine goes out of scope after this returns so we have to copy the whole thing
        cgame_import_t* gi = (cgame_import_t*)import;
        orig_import = *gi;

        // struct full of export lambdas to QMM's vmMain
        // this gets returned to the game engine, but we haven't loaded the mod yet.
        // the only thing in this struct the engine uses before calling Init is the apiversion
        ret = &qmm_export;
    }

    QMMLOG(QMM_LOG_DEBUG, "QMM") << "Q2RC_GameSupport::Entry(" << import << ") returning " << ret << "\n";
    return ret;
}


bool Q2RC_GameSupport::ModLoad(void* entry, APIType mod_api) {
    if (mod_api != QMM_API_GETCGAMEAPI)
        return false;

    mod_GetGameAPI pfnGCGA = (mod_GetGameAPI)entry;
    orig_export = (cgame_export_t*)pfnGCGA(&orig_import, nullptr);

    return !!orig_export;
}


void Q2RC_GameSupport::ModUnload(APIType) {
    orig_export = nullptr;
}


const char* Q2RC_GameSupport::EngMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(CG_PRINT);
        GEN_CASE(CG_ERROR);
        GEN_CASE(CG_ARGV);
        GEN_CASE(CG_ARGC);
        GEN_CASE(CG_SEND_CONSOLE_COMMAND);
        GEN_CASE(CG_GET_CONFIGSTRING);
        GEN_CASE(CG_CVAR_REGISTER);
        GEN_CASE(CG_CVAR_VARIABLE_STRING_BUFFER);
        GEN_CASE(CG_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(CG_FS_FOPEN_FILE);
        GEN_CASE(CG_FS_READ);
        GEN_CASE(CG_FS_WRITE);
        GEN_CASE(CG_FS_FCLOSE_FILE);

        default:
            return "unknown";
    }
}


const char* Q2RC_GameSupport::ModMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(CGAME_INIT);
        GEN_CASE(CGAME_SHUTDOWN);

        default:
            return "unknown";
    }
}


cgame_import_t Q2RC_GameSupport::orig_import;


cgame_export_t* Q2RC_GameSupport::orig_export = nullptr;


// struct with lambdas that call QMM's vmMain function or route directly to the mod's export struct.
// this is given to the game engine
cgame_export_t Q2RC_GameSupport::qmm_export = {
    CGAME_API_VERSION,    // apiversion
    +[]() { QMM::vmMain_args(CGAME_INIT, nullptr); },
    +[]() { QMM::vmMain_args(CGAME_SHUTDOWN, nullptr); },
    +[](int32_t isplit, const cg_server_data_t* data, vrect_t hud_vrect, vrect_t hud_safe, int32_t scale, int32_t playernum, const player_state_t* ps)
        { orig_export->DrawHUD(isplit, data, hud_vrect, hud_safe, scale, playernum, ps); },
    +[]() { orig_export->TouchPics(); },
    +[](const player_state_t* ps) { return orig_export->LayoutFlags(ps); },
    +[](const player_state_t* ps) { return orig_export->GetActiveWeaponWheelWeapon(ps); },
    +[](const player_state_t* ps) { return orig_export->GetOwnedWeaponWheelWeapons(ps); },
    +[](const player_state_t* ps, int32_t ammo_id) { return orig_export->GetWeaponWheelAmmoCount(ps, ammo_id); },
    +[](const player_state_t* ps, int32_t powerup_id) { return orig_export->GetPowerupWheelCount(ps, powerup_id); },
    +[](const player_state_t* ps) { return orig_export->GetHitMarkerDamage(ps); },
    +[](pmove_t* pmove) { orig_export->Pmove(pmove); },
    +[](int32_t i, const char* s) { orig_export->ParseConfigString(i, s); },
    +[](const char* str, int isplit, bool instant) { orig_export->ParseCenterPrint(str, isplit, instant); },
    +[](int isplit) { orig_export->ClearNotify(isplit); },
    +[](int isplit) { orig_export->ClearCenterprint(isplit); },
    +[](int32_t isplit, const char* msg, bool is_chat) { orig_export->NotifyMessage(isplit, msg, is_chat); },
    +[](monster_muzzleflash_id_t id, gvec3_ref_t offset) { orig_export->GetMonsterFlashOffset(id, offset); },
    +[](const char* name) { return orig_export->GetExtension(name); },
};

#endif // QMM_OS_WINDOWS && QMM_ARCH_64
