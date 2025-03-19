/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_GAME_API_H__
#define __QMM2_GAME_API_H__

typedef int (*vmsyscall_t)(unsigned char* membase, int cmd, int* args);
typedef const char* (*msgname_t)(int msg);
#ifdef QMM_GETGAMEAPI_SUPPORT
typedef void* (*apientry_t)(void* import);
#endif // QMM_GETGAMEAPI_SUPPORT

// a list of all the mod messages used by QMM
typedef enum {
	QMM_GAME_INIT,
	QMM_GAME_SHUTDOWN,
	QMM_GAME_CONSOLE_COMMAND,
	QMM_GAME_CLIENT_CONNECT,
	QMM_GAME_CLIENT_COMMAND,

	QMM_GAME_MSG_MAX
} qmm_mod_msg_t;

// a list of all the engine messages used by QMM
typedef enum {
	QMM_G_PRINT,
	QMM_G_ERROR,
	QMM_G_ARGV,
	QMM_G_ARGC,
	QMM_G_SEND_CONSOLE_COMMAND,
	QMM_G_SEND_SERVER_COMMAND,

	QMM_G_CVAR_REGISTER,
	QMM_G_CVAR_SET,
	QMM_G_CVAR_VARIABLE_STRING_BUFFER,
	QMM_G_CVAR_VARIABLE_INTEGER_VALUE,

	QMM_G_FS_FOPEN_FILE,
	QMM_G_FS_READ,
	QMM_G_FS_WRITE,
	QMM_G_FS_FCLOSE_FILE,
	
	QMM_EXEC_APPEND,
	QMM_FS_READ,
	QMM_FS_APPEND,
	QMM_FS_APPEND_SYNC,

	QMM_CVAR_SERVERINFO,
	QMM_CVAR_ROM,
	QMM_CVAR_ARCHIVE,

	QMM_G_MSG_MAX
} qmm_eng_msg_t;

// some information for each game engine supported by QMM
typedef struct {
	const char* dllname;			// default dll mod filename
	const char* qvmname;			// default qvm mod filename (NULL = qmm_<dllname>)
	const char* moddir;				// default moddir name
	const char* gamename_long;		// long, descriptive, game name
	const char* gamename_short;		// short initials for game
	int* qmm_eng_msgs;				// array of engine messages used by QMM
	int* qmm_mod_msgs;				// array of mod messages used by QMM
	msgname_t eng_msg_names;		// pointer to a function that returns a string for a given engine message
	msgname_t mod_msg_names;		// pointer to a function that returns a string for a given mod message
	const char** exe_hints;			// array of hints that should appear in the executable filename to be considered a game match
	vmsyscall_t vmsyscall;			// pointer to a function that handles mod->engine calls from a VM (NULL = not required)	
#ifdef QMM_GETGAMEAPI_SUPPORT
	apientry_t apientry;			// pointer to a function that handles GetGameAPI entry for a game
#else
	void* reserved;					// eat the pointer in the struct
#endif
	int max_syscall_args;			// max number of syscall args that this game needs (unused for now, but nice to have easily available)
	int max_vmmain_args;			// max number of vmmain args that this game needs (unused for now, but nice to have easily available)
} supportedgame_t;

extern supportedgame_t g_supportedgames[];

// macros to make game support a bit easier to do
// these macros are used in game_api.cpp and game_xyz.cpp

// generate externs for the msg arrays and functions
#define GEN_EXTS(game)		extern int game##_qmm_eng_msgs[]; \
							extern int game##_qmm_mod_msgs[]; \
							const char* game##_eng_msg_names(int msg); \
							const char* game##_mod_msg_names(int msg); \
							const char* game##_exe_hints[]

// generate extern for the vmsyscall function (if game supports it)
#define GEN_VMEXT(game)		int game##_vmsyscall(unsigned char* membase, int cmd, int* args)

#ifdef QMM_GETGAMEAPI_SUPPORT
// generate extern for the GetGameAPI function (if game supports it)
#define GEN_APIEXT(game)	void* game##_GetGameAPI(void* import)
#endif // QMM_GETGAMEAPI_SUPPORT

// generate struct info for the short name, messages arrays, and message name functions
#define GEN_INFO(game)		#game, game##_qmm_eng_msgs, game##_qmm_mod_msgs, game##_eng_msg_names, game##_mod_msg_names, game##_exe_hints

// generate a case/string line for the message name functions
#define GEN_CASE(x)			case x: return #x
 
// macro to easily output game-specific message values to match the qmm_eng_msg_t and qmm_mod_msg_t enums above
// this macro goes in game_*.cpp
#define GEN_QMM_MSGS(game) \
	int game##_qmm_eng_msgs[] = { \
		G_PRINT, G_ERROR, G_ARGV, G_ARGC, G_SEND_CONSOLE_COMMAND, G_SEND_SERVER_COMMAND, \
		G_CVAR_REGISTER, G_CVAR_SET, G_CVAR_VARIABLE_STRING_BUFFER, G_CVAR_VARIABLE_INTEGER_VALUE, \
		G_FS_FOPEN_FILE, G_FS_READ, G_FS_WRITE, G_FS_FCLOSE_FILE, \
		EXEC_APPEND, FS_READ, FS_APPEND, FS_APPEND_SYNC, \
		CVAR_SERVERINFO, CVAR_ROM, CVAR_ARCHIVE \
	}; \
	int game##_qmm_mod_msgs[] = { \
		GAME_INIT, GAME_SHUTDOWN, GAME_CONSOLE_COMMAND, GAME_CLIENT_CONNECT, GAME_CLIENT_COMMAND \
	}

// macro to create an exe hint array
#define GEN_EXE_HINTS(game)		const char* game##_exe_hints[]
 
// these macros handle qvm syscall arguments in GAME_vmsyscall functions in game_*.cpp
// this gets an argument value
#define vmarg(x)	(args[(x)])
// this adds the base VM address to a given value
#define vmadd(x)	((x) ? (x) + (int)membase : NULL)
// this subtracts the base VM address from the given value
#define vmsub(x)	((x) ? (x) - (int)membase : NULL)
// this adds the base VM address to an argument value
#define vmptr(x)	(vmadd(vmarg(x)))

#endif // __QMM2_GAME_API_H__
