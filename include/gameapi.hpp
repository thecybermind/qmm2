/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAMEAPI_H
#define QMM2_GAMEAPI_H

#include "version.h"
#include <cstdint>  // intptr_t
#include <cstdarg>
#include <vector>
#include "qmmapi.h"


// ------------------------------------
// ----- Mod suffix and extension -----
// ------------------------------------

#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
 #define SUF_DLL "x86_64"               // Standard suffixes for mod DLLs ("x86", "i386", "x86_64", "x86_64")
 #define X64_SUF_DLL "x64"              // ioRTCW and ET:Legacy suffixes for mod DLLs ("x86", "i386", "x64", "x86_64")
#elif defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_32)
 #define SUF_DLL "x86"                  // Standard suffixes for mod DLLs ("x86", "i386", "x86_64", "x86_64")
 #define X64_SUF_DLL "x86"              // ioRTCW and ET:Legacy suffixes for mod DLLs ("x86", "i386", "x64", "x86_64")
#elif defined(QMM_OS_LINUX) && defined(QMM_ARCH_64)
 #define SUF_DLL "x86_64"               // Standard suffixes for mod DLLs ("x86", "i386", "x86_64", "x86_64")
 #define X64_SUF_DLL "x86_64"           // ioRTCW and ET:Legacy suffixes for mod DLLs ("x86", "i386", "x64", "x86_64")
#elif defined(QMM_OS_LINUX) && defined(QMM_ARCH_32)
 #define SUF_DLL "i386"                 // Standard suffixes for mod DLLs ("x86", "i386", "x86_64", "x86_64")
 #define X64_SUF_DLL "i386"             // ioRTCW and ET:Legacy suffixes for mod DLLs ("x86", "i386", "x64", "x86_64")
#else
 #error Unknown architecture + OS combination
#endif

#if defined(QMM_OS_WINDOWS)
 #define EXT_DLL ".dll"                 // Standard extensions for mod DLLs per OS (".dll", ".so")
 #define SP_DLL "_sp_"                  // RTCWSP filename changes between linux/Windows
 #define MP_DLL "_mp_"                  // COD, RTCWMP & WET filename changes between linux/Windows
 #define UO_DLL "uo_game" MP_DLL        // CODUO filename changes between linux/Windows
#elif defined(QMM_OS_LINUX)
 #define EXT_DLL ".so"                  // Standard extensions for mod DLLs per OS (".dll", ".so")
 #define SP_DLL ".sp."                  // RTCWSP filename changes between linux/Windows
 #define MP_DLL ".mp."                  // COD, RTCWMP & WET filename changes between linux/Windows
 #define UO_DLL "game" MP_DLL "uo."     // CODUO filename changes between linux/Windows
#endif 

#define EXT_QVM ".qvm"                  // Standard extension for mod QVM (".qvm")
#define MOD_DLL SUF_DLL EXT_DLL         // Standard extensions and suffixes for mod DLLs ("x86.dll", "i386.so", "x86_64.dll", "x86_64.so")
#define X64_DLL X64_SUF_DLL EXT_DLL     // ioRTCW and ET:Legacy suffixes for mod DLLs ("x86.dll", "i386.so", "x64.dll", "x86_64.so")

// ------------------------------
// ----- Game support stuff -----
// ------------------------------

// API type for engine/mod
enum APIType {
    QMM_API_ERROR,          // Error/unknown

    QMM_API_QVM,			// Mod-only

    QMM_API_DLLENTRY,       // dllEntry()
    QMM_API_GETGAMEAPI,     // GetGameAPI()
    QMM_API_GETMODULEAPI,   // GetModuleAPI()
};

/**
* @brief Get name of APIType enum
*
* @param api APIType value
* @return String name of the APIType enum
*/
const char* APIType_Name(APIType api);

/**
* @brief Get function of APIType enum
*
* @param api APIType value
* @return String name of the entry function associated with api
*/
const char* APIType_Function(APIType api);

// List of all the engine messages/constants used by QMM. If you change this, update the GEN_GAME_QMM_ENG_MSGS macro.
enum {
    // General purpose
    QMM_G_PRINT, QMM_G_ERROR, QMM_G_ARGV, QMM_G_ARGC, QMM_G_SEND_CONSOLE_COMMAND, QMM_G_GET_CONFIGSTRING,
    // CVars
    QMM_G_CVAR_REGISTER, QMM_G_CVAR_VARIABLE_STRING_BUFFER, QMM_G_CVAR_VARIABLE_INTEGER_VALUE, QMM_CVAR_SERVERINFO, QMM_CVAR_ROM,
    // Files
    QMM_G_FS_FOPEN_FILE, QMM_G_FS_READ, QMM_G_FS_WRITE, QMM_G_FS_FCLOSE_FILE, QMM_EXEC_APPEND, QMM_FS_READ,

    // Array size
    QMM_ENGINE_MSG_COUNT,
};

// List of all the mod messages/constants used by QMM. If you change this, update the GEN_GAME_QMM_MOD_MSGS macro.
enum {
    QMM_GAME_INIT, QMM_GAME_SHUTDOWN, QMM_GAME_CONSOLE_COMMAND,

    // Array size
    QMM_MOD_MSG_COUNT,
};

// Output game-specific message values to match the QMM engine messages.
#define GEN_GAME_QMM_ENG_MSGS() \
	{ \
		G_PRINT, G_ERROR, G_ARGV, G_ARGC, G_SEND_CONSOLE_COMMAND, G_GET_CONFIGSTRING, \
		G_CVAR_REGISTER, G_CVAR_VARIABLE_STRING_BUFFER, G_CVAR_VARIABLE_INTEGER_VALUE, CVAR_SERVERINFO, CVAR_ROM, \
		G_FS_FOPEN_FILE, G_FS_READ, G_FS_WRITE, G_FS_FCLOSE_FILE, EXEC_APPEND, FS_READ, \
	}

// Output game-specific message values to match the QMM mod messages.
#define GEN_GAME_QMM_MOD_MSGS() \
	{ \
		GAME_INIT, GAME_SHUTDOWN, GAME_CONSOLE_COMMAND, \
	}

// Pure virtual base class for game support
// need to implement all functions except:
// * DefaultQVMName - only need if the game supports QVMs. default will return nullptr (this is how QMM determines support)
// * ModCvar - only need if the engine's cvar for determining mod is different from "fs_game"
// * QVMSyscall - only need if the game supports QVMs. default returns 0
// need to implement qmm_eng_msgs and qmm_mod_msgs (use GEN_GAME_QMM_ENG_MSGS() and GEN_GAME_QMM_MOD_MSGS() macros)
struct GameSupport {
    // return string containing name of engine message (G_x)
    virtual const char* EngMsgName(intptr_t) = 0;
    // return string containing name of mod message (GAME_x)
    virtual const char* ModMsgName(intptr_t) = 0;
    // allows game support code to check if the qmm/engine info matches the game
    virtual bool AutoDetect(APIType) = 0;
    // allows game support code to handle the entrypoint once game was detected
    virtual void* Entry(void*, void*, APIType) = 0;
    // allows game support code to handle the mod's entrypoint once DLL is loaded
    virtual bool ModLoad(void*, APIType) = 0;
    // allows game support code to handle when the mod DLL is unloaded
    virtual void ModUnload() = 0;
    // returns game-specific engine message value (G_x) for a specific message that QMM uses
    virtual int QMMEngMsg(int msg) = 0;
    // returns game-specific mod message value (GAME_x) for a specific message that QMM uses
    virtual int QMMModMsg(int msg) = 0;

    // tells game support code to call into the engine
    virtual intptr_t syscall(intptr_t, ...) = 0;
    // tells game support code to call into the mod
    virtual intptr_t vmMain(intptr_t, ...) = 0;

    // returns the mod DLL name used by the game
    virtual const char* DefaultDLLName() = 0;
    // returns the QVM name used by the game
    virtual const char* DefaultQVMName() { return nullptr; }
    // returns the default mod directory for the game
    virtual const char* DefaultModDir() = 0;
    // returns the cvar for loaded mod dir
    virtual const char* ModCvar() { return "fs_game"; }
    // returns the full name of the game
    virtual const char* GameName() = 0;
    // returns the QMM short code for the game
    virtual const char* GameCode() = 0;

    // called by the QVM interpreter for syscalls. allows the game support code to convert pointer args
    virtual int QVMSyscall(uint8_t*, int, int*) { return 0; }
};

// Table of pointers to GameSupport objects
extern std::vector<GameSupport*> api_supportedgames;

// Generate extern for each game's support object (used at the top of gameapi.cpp)
#define GEN_GAME_EXTS(game)	extern GameSupport* game##_gamesupport

// Generate game support object (used at the top of game_XYZ.cpp)
#define GEN_GAME_OBJ(game) static game##_GameSupport gamesupport; GameSupport* game##_gamesupport = &gamesupport

// Reference game support object (used in gameapi.cpp in api_supportedgames list)
#define GET_GAME_OBJ(game) game##_gamesupport

// Generate a case/string line for use in the *MsgNames functions
#define GEN_CASE(x)		case x: return #x

// ----------------------------
// ----- API vararg stuff -----
// ----------------------------

// Max amount of vmMain args for games that support QVM
constexpr int QVM_MAX_VMMAIN_ARGS = 6;

// Max amount of vmMain args in any game
constexpr int QMM_MAX_VMMAIN_ARGS = 9;

// Pull vmMain args from varargs
#define QMM_GET_VMMAIN_ARGS()   intptr_t args[QMM_MAX_VMMAIN_ARGS] = {}; \
                                va_list arglist; \
                                va_start(arglist, cmd); \
                                for (int i = 0; i < QMM_MAX_VMMAIN_ARGS; ++i) \
                                    args[i] = va_arg(arglist, intptr_t); \
                                va_end(arglist)
// Generate list of all vmMain args for callsites
#define QMM_PUT_VMMAIN_ARGS()	args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]

// Max amount of syscall args in any game
constexpr int QMM_MAX_SYSCALL_ARGS = 18;
// Pull syscall args from varargs
#define QMM_GET_SYSCALL_ARGS()  intptr_t args[QMM_MAX_SYSCALL_ARGS] = {}; \
                                va_list arglist; \
                                va_start(arglist, cmd); \
                                for (int i = 0; i < QMM_MAX_SYSCALL_ARGS; ++i) \
                                    args[i] = va_arg(arglist, intptr_t); \
                                va_end(arglist)
// Generate list of all syscall args for callsites
#define QMM_PUT_SYSCALL_ARGS()	args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]

// ----------------------------
// ----- GetGameAPI stuff -----
// ----------------------------

// Handle calls from QMM and plugins into the engine
#define ROUTE_IMPORT(field, cmd)		case cmd: ret = ((eng_syscall)(orig_import. field))(QMM_PUT_SYSCALL_ARGS()); break
// Handle accessing an engine-exported variable
#define ROUTE_IMPORT_VAR(field, cmd)	case cmd: ret = (intptr_t)&(orig_import. field); break
// Handle 0 args, void return type
#define ROUTE_IMPORT_0_V(field, cmd) case cmd: orig_import. field(); break
// Handle 1 specific arg type (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_1_V(field, cmd, type0) case cmd: orig_import. field(type0 args[0]); break
// Handle 2 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_2_V(field, cmd, type0, type1) case cmd: orig_import. field(type0 args[0], type1 args[1]); break
// Handle 3 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_3_V(field, cmd, type0, type1, type2) case cmd: orig_import. field(type0 args[0], type1 args[1], type2 args[2]); break
// Handle 4 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_4_V(field, cmd, type0, type1, type2, type3) case cmd: orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3]); break
// Handle 5 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_5_V(field, cmd, type0, type1, type2, type3, type4) case cmd: orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4]); break
// Handle 6 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_6_V(field, cmd, type0, type1, type2, type3, type4, type5) case cmd: orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5]); break
// Handle 7 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_7_V(field, cmd, type0, type1, type2, type3, type4, type5, type6) case cmd: orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6]); break
// Handle 8 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_8_V(field, cmd, type0, type1, type2, type3, type4, type5, type6, type7) case cmd: orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6], type7 args[7]); break
// Handle 9 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_9_V(field, cmd, type0, type1, type2, type3, type4, type5, type6, type7, type8) case cmd: orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6], type7 args[7], type8 args[8]); break
// Handle 10 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_IMPORT_10_V(field, cmd, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9) case cmd: orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6], type7 args[7], type8 args[8], type9 args[9]); break
// Handle 0 args
#define ROUTE_IMPORT_0(field, cmd) case cmd: ret = (intptr_t)orig_import. field(); break
// Handle 1 specific arg type (for passing float args in the right registers)
#define ROUTE_IMPORT_1(field, cmd, type0) case cmd: ret = (intptr_t)orig_import. field(type0 args[0]); break
// Handle 2 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_2(field, cmd, type0, type1) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1]); break
// Handle 3 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_3(field, cmd, type0, type1, type2) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2]); break
// Handle 4 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_4(field, cmd, type0, type1, type2, type3) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3]); break
// Handle 5 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_5(field, cmd, type0, type1, type2, type3, type4) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4]); break
// Handle 6 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_6(field, cmd, type0, type1, type2, type3, type4, type5) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5]); break
// Handle 7 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_7(field, cmd, type0, type1, type2, type3, type4, type5, type6) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6]); break
// Handle 8 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_8(field, cmd, type0, type1, type2, type3, type4, type5, type6, type7) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6], type7 args[7]); break
// Handle 9 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_9(field, cmd, type0, type1, type2, type3, type4, type5, type6, type7, type8) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6], type7 args[7], type8 args[8]); break
// Handle 10 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_10(field, cmd, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6], type7 args[7], type8 args[8], type9 args[9]); break
// Handle 18 specific arg types (for passing float args in the right registers)
#define ROUTE_IMPORT_18(field, cmd, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14, type15, type16, type17) case cmd: ret = (intptr_t)orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6], type7 args[7], type8 args[8], type9 args[9], type10 args[10], type11 args[11], type12 args[12], type13 args[13], type14 args[14], type15 args[15], type16 args[16], type17 args[17]); break
// Handle 0 args, float return type
#define ROUTE_IMPORT_0_F(field, cmd) case cmd: fret = orig_import. field(); ret = *(int*)&fret; break
// Handle 1 specific arg type (for passing float args in the right registers), float return type
#define ROUTE_IMPORT_1_F(field, cmd, type0) case cmd: fret = orig_import. field(type0 args[0]); ret = *(int*)&fret; break
// Handle 2 specific arg types (for passing float args in the right registers), float return type
#define ROUTE_IMPORT_2_F(field, cmd, type0, type1) case cmd: fret = orig_import. field(type0 args[0], type1 args[1]); ret = *(int*)&fret; break
// Handle 3 specific arg types (for passing float args in the right registers), float return type
#define ROUTE_IMPORT_3_F(field, cmd, type0, type1, type2) case cmd: fret = orig_import. field(type0 args[0], type1 args[1], type2 args[2]); ret = *(int*)&fret; break
// Handle 4 specific arg types (for passing float args in the right registers), float return type
#define ROUTE_IMPORT_4_F(field, cmd, type0, type1, type2, type3) case cmd: fret = orig_import. field(type0 args[0], type1 args[1], type2 args[2], type3 args[3]); ret = *(int*)&fret; break

// Handle calls from QMM and plugins into the mod
#define ROUTE_EXPORT(field, cmd)		case cmd: ret = ((mod_vmMain)(orig_export-> field))(QMM_PUT_VMMAIN_ARGS()); break
// Handle accessing a mod-exported variable
#define ROUTE_EXPORT_VAR(field, cmd)	case cmd: ret = (intptr_t)&(orig_export-> field); break
// Handle 0 args, void return type
#define ROUTE_EXPORT_0_V(field, cmd) case cmd: orig_export-> field(); break
// Handle 1 specific arg type (for passing float args in the right registers), void return type
#define ROUTE_EXPORT_1_V(field, cmd, type0) case cmd: orig_export-> field(type0 args[0]); break
// Handle 2 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_EXPORT_2_V(field, cmd, type0, type1) case cmd: orig_export-> field(type0 args[0], type1 args[1]); break
// Handle 3 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_EXPORT_3_V(field, cmd, type0, type1, type2) case cmd: orig_export-> field(type0 args[0], type1 args[1], type2 args[2]); break
// Handle 7 specific arg types (for passing float args in the right registers), void return type
#define ROUTE_EXPORT_7_V(field, cmd, type0, type1, type2, type3, type4, type5, type6) case cmd: orig_export-> field(type0 args[0], type1 args[1], type2 args[2], type3 args[3], type4 args[4], type5 args[5], type6 args[6]); break
// Handle 0 args
#define ROUTE_EXPORT_0(field, cmd) case cmd: ret = (intptr_t)orig_export-> field(); break
// Handle 1 specific arg type (for passing float args in the right registers)
#define ROUTE_EXPORT_1(field, cmd, type0) case cmd: ret = (intptr_t)orig_export-> field(type0 args[0]); break
// Handle 2 specific arg types (for passing float args in the right registers)
#define ROUTE_EXPORT_2(field, cmd, type0, type1) case cmd: ret = (intptr_t)orig_export-> field(type0 args[0], type1 args[1]); break
// Handle 3 specific arg types (for passing float args in the right registers)
#define ROUTE_EXPORT_3(field, cmd, type0, type1, type2) case cmd: ret = (intptr_t)orig_export-> field( type0 args[0], type1 args[1], type2 args[2]); break

// Handle calls from mod into QMM
#define GEN_IMPORT(field, cmd)	(decltype(qmm_import. field)) +[](intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16, intptr_t arg17) { return ::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17); }
// Handle specific arg types (for passing float args in the right registers)
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
#define GEN_IMPORT_18(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14, type15, type16, type17) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14, type15 arg15, type16 arg16, type17 arg17) -> typeret { return (typeret)::qmm_syscall(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17); }
// Handle specific arg types (for passing float args in the right registers), float return type
#define  GEN_IMPORT_0_F(field, cmd) +[]() -> float { intptr_t ret = ::qmm_syscall(cmd); return *(float*)&ret; }
#define  GEN_IMPORT_1_F(field, cmd, type0) +[](type0 arg0) -> float { intptr_t ret = ::qmm_syscall(cmd, arg0); return *(float*)&ret; }
#define  GEN_IMPORT_2_F(field, cmd, type0, type1) +[](type0 arg0, type1 arg1) -> float { intptr_t ret = ::qmm_syscall(cmd, arg0, arg1); return *(float*)&ret; }
#define  GEN_IMPORT_3_F(field, cmd, type0, type1, type2) +[](type0 arg0, type1 arg1, type2 arg2) -> float { intptr_t ret = ::qmm_syscall(cmd, arg0, arg1, arg2); return *(float*)&ret; }
#define  GEN_IMPORT_4_F(field, cmd, type0, type1, type2, type3) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3) -> float { intptr_t ret = ::qmm_syscall(cmd, arg0, arg1, arg2, arg3); return *(float*)&ret; }

// Handle calls from engine into QMM
#define  GEN_EXPORT(field, cmd)	(decltype(qmm_export. field)) +[](intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8) { cgameinfo.is_from_QMM = true; return ::vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }
// Handle specific arg types (for passing float args in the right registers)
#define  GEN_EXPORT_0(field, cmd, typeret) +[]() -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd); }
#define  GEN_EXPORT_1(field, cmd, typeret, type0) +[](type0 arg0) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0); }
#define  GEN_EXPORT_2(field, cmd, typeret, type0, type1) +[](type0 arg0, type1 arg1) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0, arg1); }
#define  GEN_EXPORT_3(field, cmd, typeret, type0, type1, type2) +[](type0 arg0, type1 arg1, type2 arg2) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0, arg1, arg2); }
#define  GEN_EXPORT_4(field, cmd, typeret, type0, type1, type2, type3) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0, arg1, arg2, arg3); }
#define  GEN_EXPORT_5(field, cmd, typeret, type0, type1, type2, type3, type4) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0, arg1, arg2, arg3, arg4); }
#define  GEN_EXPORT_6(field, cmd, typeret, type0, type1, type2, type3, type4, type5) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5); }
#define  GEN_EXPORT_7(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6); }
#define  GEN_EXPORT_8(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7); }
#define  GEN_EXPORT_9(field, cmd, typeret, type0, type1, type2, type3, type4, type5, type6, type7, type8) +[](type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8) -> typeret { cgameinfo.is_from_QMM = true; return (typeret)::vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }

// ---------------------
// ----- QVM stuff -----
// ---------------------

// These macros handle qvm syscall arguments in GAME_QVMSyscall functions in game_*.cpp

// This gets the n'th argument value (evaluate to an intptr_t)
#define VMARG(n)	(intptr_t)args[n]

// This adds the base VM address pointer to the n'th argument value (evaluate to a pointer)
#define VMPTR(n)	(args[n] ? membase + args[n] : nullptr)

// This subtracts the base VM address pointer from given pointer value (for returning a pointer from syscall, evaluate to an int)
#define VMRET(ptr)	(int)(ptr ? (intptr_t)ptr - (intptr_t)membase : 0)

#endif // QMM2_GAMEAPI_H
