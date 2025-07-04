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

#include <vector>
#include <string>
#include <stdarg.h>
#include "osdef.h"

typedef const char* (*msgname_t)(intptr_t msg);
typedef int (*vmsyscall_t)(std::byte* membase, int cmd, int* args);
typedef void* (*apientry_t)(void* import);

// a list of all the mod messages used by QMM
typedef enum {
	QMM_GAME_INIT,
	QMM_GAME_SHUTDOWN,
	QMM_GAME_CONSOLE_COMMAND,
	QMM_GAME_CLIENT_CONNECT
} qmm_mod_msg_t;

// a list of all the engine messages used by QMM
typedef enum {
	QMM_G_PRINT,
	QMM_G_ERROR,
	QMM_G_ARGV,
	QMM_G_ARGC,
	QMM_G_SEND_CONSOLE_COMMAND,

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

	QMM_CVAR_SERVERINFO,
	QMM_CVAR_ROM,

	QMM_G_GET_CONFIGSTRING,
} qmm_eng_msg_t;

// some information for each game engine supported by QMM
typedef struct {
	const char* dllname;				// default dll mod filename
	const char* suffix;					// dll suffix
	const char* qvmname;				// default qvm mod filename (NULL = qmm_<dllname>)
	const char* moddir;					// default moddir name
	const char* gamename_long;			// long, descriptive, game name

	// this section is made by GEN_INFO(GAME)
	const char* gamename_short;			// short initials for game
	int* qmm_eng_msgs;					// array of engine messages used by QMM
	int* qmm_mod_msgs;					// array of mod messages used by QMM
	msgname_t eng_msg_names;			// pointer to a function that returns a string for a given engine message
	msgname_t mod_msg_names;			// pointer to a function that returns a string for a given mod message

	vmsyscall_t vmsyscall;				// pointer to a function that handles mod->engine calls from a VM (NULL = not required)	
	apientry_t apientry;				// pointer to a function that handles GetGameAPI entry for a game
	int max_syscall_args;				// max number of syscall args that this game needs (unused for now, but nice to have easily available)
	int max_vmmain_args;				// max number of vmmain args that this game needs (unused for now, but nice to have easily available)
	std::vector<std::string> exe_hints;	// array of hints that should appear in the executable filename to be considered a game match
} supportedgame_t;

extern supportedgame_t g_supportedgames[];

// macros to make game support a bit easier to do
// these macros are used in game_api.cpp and game_xyz.cpp

// generate externs for each game's msg arrays and functions
#define GEN_EXTS(game)		extern int game##_qmm_eng_msgs[]; \
							extern int game##_qmm_mod_msgs[]; \
							const char* game##_eng_msg_names(intptr_t msg); \
							const char* game##_mod_msg_names(intptr_t msg); \
							int game##_vmsyscall(std::byte* membase, int cmd, int* args); \
							void* game##_GetGameAPI(void* import)

// generate struct info for the short name, messages arrays, and message name functions
#define GEN_INFO(game)		#game, game##_qmm_eng_msgs, game##_qmm_mod_msgs, game##_eng_msg_names, game##_mod_msg_names

// generate a case/string line for the message name functions
#define GEN_CASE(x)			case x: return #x
 
// macro to easily output game-specific message values to match the qmm_eng_msg_t and qmm_mod_msg_t enums above
// this macro goes in game_*.cpp
#define GEN_QMM_MSGS(game) \
	int game##_qmm_eng_msgs[] = { \
		G_PRINT, G_ERROR, G_ARGV, G_ARGC, G_SEND_CONSOLE_COMMAND, \
		G_CVAR_REGISTER, G_CVAR_SET, G_CVAR_VARIABLE_STRING_BUFFER, G_CVAR_VARIABLE_INTEGER_VALUE, \
		G_FS_FOPEN_FILE, G_FS_READ, G_FS_WRITE, G_FS_FCLOSE_FILE, \
		EXEC_APPEND, FS_READ, \
		CVAR_SERVERINFO, CVAR_ROM, \
		G_GET_CONFIGSTRING, \
	}; \
	int game##_qmm_mod_msgs[] = { \
		GAME_INIT, GAME_SHUTDOWN, GAME_CONSOLE_COMMAND, GAME_CLIENT_CONNECT \
	}

// ----------------------------
// ----- API vararg stuff -----
// ----------------------------

#define QMM_MAX_VMMAIN_ARGS     9
#define QMM_GET_VMMAIN_ARGS()   intptr_t args[QMM_MAX_VMMAIN_ARGS] = {}; \
                                va_list arglist; \
                                va_start(arglist, cmd); \
                                for (int i = 0; i < QMM_MAX_VMMAIN_ARGS; ++i) \
                                    args[i] = va_arg(arglist, intptr_t); \
                                va_end(arglist)

#define QMM_MAX_SYSCALL_ARGS    17
#define QMM_GET_SYSCALL_ARGS()  intptr_t args[QMM_MAX_SYSCALL_ARGS] = {}; \
                                va_list arglist; \
                                va_start(arglist, cmd); \
                                for (int i = 0; i < QMM_MAX_SYSCALL_ARGS; ++i) \
                                    args[i] = va_arg(arglist, intptr_t); \
                                va_end(arglist)

// ----------------------------
// ----- GetGameAPI stuff -----
// ----------------------------

// used by GetGameAPI code as a cast for generic syscall/vmmain calls
typedef intptr_t(*pfn_call_t)(intptr_t arg0, ...);

// handle calls from QMM and plugins into the engine
#define ROUTE_IMPORT(field, cmd)		case cmd: ret = ((pfn_call_t)(orig_import. field))(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]); break
#define ROUTE_IMPORT_VAR(field, cmd)	case cmd: ret = (intptr_t)&(orig_import. field); break

// handle calls from QMM and plugins into the mod
#define ROUTE_EXPORT(field, cmd)		case cmd: ret = ((pfn_call_t)(orig_export-> field))(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]); break
#define ROUTE_EXPORT_VAR(field, cmd)	case cmd: ret = (intptr_t)&(orig_export-> field); break

// handle calls from engine or mod into QMM
#define GEN_IMPORT(field, cmd)	(decltype(qmm_import. field)) +[](intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16) { return qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16); }
#define GEN_EXPORT(field, cmd)	(decltype(qmm_export. field)) +[](intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8) { return vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }
// if the syscall lambda types matter (float args in 64-bit games like Q2R), use these
// macros to easily generate a lambda with full return and argument type information:
// e.g. GEN_IMPORT_2_VOID(G_DEBUGGRAPH, float, int)
// e.g. GEN_IMPORT_2(G_ANIM_TIME, float, dtiki_t*, int)
#if defined(__LP64__) || defined(_WIN64)
#define  GEN_IMPORT_0(field, cmd, typeret) +[]() -> typeret { return (typeret)qmm_syscall(cmd); }
#define  GEN_IMPORT_1(field, cmd, typeret, type0) +[](type0 arg0) -> typeret { return (typeret)qmm_syscall(cmd, arg0); }
#define  GEN_IMPORT_2(field, cmd, typeret, type0, type1) +[](type0 arg0, type1 arg1) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1); }
#define  GEN_IMPORT_3(field, cmd, typeret, type0, type1, type2) +[](type0 arg0, type1 arg1, type2 arg2) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2); }
#define  GEN_IMPORT_4(field, cmd, typeret, type0, type1, type2, type3) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3); }
#define  GEN_IMPORT_5(field, cmd, typeret, type0, type1, type2, type3, type4) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4); }
#define  GEN_IMPORT_6(field, cmd, typeret, type0, type1, type2, type3, type4, type5) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5); }
#define  GEN_IMPORT_7(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6); }
#define  GEN_IMPORT_8(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7); }
#define  GEN_IMPORT_9(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }
#define GEN_IMPORT_10(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); }
#define GEN_IMPORT_11(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); }
#define GEN_IMPORT_12(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11); }
#define GEN_IMPORT_13(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12); }
#define GEN_IMPORT_14(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13); }
#define GEN_IMPORT_15(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14); }
#define GEN_IMPORT_16(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14, type15) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14, type15 arg15) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15); }
#define GEN_IMPORT_17(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14, type15, type16) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14, type15 arg15, type16 arg16) -> typeret { return (typeret)qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16); }
#else
#define  GEN_IMPORT_0(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_1(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_2(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_3(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_4(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_5(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_6(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_7(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_8(field, cmd, ...) GEN_IMPORT(field, cmd)
#define  GEN_IMPORT_9(field, cmd, ...) GEN_IMPORT(field, cmd)
#define GEN_IMPORT_10(field, cmd, ...) GEN_IMPORT(field, cmd)
#define GEN_IMPORT_11(field, cmd, ...) GEN_IMPORT(field, cmd)
#define GEN_IMPORT_12(field, cmd, ...) GEN_IMPORT(field, cmd)
#define GEN_IMPORT_13(field, cmd, ...) GEN_IMPORT(field, cmd)
#define GEN_IMPORT_14(field, cmd, ...) GEN_IMPORT(field, cmd)
#define GEN_IMPORT_15(field, cmd, ...) GEN_IMPORT(field, cmd)
#define GEN_IMPORT_16(field, cmd, ...) GEN_IMPORT(field, cmd)
#define GEN_IMPORT_17(field, cmd, ...) GEN_IMPORT(field, cmd)
#endif

// ---------------------
// ----- QVM stuff -----
// ---------------------

// these macros handle qvm syscall arguments in GAME_vmsyscall functions in game_*.cpp

// this gets an argument value (evaluate to an intptr_t)
#define vmarg(x)	(intptr_t)args[x]
// this adds the base VM address pointer to an argument value (evaluate to a pointer)
#define vmptr(x)	(args[x] ? membase + args[x] : nullptr)
// this subtracts the base VM address pointer from a value, for returning from syscall (this should evaluate to an int)
#define vmret(x)	(int)(x ? (intptr_t)x - (intptr_t)membase : 0)
#endif // __QMM2_GAME_API_H__
