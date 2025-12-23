/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_GAME_STVOYSP_H__
#define __QMM2_GAME_STVOYSP_H__

// import ("syscall") cmds
enum {
	G_PRINTF,
	G_WRITECAM,
	G_ERROR,
	G_MILLISECONDS,
	G_CVAR,
	G_CVAR_SET,
	G_CVAR_VARIABLE_INTEGER_VALUE,
	G_CVAR_VARIABLE_STRING_BUFFER,
	G_ARGC,
	G_ARGV,
	G_FS_FOPEN_FILE,
	G_FS_READ,
	G_FS_WRITE,
	G_FS_FCLOSE_FILE,
	G_FS_READFILE,
	G_FS_FREEFILE,
	G_FS_GETFILELIST,
	G_APPENDTOSAVEGAME,
	G_READFROMSAVEGAME,
	G_READFROMSAVEGAMEOPTIONAL,
	G_SEND_CONSOLE_COMMAND_EX,
	G_DROP_CLIENT,
	G_SEND_SERVER_COMMAND,
	G_SET_CONFIGSTRING,
	G_GET_CONFIGSTRING,
	G_GET_USERINFO,
	G_SET_USERINFO,
	G_GET_SERVERINFO,
	G_SET_BRUSH_MODEL,
	G_TRACE,
	G_POINT_CONTENTS,
	G_IN_PVS,
	G_IN_PVS_IGNOREPORTALS,
	G_ADJUSTAREAPORTALSTATE,
	G_AREAS_CONNECTED,
	G_LINKENTITY,
	G_UNLINKENTITY,
	G_ENTITIES_IN_BOX,
	G_ENTITY_CONTACT,
	GVP_S_OVERRIDE,
	G_MALLOC,
	G_FREE,

	G_PRINT = G_PRINTF,
};

// export ("vmMain") cmds
enum {
	GAMEV_APIVERSION,
	GAME_INIT,
	GAME_SHUTDOWN,
	GAME_WRITE_LEVEL,
	GAME_READ_LEVEL,
	GAME_GAMEALLOWEDTOSAVEHERE,
	GAME_CLIENT_CONNECT,
	GAME_CLIENT_BEGIN,
	GAME_CLIENT_USERINFO_CHANGED,
	GAME_CLIENT_DISCONNECT,
	GAME_CLIENT_COMMAND,
	GAME_CLIENT_THINK,
	GAME_RUN_FRAME,
	GAME_CONSOLE_COMMAND,
	GAMEVP_GENTITIES,
	GAMEV_GENTITYSIZE,
	GAMEV_NUM_ENTITIES,

	GAME_SPAWN_ENTITIES = GAME_INIT,
};

// these import messages do not have an exact analogue in STVOYSP
enum {
	G_CVAR_REGISTER = -100,			// void (vmcvar_t* ignored_cvar, const char *varName, const char *defaultValue, int flags)
	G_SEND_CONSOLE_COMMAND,			// void (int ignored_exec_when, const char *text)
	// helper for plugins to not need separate logic
	G_LOCATE_GAME_DATA,				// void (gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient)
	G_GET_ENTITY_TOKEN,				// qboolean (char *buffer, int bufferSize)
};

#endif // __QMM2_GAME_STVOYSP_H__
