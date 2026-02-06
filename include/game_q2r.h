/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAME_Q2R_H
#define QMM2_GAME_Q2R_H

// AFAIK Quake 2 Remastered is only available on 64-bit Windows, so skip the whole file otherwise.
// The game entry in game_api is similarly conditionally compiled.
#if defined(_WIN64)

// import ("syscall") cmds
enum {
	GV_TICK_RATE,
	GV_FRAME_TIME_S,
	GV_FRAME_TIME_MS,
	G_BROADCAST_PRINT,
	G_COM_PRINT,
	G_CLIENT_PRINT,
	G_CENTERPRINT,
	G_SOUND,
	G_POSITIONED_SOUND,
	G_LOCAL_SOUND,
	G_CONFIGSTRING,
	G_GET_CONFIGSTRING,
	G_COM_ERROR,
	G_MODELINDEX,
	G_SOUNDINDEX,
	G_IMAGEINDEX,
	G_SETMODEL,
	G_TRACE,
	G_CLIP,
	G_POINT_CONTENTS,
	G_IN_PVS,
	G_IN_PHS,
	G_SETAREAPORTALSTATE,
	G_AREAS_CONNECTED,
	G_LINKENTITY,
	G_UNLINKENTITY,
	G_BOXEDICTS,
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
	G_MSG_WRITEENTITY,
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
	G_GET_EXTENSION,
	G_BOT_REGISTEREDICT,
	G_BOT_UNREGISTEREDICT,
	G_BOT_MOVETOPOINT,
	G_BOT_FOLLOWACTOR,
	G_GETPATHTOGOAL,
	G_LOC_PRINT,
	G_DRAW_LINE,
	G_DRAW_POINT,
	G_DRAW_CIRCLE,
	G_DRAW_BOUNDS,
	G_DRAW_SPHERE,
	G_DRAW_ORIENTEDWORLDTEXT,
	G_DRAW_STATICWORLDTEXT,
	G_DRAW_CYLINDER,
	G_DRAW_RAY,
	G_DRAW_ARROW,
	G_REPORTMATCHDETAILS_MULTICAST,
	G_SERVER_FRAME,
	G_SENDTOCLIPBOARD,
	G_INFO_VALUEFORKEY,
	G_INFO_REMOVEKEY,
	G_INFO_SETVALUEFORKEY,

	G_PRINT = G_COM_PRINT,
	G_ERROR = G_COM_ERROR,
	G_SET_CONFIGSTRING = G_CONFIGSTRING,
	G_CPRINTF = G_CLIENT_PRINT,
};

// export ("vmMain") cmds
enum {
	GAMEV_APIVERSION,
	GAME_PREINIT,
	GAME_INIT,
	GAME_SHUTDOWN,
	GAME_SPAWN_ENTITIES,
	GAME_WRITE_GAME,
	GAME_READ_GAME,
	GAME_WRITE_LEVEL,
	GAME_READ_LEVEL,
	GAME_CAN_SAVE,
	GAME_CLIENT_CHOOSESLOT,
	GAME_CLIENT_CONNECT,
	GAME_CLIENT_BEGIN,
	GAME_CLIENT_USERINFO_CHANGED,
	GAME_CLIENT_DISCONNECT,
	GAME_CLIENT_COMMAND,
	GAME_CLIENT_THINK,
	GAME_RUN_FRAME,
	GAME_PREP_FRAME,
	GAME_SERVER_COMMAND,
	GAMEVP_EDICTS,
	GAMEV_EDICT_SIZE,
	GAMEV_NUM_EDICTS,
	GAMEV_MAX_EDICTS,
	GAMEV_SERVER_FLAGS,
	GAME_PMOVE,
	GAME_GET_EXTENSION,
	GAME_BOT_SETWEAPON,
	GAME_BOT_TRIGGEREDICT,
	GAME_BOT_USEITEM,
	GAME_BOT_GETITEMID,
	GAME_EDICT_FORCELOOKATPOINT,
	GAME_BOT_PICKEDUPITEM,
	GAME_ENTITY_ISVISIBLETOPLAYER,
	GAME_GETSHADOWLIGHTDATA,

	GAME_CONSOLE_COMMAND = GAME_SERVER_COMMAND,
};

// these import messages do not have an exact analogue in Q2R
enum {
	// used by QMM to make and check cvars
	G_CVAR_REGISTER = -100,			// void ( vmcvar_t* ignored_cvar, const char *varName, const char *defaultValue, int flags)
	G_CVAR_VARIABLE_STRING_BUFFER,	// void (const char* var_name, char* buffer, int bufsize)
	G_CVAR_VARIABLE_INTEGER_VALUE,	// int (const char* var_name)
	G_SEND_CONSOLE_COMMAND,			// void (int ignored_exec_when, const char *text)
	// file loading
	G_FS_FOPEN_FILE,				// int (const char *qpath, fileHandle_t *f, fsMode_t mode)
	G_FS_READ,						// void (void* buffer, int len, fileHandle_t f)
	G_FS_WRITE,						// void (const void* buffer, int len, fileHandle_t f)
	G_FS_FCLOSE_FILE,				// void (fileHandle_t f)
	// helper for plugins to not need separate logic
	G_LOCATE_GAME_DATA,				// void (gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient)
	G_DROP_CLIENT,					// void (int clientNum)
	G_GET_USERINFO,					// void (edict_t* ent, char* userinfo, int bufferSize)
	G_GET_ENTITY_TOKEN,				// bool (char *buffer, int bufferSize)
};

typedef intptr_t fileHandle_t;

// allow plugins to use this type for easier code
typedef edict_t gentity_t;

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
	CVAR_ROM = CVAR_NOSET // 8
};

#endif // _WIN64

#endif // QMM2_GAME_Q2R_H
