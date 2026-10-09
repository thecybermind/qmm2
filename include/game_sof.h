/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAME_SOF_H
#define QMM2_GAME_SOF_H

// import ("syscall") cmds
enum {
    G_MODELINDEX,
    G_SOUNDINDEX,
    G_EFFECTINDEX,
    G_IMAGEINDEX,
    G_UNLOAD_SOUND,
    G_FILTERPACKET,
    G_CREATEGHOULCONFIGSTRINGS,
    G_SETMODEL,
    G_SETRENDERMODEL,
    G_ARGC,
    G_ARGV,
    G_ARGS,
    G_BPRINTF,
    G_DPRINTF,
    G_CPRINTF,
    G_CLPRINTF,
    G_WELCOMEPRINT,
    G_CENTERPRINTF,
    G_CINPRINTF,
    G_BCAPTION,
    G_CAPTIONPRINTF,
    G_CON_CLEARNOTIFY,
    G_SOUND,
    G_POSITIONED_SOUND,
    G_DEBUGGRAPH,
    G_DAMAGETEXTURE,
    G_SURFACETYPELIST,
    G_UPDATE,
    G_MULTICAST,
    G_MULTICASTIGNORE,
    G_UNICAST,
    G_MSG_WRITECHAR,
    G_MSG_WRITEBYTE,
    G_MSG_WRITESHORT,
    G_MSG_WRITELONG,
    G_MSG_WRITEFLOAT,
    G_MSG_WRITESTRING,
    G_MSG_WRITEPOSITION,
    G_MSG_WRITEDIR,
    G_MSG_WRITEANGLE,
    G_WRITEBYTE_SIZEBUF,
    G_WRITESHORT_SIZEBUF,
    G_WRITELONG_SIZEBUF,
    G_RELIABLE_WRITEBYTE_TOCLIENT,
    G_RELIABLE_WRITEDATA_TOCLIENT,
    G_GETNEARESTBYTENORMAL,
    G_SENDPLAYERNAMECOLORS,
    G_SP_REGISTER,
    G_SP_PRINT,
    G_SP_PRINT_OBIT,
    G_SP_SPRINT,
    G_SP_GETSTRINGTEXT,
    G_TRACE,
    G_POLYTRACE,
    G_POINT_CONTENTS,
    G_REGIONDISTANCE,
    G_IN_PVS,
    G_IN_PHS,
    G_SETAREAPORTALSTATE,
    G_AREAS_CONNECTED,
    G_GETGHOUL,
    G_NEWPLAYERMODELINFO,
    G_FINDGSQFILE,
    G_READGSQENTRY,
    G_PRECACHEGSQFILE,
    G_REGISTERGSQSEQUENCES,
    G_TURNOFFPARTSFROMGSQFILE,
    GVP_ISCLIENT,
    G_CONFIGSTRING,
    G_SZ_INIT,
    G_SZ_CLEAR,
    G_SZ_WRITE,
    G_ERROR,
    G_SYS_CONSOLEOUTPUT,
    G_SYS_GETPLAYERAPI,
    G_SYS_UNLOADPLAYER,
    G_FLRAND,
    G_IRAND,
    G_LINKENTITY,
    G_UNLINKENTITY,
    G_BOXEDICTS,
    G_PMOVE,
    G_TAGMALLOC,
    G_TAGFREE,
    G_FREETAGS,
    G_APPENDTOSAVEGAME,
    G_READFROMSAVEGAME,
    G_CVAR,
    G_CVAR_SET,
    G_CVAR_SETVALUE,
    G_CVAR_FORCESET,
    G_CVAR_INFO,
    G_CVAR_VARIABLEVALUE,
    G_FS_LOADFILE,
    G_FS_FREEFILE,
    G_FS_USERDIR,
    G_FS_CREATEPATH,
    G_FS_FILEEXISTS,
    G_ADDCOMMANDSTRING,
    
    G_SET_CONFIGSTRING = G_CONFIGSTRING,
    G_CLIENT_PRINT = G_CPRINTF,
};

// export ("vmMain") cmds
enum {
    GAMEV_APIVERSION,
    GAME_INIT,
    GAME_SHUTDOWN,
    GAME_SPAWN_ENTITIES,
    GAME_WRITE_GAME,
    GAME_READ_GAME,
    GAME_WRITE_LEVEL,
    GAME_READ_LEVEL,
    GAME_CLIENT_CONNECT,
    GAME_CLIENT_BEGIN,
    GAME_CLIENT_USERINFO_CHANGED,
    GAME_CLIENT_DISCONNECT,
    GAME_CLIENT_COMMAND,
    GAME_CLIENT_THINK,
    GAME_RESETCTFTEAM,
    GAME_GAMEALLOWASAVE,
    GAME_SAVESLEFT,
    GAME_GETGAMESTATS,
    GAME_UPDATEINVEN,
    GAME_GETDMGAMENAME,
    GAME_GETCINEMATICFREEZE,
    GAME_SETCINEMATICFREEZE,
    GAME_RUN_FRAME,
    
    GAMEVP_EDICTS,
    GAMEV_EDICT_SIZE,
    GAMEV_NUM_EDICTS,
    GAMEV_MAX_EDICTS,
};

// these import messages do not have an exact analogue in SOF
enum {
    // used by QMM to make and check cvars
    G_CVAR_REGISTER = -100,           // void (vmcvar_t* ignored_cvar, const char *varName, const char *defaultValue, int flags)
    G_CVAR_VARIABLE_STRING_BUFFER,    // void (const char* var_name, char* buffer, int bufsize)
    G_CVAR_VARIABLE_INTEGER_VALUE,    // int (const char* var_name)
    G_SEND_CONSOLE_COMMAND,           // void (int ignored_exec_when, const char *text)
    G_PRINT,                          // void (const char *fmt)
    G_QMM_REGISTER_CONSOLE_COMMAND,   // void (const char *cmd)
    // file loading
    G_FS_FOPEN_FILE,                  // int (const char *qpath, fileHandle_t *f, fsMode_t mode)
    G_FS_READ,                        // void (void* buffer, int len, fileHandle_t f)
    G_FS_WRITE,                       // void (const void* buffer, int len, fileHandle_t f)
    G_FS_FCLOSE_FILE,                 // void (fileHandle_t f)
    // helper for plugins to not need separate logic
    G_LOCATE_GAME_DATA,               // void (gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient)
    G_DROP_CLIENT,                    // void (int clientNum)
    G_GET_USERINFO,                   // void (edict_t* ent, char* userinfo, int bufferSize)
    G_GET_ENTITY_TOKEN,               // bool (char *buffer, int bufferSize)
    G_GET_CONFIGSTRING,               // void ( int num, char *buffer, int bufferSize )
    G_MILLISECONDS,                   // int ()
};

// these export messages do not have an exact analogue in SOF
enum {
    GAME_CONSOLE_COMMAND = -100,      // void ()
};

typedef intptr_t fileHandle_t;

// allow plugins to use this type for easier code
typedef edict_t gentity_t;

// other values
enum {
    // file flags
    FS_READ,
    FS_WRITE,
    FS_APPEND,
    FS_APPEND_SYNC = FS_APPEND,
    // used by qmm_version cvar
    CVAR_ROM = CVAR_NOSET,
};

#endif // QMM2_GAME_SOF_H
