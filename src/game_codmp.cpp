/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "version.h"

#if defined(QMM_ARCH_32)

#include <codmp/game/g_public.h>

#include "game_api.hpp"
#include "log.hpp"
#include "format.hpp"
#include <string>
// QMM-specific CODMP header
#include "game_codmp.h"
#include "gameinfo.hpp"
#include "util.hpp"

struct CODMP_GameSupport : public GameSupport {
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

    virtual const char* DefaultDLLName() { return "game" MP_DLL MOD_DLL; }
    virtual const char* DefaultModDir() { return "Main"; }
    virtual const char* GameName() { return "Call of Duty (MP)"; }
    virtual const char* GameCode() { return "CODMP"; }

private:
    // a copy of the original syscall from the engine
    eng_syscall orig_syscall = nullptr;

    // a copy of the vmMain function from the mod
    mod_vmMain orig_vmMain = nullptr;

    const int qmm_eng_msgs[QMM_ENGINE_MSG_COUNT] = GEN_GAME_QMM_ENG_MSGS();
    // GAME_GET_APIVERSION gets called first, which is when QMM has to perform mod/plugin loading, but we
    // don't want to make plugins have to use separate code to handle the actual GAME_INIT message
    const int qmm_mod_msgs[QMM_MOD_MSG_COUNT] = { GAME_GET_APIVERSION, GAME_SHUTDOWN, GAME_CONSOLE_COMMAND, };
};

GEN_GAME_OBJ(CODMP);


// auto-detection logic for CODMP
bool CODMP_GameSupport::AutoDetect(APIType engineapi) {
    if (engineapi != QMM_API_DLLENTRY)
        return false;

    if (!str_striequal(gameinfo.qmm_file, DefaultDLLName()))
        return false;

    if (!str_stristr(gameinfo.exe_file, "codmp") && !str_stristr(gameinfo.exe_file, "cod_lnxded"))
        return false;

    return true;
}


// wrapper syscall function that calls actual engine func in orig_syscall
// this is how QMM and plugins will call into the engine
intptr_t CODMP_GameSupport::syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_GameSupport::syscall({} {}) called\n", EngMsgName(cmd), cmd);
#endif

    intptr_t ret = 0;

    switch (cmd) {
    // handle special cmds which QMM uses but CODMP doesn't have an analogue for
    case G_ARGS: {
        // quake2: char* (*args)(void);
        static std::string s;
        static char buf[MAX_STRING_CHARS];
        s = "";
        int i = 1;
        while (i < orig_syscall(G_ARGC)) {
            orig_syscall(G_ARGV, i, buf, sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            if (i != 1)
                s += " ";
            s += buf;
        }
        ret = (intptr_t)s.c_str();
        break;
    }

    default:
        // all normal engine functions go to syscall
        ret = orig_syscall(cmd, QMM_PUT_SYSCALL_ARGS());
    }

    // do anything that needs to be done after function call here

#ifdef _DEBUG
    if (cmd != G_PRINT)
        LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_GameSupport::syscall({} {}) returning {}\n", EngMsgName(cmd), cmd, ret);
#endif

    return ret;
}


// wrapper vmMain function that calls actual mod func in orig_vmMain
// this is how QMM and plugins will call into the mod
intptr_t CODMP_GameSupport::vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_GameSupport::vmMain({} {}) called\n", ModMsgName(cmd), cmd);
#endif

    if (!orig_vmMain)
        return 0;

    // store return value since we do some stuff after the function call is over
    intptr_t ret = 0;

    // all normal mod functions go to vmMain
    ret = orig_vmMain(cmd, QMM_PUT_VMMAIN_ARGS());

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_GameSupport::vmMain({} {}) returning {}\n", ModMsgName(cmd), cmd, ret);
#endif

    return ret;
}


void* CODMP_GameSupport::Entry(void* syscall, void*, APIType) {
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_GameSupport::Entry({}) called\n", syscall);

    // store original syscall from engine
    orig_syscall = (eng_syscall)syscall;

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_GameSupport::Entry({}) returning\n", syscall);

    return nullptr;
}


bool CODMP_GameSupport::ModLoad(void* entry, APIType modapi) {
    if (modapi != QMM_API_DLLENTRY)
        return false;

    orig_vmMain = (mod_vmMain)entry;

    return !!orig_vmMain;
}


void CODMP_GameSupport::ModUnload() {
    orig_vmMain = nullptr;
}


const char* CODMP_GameSupport::EngMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(G_PRINTF);
        GEN_CASE(G_ERROR);
        GEN_CASE(G_ERROR_LOCALIZED);
        GEN_CASE(G_MILLISECONDS);
        GEN_CASE(G_CVAR_REGISTER);
        GEN_CASE(G_CVAR_UPDATE);
        GEN_CASE(G_CVAR_SET);
        GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
        GEN_CASE(G_CVAR_VARIABLE_VALUE);
        GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
        GEN_CASE(G_ARGC);
        GEN_CASE(G_ARGV);
        GEN_CASE(G_HUNK_ALLOCINTERNAL);
        GEN_CASE(G_HUNK_ALLOCLOWINTERNAL);
        GEN_CASE(G_HUNK_ALLOCALIGNINTERNAL);
        GEN_CASE(G_HUNK_ALLOCLOWALIGNINTERNAL);
        GEN_CASE(G_HUNK_ALLOCATETEMPMEMORYINTERNAL);
        GEN_CASE(G_HUNK_FREETEMPMEMORYINTERNAL);
        GEN_CASE(G_FS_FOPEN_FILE);
        GEN_CASE(G_FS_READ);
        GEN_CASE(G_FS_WRITE);
        GEN_CASE(G_FS_RENAME);
        GEN_CASE(G_FS_FCLOSE_FILE);
        GEN_CASE(G_SEND_CONSOLE_COMMAND);
        GEN_CASE(G_LOCATE_GAME_DATA);
        GEN_CASE(G_GET_GUID);
        GEN_CASE(G_DROP_CLIENT);
        GEN_CASE(G_SEND_SERVER_COMMAND);
        GEN_CASE(G_SET_CONFIGSTRING);
        GEN_CASE(G_GET_CONFIGSTRING);
        GEN_CASE(G_GET_CONFIGSTRING_CONST);
        GEN_CASE(G_ISLOCALCLIENT);
        GEN_CASE(G_GET_CLIENT_PING);
        GEN_CASE(G_GET_USERINFO);
        GEN_CASE(G_SET_USERINFO);
        GEN_CASE(G_GET_SERVERINFO);
        GEN_CASE(G_SET_BRUSH_MODEL);
        GEN_CASE(G_TRACE);
        GEN_CASE(G_TRACECAPSULE);
        GEN_CASE(G_SIGHTTRACE);
        GEN_CASE(G_SIGHTTRACE_CAPSULE);
        GEN_CASE(G_SIGHTTRACEENTITY);
        GEN_CASE(G_CM_BOXTRACE);
        GEN_CASE(G_CM_CAPSULETRACE);
        GEN_CASE(G_CM_BOXSIGHTTRACE);
        GEN_CASE(G_CM_CAPSULESIGHTTRACE);
        GEN_CASE(G_LOCATIONAL_TRACE);
        GEN_CASE(G_POINT_CONTENTS);
        GEN_CASE(G_IN_PVS);
        GEN_CASE(G_IN_PVS_IGNORE_PORTALS);
        GEN_CASE(G_IN_SNAPSHOT);
        GEN_CASE(G_ADJUST_AREA_PORTAL_STATE);
        GEN_CASE(G_AREAS_CONNECTED);
        GEN_CASE(G_LINKENTITY);
        GEN_CASE(G_UNLINKENTITY);
        GEN_CASE(G_ENTITIES_IN_BOX);
        GEN_CASE(G_ENTITY_CONTACT);
        GEN_CASE(G_GET_USERCMD);
        GEN_CASE(G_GET_ENTITY_TOKEN);
        GEN_CASE(G_FS_GETFILELIST);
        GEN_CASE(G_MAPEXISTS);
        GEN_CASE(G_REAL_TIME);
        GEN_CASE(G_SNAPVECTOR);
        GEN_CASE(G_ENTITY_CONTACTCAPSULE);
        GEN_CASE(G_SOUNDALIASSTRING);
        GEN_CASE(G_PICKSOUNDALIAS);
        GEN_CASE(G_SOUNDALIASINDEX);
        GEN_CASE(G_SURFACETYPEFROMNAME);
        GEN_CASE(G_SURFACETYPETONAME);
        GEN_CASE(G_ADDTESTCLIENT);
        GEN_CASE(G_ARCHIVED_CLIENTINFO);
        GEN_CASE(G_ADDDEBUGSTRING);
        GEN_CASE(G_ADDDEBUGLINE);
        GEN_CASE(G_SET_ARCHIVE);
        GEN_CASE(G_ZMALLOC_INTERNAL);
        GEN_CASE(G_ZFREE_INTERNAL);
        GEN_CASE(G_XANIM_CREATETREE);
        GEN_CASE(G_XANIM_CREATESMALLTREE);
        GEN_CASE(G_XANIM_FREESMALLTREE);
        GEN_CASE(G_XMODELEXISTS);
        GEN_CASE(G_XMODELGET);
        GEN_CASE(G_DOBJ_CREATE);
        GEN_CASE(G_DOBJ_EXISTS);
        GEN_CASE(G_DOBJ_SAFEFREE);
        GEN_CASE(G_XANIM_GETANIMS);
        GEN_CASE(G_XANIM_CLEARTREEGOALWEIGHTS);
        GEN_CASE(G_XANIM_CLEARGOALWEIGHT);
        GEN_CASE(G_XANIM_CLEARTREEGOALWEIGHTSSTRICT);
        GEN_CASE(G_XANIM_SETCOMPLETEGOALWEIGHTKNOB);
        GEN_CASE(G_XANIM_SETCOMPLETEGOALWEIGHTKNOBALL);
        GEN_CASE(G_XANIM_SETANIMRATE);
        GEN_CASE(G_XANIM_SETTIME);
        GEN_CASE(G_XANIM_SETGOALWEIGHTKNOB);
        GEN_CASE(G_XANIM_CLEARTREE);
        GEN_CASE(G_XANIM_HASTIME);
        GEN_CASE(G_XNAIM_ISPRIMITIVE);
        GEN_CASE(G_XANIM_GETLENGTH);
        GEN_CASE(G_XANIM_GETLENGTHSECONDS);
        GEN_CASE(G_XANIM_SETCOMPLETEGOALWEIGHT);
        GEN_CASE(G_XANIM_SETGOALWEIGHT);
        GEN_CASE(G_XANIM_CALCABSDELTA);
        GEN_CASE(G_XANIM_CALCDELTA);
        GEN_CASE(G_XANIM_GETRELDELTA);
        GEN_CASE(G_XANIM_GETABSDELTA);
        GEN_CASE(G_XANIM_ISLOOPED);
        GEN_CASE(G_XANIM_NOTETRACKEXISTS);
        GEN_CASE(G_XANIM_GETTIME);
        GEN_CASE(G_XANIM_GETWEIGHT);
        GEN_CASE(G_DOBJ_DUMPINFO);
        GEN_CASE(G_DOBJ_CREATESKELFORBONE);
        GEN_CASE(G_DOBJ_CREATESKELFORBONES);
        GEN_CASE(G_DOBJ_UPDATESERVERTIME);
        GEN_CASE(G_DOBJ_INITSERVERTIME);
        GEN_CASE(G_DOBJ_GETHIERARCHYBITS);
        GEN_CASE(G_DOBJ_CALCANIM);
        GEN_CASE(G_DOBJ_CALCSKEL);
        GEN_CASE(G_XANIM_LOADANIMTREE);
        GEN_CASE(G_XANIM_SAVEANIMTREE);
        GEN_CASE(G_XANIM_CLONEANIMTREE);
        GEN_CASE(G_DOBJ_NUMBONES);
        GEN_CASE(G_DOBJ_GETBONEINDEX);
        GEN_CASE(G_DOBJ_GETMATRIXARRAY);
        GEN_CASE(G_DOBJ_DISPLAYANIM);
        GEN_CASE(G_XANIM_HASFINISHED);
        GEN_CASE(G_XANIM_GETNUMCHILDREN);
        GEN_CASE(G_XANIM_GETCHILDAT);
        GEN_CASE(G_XMODEL_NUMBONES);
        GEN_CASE(G_XMODEL_GETBONENAMES);
        GEN_CASE(G_DOBJ_GETROTTRANSARRAY);
        GEN_CASE(G_DOBJ_SETROTTRANSARRAY);
        GEN_CASE(G_DOBJ_SETCONTROLROTTRANSINDEX);
        GEN_CASE(G_DOBJ_GETBOUNDS);
        GEN_CASE(G_XANIM_GETANIMNAME);
        GEN_CASE(G_DOBJ_GETTREE);
        GEN_CASE(G_XANIM_GETANIMTREESIZE);
        GEN_CASE(G_XMODEL_DEBUGBOXES);
        GEN_CASE(G_GETWEAPONINFOMEMORY);
        GEN_CASE(G_FREEWEAPONINFOMEMORY);
        GEN_CASE(G_FREECLIENTSCRIPTPERS);
        GEN_CASE(G_RESETENTITYPARSEPOINT);

        // polyfills
        GEN_CASE(G_ARGS);

    default:
        return "unknown";
    }
}


const char* CODMP_GameSupport::ModMsgName(intptr_t cmd) {
    switch (cmd) {
        GEN_CASE(GAME_DEFAULT_0);
        GEN_CASE(GAME_GET_APIVERSION);
        GEN_CASE(GAME_INIT);
        GEN_CASE(GAME_SHUTDOWN);
        GEN_CASE(GAME_CLIENT_CONNECT);
        GEN_CASE(GAME_CLIENT_BEGIN);
        GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
        GEN_CASE(GAME_CLIENT_DISCONNECT);
        GEN_CASE(GAME_CLIENT_COMMAND);
        GEN_CASE(GAME_CLIENT_THINK);
        GEN_CASE(GAME_GET_FOLLOWPLAYERSTATE);
        GEN_CASE(GAME_UPDATE_CVARS);
        GEN_CASE(GAME_RUN_FRAME);
        GEN_CASE(GAME_CONSOLE_COMMAND);
        GEN_CASE(GAME_SCR_FARHOOK);
        GEN_CASE(GAME_DOBJ_CALCPOSE);
        GEN_CASE(GAME_GET_NUMWEAPONS);
        GEN_CASE(GAME_SET_SAVEPERSIST);
        GEN_CASE(GAME_GET_SAVEPERSIST);
        GEN_CASE(GAME_GET_CLIENTSTATE);
        GEN_CASE(GAME_GET_CLIENTARCHIVETIME);
        GEN_CASE(GAME_SET_CLIENTARCHIVETIME);
        GEN_CASE(GAME_GET_CLIENTSCORE);
        GEN_CASE(GAME_GET_FOG_DISTANCE);

    default:
        return "unknown";
    }
}

#endif // QMM_ARCH_32
