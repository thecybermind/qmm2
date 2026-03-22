/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAME_API_H
#define QMM2_GAME_API_H

#include "version.h"
#include <cstdint>  // intptr_t
#include <cstdarg>
#include <vector>
#include "qmmapi.h"


// mod suffixes and extensions

// SUF_DLL: standard suffixes for mod DLLs per OS/arch ("x86", "i386", "x86_64", "x86_64")
// X64_SUF_DLL: ioRTCW and ET:Legacy use "x64" suffix for 64-bit windows instead of "x86_64"
// EXT_DLL: standard extensions for mod DLLs per OS (".dll", ".so")
// SP_DLL + MP_DLL: COD, RTCWMP & WET filename changes between linux/windows
// UO_DLL: CODUO filename changes between linux/windows

// EXT_QVM: standard extension for mod QVM (".qvm")
// MOD_DLL: concatenation of SUF_DLL and EXT_DLL
// X64_DLL: concatenation of X64_SUF_DLL and EXT_DLL

#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
 #define SUF_DLL "x86_64"
 #define X64_SUF_DLL "x64"
#elif defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_32)
 #define SUF_DLL "x86"
 #define X64_SUF_DLL "x86"
#elif defined(QMM_OS_LINUX) && defined(QMM_ARCH_64)
 #define SUF_DLL "x86_64"
 #define X64_SUF_DLL "x86_64"
#elif defined(QMM_OS_LINUX) && defined(QMM_ARCH_32)
 #define SUF_DLL "i386"
 #define X64_SUF_DLL "i386" 
#else
 #error Unknown architecture + OS combination
#endif

#if defined(QMM_OS_WINDOWS)
 #define EXT_DLL ".dll"
 #define SP_DLL "_sp_"
 #define MP_DLL "_mp_"
 #define UO_DLL "uo_game" MP_DLL
#elif defined(QMM_OS_LINUX)
 #define EXT_DLL ".so"
 #define SP_DLL ".sp."
 #define MP_DLL ".mp."
 #define UO_DLL "game" MP_DLL "uo."
#endif 

#define EXT_QVM ".qvm"
#define MOD_DLL SUF_DLL EXT_DLL
#define X64_DLL X64_SUF_DLL EXT_DLL

// engine/mod API type
enum APIType {
    QMM_API_ERROR,

    QMM_API_QVM,			// mod-only

    QMM_API_DLLENTRY,
    QMM_API_GETGAMEAPI,
    QMM_API_GETMODULEAPI,
};
const char* APIType_Name(APIType);		// return the string form of the enum
const char* APIType_Function(APIType);	// return the entry function name

// a list of all the engine messages/constants used by QMM. if you change this, update the GEN_GAME_QMM_*_MSGS macros
enum {
    // general purpose
    QMM_G_PRINT, QMM_G_ERROR, QMM_G_ARGV, QMM_G_ARGC, QMM_G_SEND_CONSOLE_COMMAND, QMM_G_GET_CONFIGSTRING,
    // cvars
    QMM_G_CVAR_REGISTER, QMM_G_CVAR_VARIABLE_STRING_BUFFER, QMM_G_CVAR_VARIABLE_INTEGER_VALUE, QMM_CVAR_SERVERINFO, QMM_CVAR_ROM,
    // files
    QMM_G_FS_FOPEN_FILE, QMM_G_FS_READ, QMM_G_FS_WRITE, QMM_G_FS_FCLOSE_FILE, QMM_EXEC_APPEND, QMM_FS_READ,

    // array size
    QMM_ENGINE_MSG_COUNT,
};

// a list of all the mod messages used by QMM. if you change this, update the GEN_GAME_QMM_MSGS macro
enum {
    QMM_GAME_INIT, QMM_GAME_SHUTDOWN, QMM_GAME_CONSOLE_COMMAND,

    // array size
    QMM_MOD_MSG_COUNT,
};

// macros to output game-specific message values to match the QMM_ enums above
#define GEN_GAME_QMM_ENG_MSGS() \
	{ \
		G_PRINT, G_ERROR, G_ARGV, G_ARGC, G_SEND_CONSOLE_COMMAND, G_GET_CONFIGSTRING, \
		G_CVAR_REGISTER, G_CVAR_VARIABLE_STRING_BUFFER, G_CVAR_VARIABLE_INTEGER_VALUE, CVAR_SERVERINFO, CVAR_ROM, \
		G_FS_FOPEN_FILE, G_FS_READ, G_FS_WRITE, G_FS_FCLOSE_FILE, EXEC_APPEND, FS_READ, \
	}
#define GEN_GAME_QMM_MOD_MSGS() \
	{ \
		GAME_INIT, GAME_SHUTDOWN, GAME_CONSOLE_COMMAND, \
	}

// pure virtual base class for game support
// need to implement all functions except:
// * DefaultQVMName - only need if the game supports QVMs. default will return nullptr (this is how QMM determines support)
// * QVMSyscall - only need if the game supports QVMs. default returns 0
// need to implement qmm_eng_msgs and qmm_mod_msgs (use GEN_GAME_QMM_ENG_MSGS() and GEN_GAME_QMM_MOD_MSGS() macros)
struct GameSupport {
    virtual const char* EngMsgName(intptr_t) = 0;
    virtual const char* ModMsgName(intptr_t) = 0;
    virtual bool AutoDetect(APIType) = 0;
    virtual void* Entry(void*, void*, APIType) = 0;
    virtual bool ModLoad(void*, APIType) = 0;
    virtual void ModUnload() = 0;
    virtual int QMMEngMsg(int msg) = 0;
    virtual int QMMModMsg(int msg) = 0;

    virtual intptr_t syscall(intptr_t, ...) = 0;
    virtual intptr_t vmMain(intptr_t, ...) = 0;

    virtual const char* DefaultDLLName() = 0;
    virtual const char* DefaultQVMName() { return nullptr; }
    virtual const char* DefaultModDir() = 0;
    virtual const char* GameName() = 0;
    virtual const char* GameCode() = 0;

    virtual int QVMSyscall(uint8_t*, int, int*) { return 0; }
};

extern std::vector<GameSupport*> api_supportedgames;

// macros to make game support a bit easier to do

// generate extern for each game's support object (used at the top of game_api.cpp)
#define GEN_GAME_EXTS(game)	extern GameSupport* game##_gamesupport

// generate game support object (used at the top of game_XYZ.cpp)
#define GEN_GAME_OBJ(game) static game##_GameSupport gamesupport; GameSupport* game##_gamesupport = &gamesupport

// reference game support object in api_supportedgames list
#define GET_GAME_OBJ(game) game##_gamesupport

// generate a case/string line for use in the *MsgNames functions
#define GEN_CASE(x)		case x: return #x

// cache some dynamic message values that get evaluated a lot
extern intptr_t msg_G_PRINT, msg_GAME_INIT, msg_GAME_CONSOLE_COMMAND, msg_GAME_SHUTDOWN;

// ----------------------------
// ----- API vararg stuff -----
// ----------------------------

// max amount of vmMain args for games that support QVM
constexpr int QVM_MAX_VMMAIN_ARGS = 6;

constexpr int QMM_MAX_VMMAIN_ARGS = 9;
#define QMM_GET_VMMAIN_ARGS()   intptr_t args[QMM_MAX_VMMAIN_ARGS] = {}; \
                                va_list arglist; \
                                va_start(arglist, cmd); \
                                for (int i = 0; i < QMM_MAX_VMMAIN_ARGS; ++i) \
                                    args[i] = va_arg(arglist, intptr_t); \
                                va_end(arglist)
#define QMM_PUT_VMMAIN_ARGS()	args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]

constexpr int QMM_MAX_SYSCALL_ARGS = 17;
#define QMM_GET_SYSCALL_ARGS()  intptr_t args[QMM_MAX_SYSCALL_ARGS] = {}; \
                                va_list arglist; \
                                va_start(arglist, cmd); \
                                for (int i = 0; i < QMM_MAX_SYSCALL_ARGS; ++i) \
                                    args[i] = va_arg(arglist, intptr_t); \
                                va_end(arglist)
#define QMM_PUT_SYSCALL_ARGS()	args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]

// ----------------------------
// ----- GetGameAPI stuff -----
// ----------------------------

// handle calls from QMM and plugins into the engine
#define ROUTE_IMPORT(field, cmd)		case cmd: ret = ((eng_syscall)(orig_import. field))(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]); break
#define ROUTE_IMPORT_VAR(field, cmd)	case cmd: ret = (intptr_t)&(orig_import. field); break

#define ROUTE_IMPORT_0_V(field, cmd) case cmd: orig_import. field(); break
#define ROUTE_IMPORT_1_V(field, cmd, type0) case cmd: orig_import. field((type0)args[0]); break
#define ROUTE_IMPORT_2_V(field, cmd, type0, type1) case cmd: orig_import. field((type0)args[0], (type1)args[1]); break
#define ROUTE_IMPORT_3_V(field, cmd, type0, type1, type2) case cmd: orig_import. field((type0)args[0], (type1)args[1], (type2)args[2]); break
#define ROUTE_IMPORT_4_V(field, cmd, type0, type1, type2, type3) case cmd: orig_import. field((type0)args[0], (type1)args[1], (type2)args[2], (type3)args[3]); break
#define ROUTE_IMPORT_5_V(field, cmd, type0, type1, type2, type3, type4) case cmd: orig_import. field((type0)args[0], (type1)args[1], (type2)args[2], (type3)args[3], (type4)args[4]); break
#define ROUTE_IMPORT_6_V(field, cmd, type0, type1, type2, type3, type4, type5) case cmd: orig_import. field((type0)args[0], (type1)args[1], (type2)args[2], (type3)args[3], (type4)args[4], (type5)args[5]); break
#define ROUTE_IMPORT_7_V(field, cmd, type0, type1, type2, type3, type4, type5, type6) case cmd: orig_import. field((type0)args[0], (type1)args[1], (type2)args[2], (type3)args[3], (type4)args[4], (type5)args[5], (type6)args[6]); break
#define ROUTE_IMPORT_0(field, cmd) case cmd: ret = (intptr_t)orig_import. field(); break
#define ROUTE_IMPORT_1(field, cmd, type0) case cmd: ret = (intptr_t)orig_import. field((type0)args[0]); break
#define ROUTE_IMPORT_2(field, cmd, type0, type1) case cmd: ret = (intptr_t)orig_import. field((type0)args[0], (type1)args[1]); break
#define ROUTE_IMPORT_3(field, cmd, type0, type1, type2) case cmd: ret = (intptr_t)orig_import. field((type0)args[0], (type1)args[1], (type2)args[2]); break
#define ROUTE_IMPORT_4(field, cmd, type0, type1, type2, type3) case cmd: ret = (intptr_t)orig_import. field((type0)args[0], (type1)args[1], (type2)args[2], (type3)args[3]); break
#define ROUTE_IMPORT_5(field, cmd, type0, type1, type2, type3, type4) case cmd: ret = (intptr_t)orig_import. field((type0)args[0], (type1)args[1], (type2)args[2], (type3)args[3], (type4)args[4]); break
#define ROUTE_IMPORT_6(field, cmd, type0, type1, type2, type3, type4, type5) case cmd: ret = (intptr_t)orig_import. field((type0)args[0], (type1)args[1], (type2)args[2], (type3)args[3], (type4)args[4], (type5)args[5]); break
#define ROUTE_IMPORT_7(field, cmd, type0, type1, type2, type3, type4, type5, type6) case cmd: ret = (intptr_t)orig_import. field((type0)args[0], (type1)args[1], (type2)args[2], (type3)args[3], (type4)args[4], (type5)args[5], (type6)args[6]); break

// handle calls from QMM and plugins into the mod
#define ROUTE_EXPORT(field, cmd)		case cmd: ret = ((mod_vmMain)(orig_export-> field))(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]); break
#define ROUTE_EXPORT_VAR(field, cmd)	case cmd: ret = (intptr_t)&(orig_export-> field); break

#define ROUTE_EXPORT_0_V(field, cmd) case cmd: orig_export-> field(); break
#define ROUTE_EXPORT_1_V(field, cmd, type0) case cmd: orig_export-> field((type0)args[0]); break
#define ROUTE_EXPORT_2_V(field, cmd, type0, type1) case cmd: orig_export-> field((type0)args[0], (type1)args[1]); break
#define ROUTE_EXPORT_3_V(field, cmd, type0, type1, type2) case cmd: orig_export-> field((type0)args[0], (type1)args[1], (type2)args[2]); break
#define ROUTE_EXPORT_0(field, cmd) case cmd: ret = (intptr_t)orig_export-> field(); break
#define ROUTE_EXPORT_1(field, cmd, type0) case cmd: ret = (intptr_t)orig_export-> field((type0)args[0]); break
#define ROUTE_EXPORT_2(field, cmd, type0, type1) case cmd: ret = (intptr_t)orig_export-> field((type0)args[0], (type1)args[1]); break
#define ROUTE_EXPORT_3(field, cmd, type0, type1, type2) case cmd: ret = (intptr_t)orig_export-> field((type0)args[0], (type1)args[1], (type2)args[2]); break

// handle calls from engine or mod into QMM
#define GEN_IMPORT(field, cmd)	(decltype(qmm_import. field)) +[](intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16) { return ::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16); }
#define GEN_EXPORT(field, cmd)	(decltype(qmm_export. field)) +[](intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8) { cgame.is_from_QMM = true; return ::vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }

// if the syscall lambda types matter (float args in 64-bit games like Q2R), use these
// macros to easily generate a lambda with full return and argument type information:
// e.g. GEN_IMPORT_2(G_DEBUGGRAPH, void, float, int)
// e.g. GEN_IMPORT_2(G_ANIM_TIME, float, dtiki_t*, int)
#define  GEN_IMPORT_0(field, cmd, typeret) +[]() -> typeret { return (typeret)::qmm_syscall(cmd); }
#define  GEN_IMPORT_1(field, cmd, typeret, type0) +[](type0 arg0) -> typeret { return (typeret)::qmm_syscall(cmd, arg0); }
#define  GEN_IMPORT_2(field, cmd, typeret, type0, type1) +[](type0 arg0, type1 arg1) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1); }
#define  GEN_IMPORT_3(field, cmd, typeret, type0, type1, type2) +[](type0 arg0, type1 arg1, type2 arg2) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2); }
#define  GEN_IMPORT_4(field, cmd, typeret, type0, type1, type2, type3) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3); }
#define  GEN_IMPORT_5(field, cmd, typeret, type0, type1, type2, type3, type4) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4); }
#define  GEN_IMPORT_6(field, cmd, typeret, type0, type1, type2, type3, type4, type5) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5); }
#define  GEN_IMPORT_7(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6); }
#define  GEN_IMPORT_8(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7); }
#define  GEN_IMPORT_9(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }
#define GEN_IMPORT_10(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); }
#define GEN_IMPORT_11(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); }
#define GEN_IMPORT_12(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11); }
#define GEN_IMPORT_13(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12); }
#define GEN_IMPORT_14(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13); }
#define GEN_IMPORT_15(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14); }
#define GEN_IMPORT_16(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14, type15) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14, type15 arg15) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15); }
#define GEN_IMPORT_17(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14, type15, type16) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14, type15 arg15, type16 arg16) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16); }

#define  GEN_EXPORT_0(field, cmd, typeret) +[]() -> typeret { return (typeret)::vmMain(cmd); }
#define  GEN_EXPORT_1(field, cmd, typeret, type0) +[](type0 arg0) -> typeret { return (typeret)::vmMain(cmd, arg0); }
#define  GEN_EXPORT_2(field, cmd, typeret, type0, type1) +[](type0 arg0, type1 arg1) -> typeret { return (typeret)::vmMain(cmd, arg0, arg1); }
#define  GEN_EXPORT_3(field, cmd, typeret, type0, type1, type2) +[](type0 arg0, type1 arg1, type2 arg2) -> typeret { return (typeret)::vmMain(cmd, arg0, arg1, arg2); }

// ---------------------
// ----- QVM stuff -----
// ---------------------

// these macros handle qvm syscall arguments in GAME_QVMSyscall functions in game_*.cpp

// this gets an argument value (evaluate to an intptr_t)
#define VMARG(arg)	(intptr_t)args[arg]

// this adds the base VM address pointer to an argument value (evaluate to a pointer)
#define VMPTR(arg)	(args[arg] ? membase + args[arg] : nullptr)

// this subtracts the base VM address pointer from a value, for returning a pointer from syscall (this should evaluate to an int)
#define VMRET(ptr)	(int)(ptr ? (intptr_t)ptr - (intptr_t)membase : 0)

#endif // QMM2_GAME_API_H
