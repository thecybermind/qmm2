/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_GAME_CODMP_H__
#define __QMM2_GAME_CODMP_H__

#include <stdint.h>     // intptr_t

#define GAME_API_VERSION 5

// syscall cmds
enum {
	G_PRINTF,							// ( const char* msg );
	G_ERROR,							// ( const char* err );
	G_ERROR_LOCALIZED,					// ( const char* err );
	G_MILLISECONDS,						// ( void );
	G_CVAR_REGISTER,					// ( vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags );
	G_CVAR_UPDATE,						// ( vmCvar_t* vmCvar);
	G_CVAR_SET,							// ( const char* var_name, const char* value);
	G_CVAR_VARIABLE_INTEGER_VALUE,		// ( const char *var_name );
	G_CVAR_VARIABLE_VALUE,				// float ( const char *var_name );
	G_CVAR_VARIABLE_STRING_BUFFER,		// ( const char *var_name, char *buffer, int bufsize );
	G_ARGC,								// ( void );
	G_ARGV,								// ( int n, char *buffer, int bufferLength );
	G_HUNK_ALLOCINTERNAL,				// ? ( int size );
	G_HUNK_ALLOCLOWINTERNAL,			// ? ( int size );
	G_HUNK_ALLOCALIGNINTERNAL,			// ? ( int size, int align );
	G_HUNK_ALLOCLOWALIGNINTERNAL,		// ? ( int size, int align );
	G_HUNK_ALLOCATETEMPMEMORYINTERNAL,	// ? ( int size ) ;
	G_HUNK_FREETEMPMEMORYINTERNAL,		// ? ( void* mem );
	G_FS_FOPEN_FILE,					// ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	G_FS_READ,							// ( void *buffer, int len, fileHandle_t f );
	G_FS_WRITE,							// ( const void *buffer, int len, fileHandle_t f );
	G_FS_RENAME,						// ( char* from, char* to );
	G_FS_FCLOSE_FILE,					// ( fileHandle_t f );
	G_SEND_CONSOLE_COMMAND,				// ( int when, const char *text );
	G_LOCATE_GAME_DATA,					// ( gentity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient );
	G_GET_GUID,							// char* ( int clientNum );
	G_DROP_CLIENT,						// ( int clientNum, const char* reason );
	G_SEND_SERVER_COMMAND,				// ( int clientNum, svscmd_type type, const char *text );
	G_SET_CONFIGSTRING,					// ( int num, const char* string );
	G_GET_CONFIGSTRING,					// ( int num, char* buffer, int bufferSize );
	G_GET_CONFIGSTRING_CONST,			// unsigned int ( int num );
	G_ISLOCALCLIENT,					// bool ( int clientNum );
	G_GET_CLIENT_PING,					// int ( int clientNum );
	G_GET_USERINFO,						// ( int num, char* buffer, int bufferSize );
	G_SET_USERINFO,						// ( int num, const char* buffer );
	G_GET_SERVERINFO,					// ( char* buffer, int bufferSize );
	G_SET_BRUSH_MODEL,					// ( gentity_t* ent, const char* name );
	G_TRACE,							// ( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	G_TRACECAPSULE,						// ( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	G_SIGHTTRACE,						// ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int passEntityNum2, int contentMask, qboolean cylinder );
	G_SIGHTTRACE_CAPSULE,				// ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int passEntityNum2, int contentMask, qboolean cylinder );
	G_SIGHTTRACEENTITY,					// int ( float *start, float *mins, float *maxs, float *end, int entityNum, int contentmask );
	G_CM_BOXTRACE,						// ( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
	G_CM_CAPSULETRACE,					// ( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
	G_CM_BOXSIGHTTRACE,					// ? 6 args
	G_CM_CAPSULESIGHTTRACE,				// ? 6 args
	G_LOCATIONAL_TRACE,					// ? 6 args
	G_POINT_CONTENTS,					// int ( float *p, int passEntityNum, int contentmask )
	G_IN_PVS,							// ( const vec3_t p1, const vec3_t p2 );
	G_IN_PVS_IGNORE_PORTALS,			// ( const vec3_t p1, const vec3_t p2 );
	G_IN_SNAPSHOT,						// ? 3 args
	G_ADJUST_AREA_PORTAL_STATE,			// ? 2 args
	G_AREAS_CONNECTED,					// ( int area1, int area2 );
	G_LINKENTITY,						// ( gentity_t* ent );
	G_UNLINKENTITY,						// ( gentity_t* ent );
	G_ENTITIES_IN_BOX,					// ? 5 args ( const vec3_t mins, const vec3_t maxs, gentity_t* *list, int maxcount );
	G_ENTITY_CONTACT,					// ( const vec3_t mins, const vec3_t maxs, const gentity_t* ent );
	G_GET_USERCMD,						// ( int clientNum, usercmd_t* cmd );
	G_GET_ENTITY_TOKEN,					// ( char* buffer, int bufferSize );
	G_FS_GETFILELIST,					// ( const char *path, const char *extension, char *listbuf, int bufsize );
	G_MAPEXISTS,						// bool ( char* map )
	G_REAL_TIME,						// ( qtime_t* qtime )
	G_SNAPVECTOR,						// ( float* v )
	G_ENTITY_CONTACTCAPSULE,			// ( const vec3_t mins, const vec3_t maxs, const gentity_t* ent );
	G_SOUNDALIASSTRING,					// ? ( const char* sound );
	G_PICKSOUNDALIAS,					// ( gentity_s *ent, unsigned char index );
	G_SOUNDALIASINDEX,					// ( const char* name );
	G_SURFACETYPEFROMNAME,				// ( const char* surfacename );
	G_SURFACETYPETONAME,				// ( unsigned int surfaceType );
	G_ADDTESTCLIENT,					// ? 2 args
	G_ARCHIVED_CLIENTINFO,				// ? 4 args
	G_ADDDEBUGSTRING,					// ? 4 args
	G_ADDDEBUGLINE,						// ? 5 args
	G_SET_ARCHIVE,						// ? ( qboolean archive );
	G_ZMALLOC_INTERNAL,					// ? ( int size ); 
	G_ZFREE_INTERNAL,					// ? ( void* mem );
	G_XANIM_CREATETREE,					// ? ( int size ); 
	G_XANIM_CREATESMALLTREE,			// ? ( int size );
	G_XANIM_FREESMALLTREE,				// ? ( void* tree );
	G_XMODELEXISTS,						// ? ( xmodel );
	G_XMODELGET,						// ? ( xmodel );
	G_DOBJ_CREATE,						// ? 5 args
	G_DOBJ_EXISTS,						// ? ( void* dobj );
	G_DOBJ_SAFEFREE,					// ? 2 args
	G_XANIM_GETANIMS,					// ? 1 arg
	G_UNUSED_85,
	G_XANIM_CLEARTREEGOALWEIGHTS,		// ? 3 args
	G_XANIM_CLEARGOALWEIGHT,			// ? 3 args
	G_XANIM_CLEARTREEGOALWEIGHTSSTRICT,	// ? 3 args
	G_XANIM_SETCOMPLETEGOALWEIGHTKNOB,	// ? 7 args
	G_XANIM_SETCOMPLETEGOALWEIGHTKNOBALL,	// ? 8 args
	G_XANIM_SETANIMRATE,				// ? 3 args
	G_XANIM_SETTIME,					// ? 3 args
	G_XANIM_SETGOALWEIGHTKNOB,			// ? 7 args
	G_XANIM_CLEARTREE,					// ? 1 arg
	G_XANIM_HASTIME,					// ? 2 args
	G_XNAIM_ISPRIMITIVE,				// ? 2 args
	G_XANIM_GETLENGTH,					// ? 2 args
	G_XANIM_GETLENGTHSECONDS,			// ? 2 args, return float?
	G_XANIM_SETCOMPLETEGOALWEIGHT,		// ? 7 args
	G_XANIM_SETGOALWEIGHT,				// ? 7 args
	G_XANIM_CALCABSDELTA,				// ? 4 args
	G_XANIM_CALCDELTA,					// ? 5 args
	G_XANIM_GETRELDELTA,				// ? 6 args
	G_XANIM_GETABSDELTA,				// ? 5 args
	G_XANIM_ISLOOPED,					// ? 2 args
	G_XANIM_NOTETRACKEXISTS,			// ? 3 args
	G_XANIM_GETTIME,					// ? 2 args, return float?
	G_XANIM_GETWEIGHT,					// ? 2 args
	G_DOBJ_DUMPINFO,					// ? 1 arg
	G_DOBJ_CREATESKELFORBONE,			// ? 2 args
	G_DOBJ_CREATESKELFORBONES,			// ? 2 args
	G_DOBJ_UPDATESERVERTIME,			// ? 3 args
	G_DOBJ_INITSERVERTIME,				// ? 2 args
	G_DOBJ_GETHIERARCHYBITS,			// ? 3 args
	G_DOBJ_CALCANIM,					// ? 2 args
	G_DOBJ_CALCSKEL,					// ? 2 args
	G_XANIM_LOADANIMTREE,				// ? 1 arg
	G_XANIM_SAVEANIMTREE,				// ? 1 arg
	G_XANIM_CLONEANIMTREE,				// ? 2 args
	G_DOBJ_NUMBONES,					// ? 1 arg
	G_DOBJ_GETBONEINDEX,				// ? 2 args
	G_DOBJ_GETMATRIXARRAY,				// ? 1 arg
	G_DOBJ_DISPLAYANIM,					// ? 1 arg
	G_XANIM_HASFINISHED,				// ? 2 args
	G_XANIM_GETNUMCHILDREN,				// ? 2 args
	G_XANIM_GETCHILDAT,					// ? 3 args
	G_XMODEL_NUMBONES,					// ? 1 arg
	G_XMODEL_GETBONENAMES,				// ? 1 arg
	G_DOBJ_GETROTTRANSARRAY,			// ? 1 arg
	G_DOBJ_SETROTTRANSARRAY,			// ? 3 args
	G_DOBJ_SETCONTROLROTTRANSINDEX,		// ? 3 args
	G_DOBJ_GETBOUNDS,					// ? 3 args
	G_XANIM_GETANIMNAME,				// ? 2 args
	G_DOBJ_GETTREE,						// ? 1 arg
	G_XANIM_GETANIMTREESIZE,			// ? 1 arg
	G_XMODEL_DEBUGBOXES,				// ? 1 arg
	G_GETWEAPONINFOMEMORY,				// ? 2 args
	G_FREEWEAPONINFOMEMORY,				// ( int source );
	G_FREECLIENTSCRIPTPERS,				// ? ( void );
	G_RESETENTITYPARSEPOINT,			// ( void );

	G_PRINT = G_PRINTF,
};

// vmMain cmds
enum {
	GAME_DEFAULT_0,					// return -1;
	GAME_GET_APIVERSION,			// return GAME_API_VERSION;
	GAME_INIT,						// ( int levelTime, int randomSeed, int restart, int savepersist );
	GAME_SHUTDOWN,					// ( qboolean isRestart );
	GAME_CLIENT_CONNECT,			// char* ( int clientNum, qboolean firstTime );
	GAME_CLIENT_BEGIN,				// ( int clientNum );
	GAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum );
	GAME_CLIENT_DISCONNECT,			// ( int clientNum );
	GAME_CLIENT_COMMAND,			// ( int clientNum );
	GAME_CLIENT_THINK,				// ( int clientNum );
	GAME_GET_FOLLOWPLAYERSTATE,		// int ( int clientNum, playerState_s *ps );
	GAME_UPDATE_CVARS,				// ( void );
	GAME_RUN_FRAME,					// ( int levelTime );
	GAME_CONSOLE_COMMAND,			// int ( void );
	GAME_SCR_FARHOOK,				// ? int ( int arg0 );
	GAME_DOBJ_CALCPOSE,				// ( int clientNum );
	GAME_GET_NUMWEAPONS,			// int ( void );
	GAME_SET_SAVEPERSIST,			// ( int savepersist ); stores arg in global level.savepersist
	GAME_GET_SAVEPERSIST,			// int ( void ); returns global level.savepersist
	GAME_GET_CLIENTSTATE,			// clientState_s* ( int clientNum ); returns &level.clients[clientNum].sess.cs;
	GAME_GET_CLIENTARCHIVETIME,		// int ( int clientNum ); returns level.clients[clientNum].sess.archiveTime
	GAME_SET_CLIENTARCHIVETIME,		// ( int clientNum, int time ); stores time in level.clients[clientNum].sess.archiveTime
	GAME_GET_CLIENTSCORE,			// int ( int clientNum ); returns level.clients[clientNum].sess.score;
	GAME_GET_FOG_DISTANCE,			// float ( void ); returns global float level.fFogOpaqueDistSqrd
};

enum fsMode_t {
	FS_READ = 0x0,
	FS_WRITE = 0x1,
	FS_APPEND = 0x2,
	FS_APPEND_SYNC = 0x3,
};

// parameters for adding commands to command buffer
typedef enum {
	EXEC_NOW,			// don't return until completed, a VM should NEVER use this,
						// because some commands might cause the VM to be unloaded...
	EXEC_INSERT,		// insert at current position, but don't run yet
	EXEC_APPEND			// add to end of the command buffer (normal case)
} cbufExec_t;

enum CvarFlags {
	CVAR_NOFLAG             = 0x0,
	CVAR_ARCHIVE            = 0x1,  // will be saved to config(_mp).cfg
	CVAR_USERINFO           = 0x2,  // sent to server on connect or change
	CVAR_SERVERINFO         = 0x4,  // sent in response to front end requests
	CVAR_SYSTEMINFO         = 0x8,  // this is sent (replicated) to all clients if you are host
	CVAR_INIT               = 0x10, // don't allow change from console at all (i think)
	CVAR_LATCH              = 0x20, // will only change when C code next does
                                    // a Cvar_Get(), so it can't be changed
                                    // without proper initialization.  modified
                                    // will be set, even though the value hasn't changed yet
	
	CVAR_ROM                = 0x40, // display only, cannot be set by user at all (can be set by code)
	CVAR_CHEAT              = 0x80, // can not be changed if cheats are disabled

	// CVAR_AUTOEXEC, CVAR_SAVED, and CVAR_CHANGEABLE_RESET are not 100% sure
	CVAR_TEMP               = 0x100,
	CVAR_AUTOEXEC           = 0x200,
	CVAR_NORESTART          = 0x400,    // do not clear when a cvar_restart is issued
	CVAR_SAVED              = 0x1000,
	CVAR_EXTERNAL           = 0x4000,   // created by a set command or setclientdvar
	CVAR_CHANGEABLE_RESET   = 0x8000,
};

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		8192
#define	BIG_INFO_VALUE		8192

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

typedef intptr_t fileHandle_t;

#endif // __QMM2_GAME_CODMP_H__
