/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_GAME_QUAKE2_H__
#define __QMM2_GAME_QUAKE2_H__

// import ("syscall") cmds
enum {
	G_BPRINTF,
	G_DPRINTF,
	G_CPRINTF,
	G_CENTERPRINTF,
	G_SOUND,
	G_POSITIONED_SOUND,
	G_CONFIGSTRING,
	G_ERROR,
	G_MODELINDEX,
	G_SOUNDINDEX,
	G_IMAGEINDEX,
	G_SETMODEL,
	G_TRACE,
	G_POINTCONTENTS,
	G_INPVS,
	G_INPHS,
	G_SETAREAPORTALSTATE,
	G_AREASCONNECTED,
	G_LINKENTITY,
	G_UNLINKENTITY,
	G_BOXEDICTS,
	G_PMOVE,
	G_MULTICAST,
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
	G_TAGMALLOC,
	G_TAGFREE,
	G_FREETAGS,
	G_CVAR,
	G_CVAR_SET,
	G_CVAR_FORCESET,
	G_ARGC,
	G_ARGV,
	G_ARGS,
	G_ADDCOMMANDSTRING,
	G_DEBUGGRAPH,

	G_SET_CONFIGSTRING = G_CONFIGSTRING,
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
	GAME_RUN_FRAME,
	GAME_SERVER_COMMAND,
	GAMEVP_EDICTS,
	GAMEV_EDICT_SIZE,
	GAMEV_NUM_EDICTS,
	GAMEV_MAX_EDICTS,

	GAME_CONSOLE_COMMAND = GAME_SERVER_COMMAND,
};

// these import messages do not have an exact analogue in QUAKE2
enum {
	// used by QMM to make and check cvars
	G_CVAR_REGISTER = -100,			// void (void* ignored, const char *varName, const char *defaultValue, int flags)
	G_CVAR_VARIABLE_STRING_BUFFER,	// void (const char* var_name, char* buffer, int bufsize)
	G_CVAR_VARIABLE_INTEGER_VALUE,	// int (const char* var_name)
	G_SEND_CONSOLE_COMMAND,			// void (int exec_when, const char *text)
	G_PRINT,						// void (const char *fmt)
	// file loading
	G_FS_FOPEN_FILE,				// int (const char *qpath, fileHandle_t *f, fsMode_t mode)
	G_FS_READ,						// void (void* buffer, int len, fileHandle_t f)
	G_FS_WRITE,						// void (const void* buffer, int len, fileHandle_t f)
	G_FS_FCLOSE_FILE,				// void (fileHandle_t f)
	// helper for plugins to not need separate logic
	G_LOCATE_GAME_DATA,				// (gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient)
	G_DROP_CLIENT,					// (int clientNum)
	G_GET_USERINFO,					// (edict_t* ent, char* userinfo, int bufferSize)
};

typedef intptr_t fileHandle_t;

// other values
enum {
	// not used with Q2R
	EXEC_APPEND,
	// file flags
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC = FS_APPEND,
	// used by qmm_version cvar
	CVAR_ROM = CVAR_NOSET,
};

#endif // __QMM2_GAME_QUAKE2_H__
