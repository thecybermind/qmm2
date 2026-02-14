/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_QMMAPI_H
#define QMM2_QMMAPI_H

// a lot of game sdks use a "byte" typedef so try to avoid pulling in std::byte
#if defined(_HAS_STD_BYTE)
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0
#include <stdint.h>     // intptr_t

// plugins and internal use
#if defined(_WIN32)
 #define DLLEXPORT __declspec(dllexport)
#elif defined(__linux__)
 #define DLLEXPORT __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
 #define C_DLLEXPORT extern "C" DLLEXPORT
#else
 #define C_DLLEXPORT DLLEXPORT
#endif

// various function pointer types
typedef intptr_t (*eng_syscall_t)(intptr_t, ...);
typedef intptr_t (*mod_vmMain_t)(intptr_t, ...);
typedef void (*mod_dllEntry_t)(eng_syscall_t);
typedef void* (*mod_GetGameAPI_t)(void*, void*);

// major interface version increases with change to the signature of QMM_Query, QMM_Attach, QMM_Detach, pluginfunc_t, or plugininfo_t
#define QMM_PIFV_MAJOR  4
// minor interface version increases with trailing addition to pluginfunc_t or plugininfo_t structs
#define QMM_PIFV_MINOR  1
// 2:0
// - removed canpause, loadcmd, unloadcmd from plugininfo_t
// - renamed old pause/cmd args to QMM_ functions (iscmd, etc) to "reserved"
// 2:1
// - added preturn to QMM_Attach for the current hook return value
// 3:0
// - changed plugininfo_t to move pifv_major and pifv_minor to the beginning
// - added pluginvars pointer to QMM_Attach
// - removed reserved args to QMM_ function
// 3:1
// - introduced pfnConfigGetStr, pfnConfigGetInt, pfnConfigGetBool, pfnConfigGetArrayStr, and pfnConfigGetArrayInt
// 4:0
// - introduced the PLID arg to all plugin functions
// - added logtag to plugininfo_t
// - added porigreturn and phighresult to pluginvars_t
// - added PluginMessage function to broadcast a message to all plugins' QMM_PluginMessage function
// - renamed QMM_GET_RETURN to QMM_VAR_RETURN
// 4:1
// - added QMM_PLUGIN_SEND
// - added is_broadcast argument to QMM_PluginMessage
// - QMM_PLUGIN_BROADCAST now returns int with how many plugins were called


// holds plugin info to pass back to QMM
typedef struct plugininfo_s {
    intptr_t pifv_major;	// major plugin interface version
    intptr_t pifv_minor;	// minor plugin interface version
    const char* name;		// name of plugin
    const char* version;	// version of plugin
    const char* desc;		// description of plugin
    const char* author;		// author of plugin
    const char* url;		// website of plugin
    const char* logtag;     // log tag
    intptr_t reserved1;		// reserved
    intptr_t reserved2;		// reserved
} plugininfo_t;

// "opaque" plugin info pointer to use as an identifier for plugin funcs
typedef plugininfo_t* plid_t;
#define PLID ((plid_t)&g_plugininfo)

// log severity for QMM_WRITEQMMLOG
enum {
    QMMLOG_TRACE,
    QMMLOG_DEBUG,
    QMMLOG_INFO,
    QMMLOG_NOTICE,
    QMMLOG_WARNING,
    QMMLOG_ERROR,
    QMMLOG_FATAL
};

// only set QMM_ERROR, QMM_IGNORED, QMM_OVERRIDE, and QMM_SUPERCEDE
typedef enum pluginres_e {
    QMM_UNUSED = -2,        // default value, output a warning message in logs, do not use, but otherwise functions like QMM_IGNORED
    QMM_ERROR = -1,         // output an error message in logs, but otherwise functions like QMM_IGNORED
    QMM_IGNORED = 0,        // this plugin doesn't need special handling
    QMM_OVERRIDE,           // this plugin has overridden the return value
    QMM_SUPERCEDE           // this plugin has overridden the return value AND wants the original function to not be called
} pluginres_t;

// macros to help set the plugin result value
#define QMM_SET_RESULT(res)		*g_result = (pluginres_t)(res)          // set result flag to "re"s
#define QMM_RETURN(res, ret)	return (QMM_SET_RESULT(res), (ret))     // set result flag to "res" and return "ret"
#define QMM_RET_ERROR(ret)		QMM_RETURN(QMM_ERROR, (ret))            // output an error message in logs, but otherwise functions like QMM_IGNORED, and return "ret"
#define QMM_RET_IGNORED(ret)	QMM_RETURN(QMM_IGNORED, (ret))          // this plugin doesn't need special handling, and return "ret"
#define QMM_RET_OVERRIDE(ret)	QMM_RETURN(QMM_OVERRIDE, (ret))         // this plugin has overridden the return value, and return "ret"
#define QMM_RET_SUPERCEDE(ret)	QMM_RETURN(QMM_SUPERCEDE, (ret))        // this plugin has overridden the return value AND wants the original function to not be called, and return "ret"

// prototype struct for QMM plugin util funcs
typedef struct pluginfuncs_s {
    void (*pfnWriteQMMLog)(plid_t plid, const char* text, int severity);                                // write to the QMM log
    char* (*pfnVarArgs)(plid_t plid, const char* format, ...);                                          // simple vsprintf helper with rotating buffer
    int (*pfnIsQVM)(plid_t plid);                                                                       // returns 1 if the mod is QVM
    const char* (*pfnEngMsgName)(plid_t plid, intptr_t msg);                                            // get the string name of a syscall code
    const char* (*pfnModMsgName)(plid_t plid, intptr_t msg);                                            // get the string name of a vmMain code
    intptr_t (*pfnGetIntCvar)(plid_t plid, const char* cvar);                                           // get the int value of a cvar
    const char* (*pfnGetStrCvar)(plid_t plid, const char* cvar);                                        // get the str value of a cvar
    const char* (*pfnGetGameEngine)(plid_t plid);                                                       // return the QMM short code for the game engine
    void (*pfnArgv)(plid_t plid, intptr_t argn, char* buf, intptr_t buflen);                            // call G_ARGV, but can handle both engine styles
    const char* (*pfnInfoValueForKey)(plid_t plid, const char* userinfo, const char* key);              // same as SDK's Info_ValueForKey
    const char* (*pfnConfigGetStr)(plid_t plid, const char* key);                                       // get a string config entry
    int (*pfnConfigGetInt)(plid_t plid, const char* key);                                               // get an int config entry
    int (*pfnConfigGetBool)(plid_t plid, const char* key);                                              // get a bool config entry
    const char** (*pfnConfigGetArrayStr)(plid_t plid, const char* key);                                 // get an array-of-strings config entry (array terminated with a null pointer)
    int* (*pfnConfigGetArrayInt)(plid_t plid, const char* key);                                         // get an array-of-ints config entry (array starts with remaining length)
    void (*pfnGetConfigString)(plid_t plid, intptr_t argn, char* buf, intptr_t buflen);                 // call G_GET_CONFIGSTRING, but can handle both engine styles
    int (*pfnPluginBroadcast)(plid_t plid, const char* message, void* buf, intptr_t buflen);            // broadcast a message to plugins' QMM_PluginMessage functions
    int (*pfnPluginSend)(plid_t plid, plid_t to_plid, const char* message, void* buf, intptr_t buflen); // send a message to a specific plugin's QMM_PluginMessage function
    int (*pfnQVMRegisterFunc)(plid_t plid);                                                             // register a new QVM function ID to the calling plugin (0 if unsuccessful)
    int (*pfnQVMExecFunc)(plid_t plid, int funcid, int argc, int* argv);                                // exec a given QVM function function ID
} pluginfuncs_t;

// macros for QMM plugin util funcs
#define QMM_WRITEQMMLOG             (g_pluginfuncs->pfnWriteQMMLog)         // write to the QMM log
#define QMM_VARARGS                 (g_pluginfuncs->pfnVarArgs)             // simple vsprintf helper
#define QMM_ISQVM                   (g_pluginfuncs->pfnIsQVM)               // returns 1 if the mod is QVM
#define QMM_ENGMSGNAME              (g_pluginfuncs->pfnEngMsgName)          // get the string name of a syscall code
#define QMM_MODMSGNAME              (g_pluginfuncs->pfnModMsgName)          // get the string name of a vmMain code
#define QMM_GETINTCVAR              (g_pluginfuncs->pfnGetIntCvar)          // get the int value of a cvar
#define QMM_GETSTRCVAR              (g_pluginfuncs->pfnGetStrCvar)          // get the str value of a cvar
#define QMM_GETGAMEENGINE           (g_pluginfuncs->pfnGetGameEngine)       // return the QMM short code for the game engine
#define QMM_ARGV                    (g_pluginfuncs->pfnArgv)                // call G_ARGV, but can handle both engine styles
#define QMM_INFOVALUEFORKEY         (g_pluginfuncs->pfnInfoValueForKey)     // same as SDK's Info_ValueForKey
#define QMM_CFG_GETSTR              (g_pluginfuncs->pfnConfigGetStr)        // get a string config entry
#define QMM_CFG_GETINT              (g_pluginfuncs->pfnConfigGetInt)        // get an int config entry
#define QMM_CFG_GETBOOL             (g_pluginfuncs->pfnConfigGetBool)       // get a bool config entry
#define QMM_CFG_GETARRAYSTR         (g_pluginfuncs->pfnConfigGetArrayStr)   // get an array-of-strings config entry (array terminated with a null pointer)
#define QMM_CFG_GETARRAYINT         (g_pluginfuncs->pfnConfigGetArrayInt)   // get an array-of-ints config entry (array starts with remaining length)
#define QMM_GETCONFIGSTRING         (g_pluginfuncs->pfnGetConfigString)     // call G_GET_CONFIGSTRING, but can handle both engine styles
#define QMM_PLUGIN_BROADCAST        (g_pluginfuncs->pfnPluginBroadcast)     // broadcast a message to plugins' QMM_PluginMessage functions
#define QMM_PLUGIN_SEND             (g_pluginfuncs->pfnPluginSend)          // send a message to a specific plugin's QMM_PluginMessage function
#define QMM_QVM_REGISTER_FUNC       (g_pluginfuncs->pfnQVMRegisterFunc)     // register a new QVM function ID to the calling plugin (0 if unsuccessful)
#define QMM_QVM_EXEC_FUNC           (g_pluginfuncs->pfnQVMExecFunc)         // exec a given QVM function function ID

// struct of vars for QMM plugin utils
typedef struct pluginvars_s {
    intptr_t vmbase;                // base address of the QVM memory block (automatically added to pointer args in syscalls)
    intptr_t* preturn;              // pointer to an int that holds the current value to be returned from a function call (updated by QMM_OVERRIDE/QMM_SUPERCEDE in all hooks)
    intptr_t* porigreturn;          // pointer to an int that holds the real mod/engine return value (if called, only available in _Post hooks)
    pluginres_t* phighresult;       // highest result so far (only tracks results from Pre hooks)
} pluginvars_t;

// macros for QMM plugin vars
#define QMM_VAR_RETURN(cast)        ((cast)*(g_pluginvars->preturn))        // get the value to be passed to the caller (from override/supercede or original call), with given cast
#define QMM_VAR_ORIG_RETURN(cast)   ((cast)*(g_pluginvars->porigreturn))    // get the actual return value of a real call while inside a QMM_x_Post call, with given cast
#define QMM_VAR_HIGH_RES()          (*(g_pluginvars->phighresult))          // get the current highest QMM result value while inside a QMM_x (pre) call

// GETPTR and SETPTR are macros to convert between VM pointers and real pointers.
// These are generally only needed for pointers inside objects, like gent->client,
// as the objects are generally tracked through syscall arguments, which are already
// converted in plugin QMM_syscall functions. SETPTR should only be used to refer to
// objects that already exist within the QVM, but you have a real pointer to
#define GETPTR(ptr, cast)     ((ptr) ? (cast)((intptr_t)(ptr) + g_pluginvars->vmbase) : NULL)   // convert "ptr" from a QVM pointer to a real pointer, and cast to "cast" type
#define SETPTR(ptr, cast)     ((ptr) ? (cast)((intptr_t)(ptr) - g_pluginvars->vmbase) : NULL)   // convert "ptr" from a real pointer to a QVM pointer, and cast to "cast" type

// plugin use only
extern plugininfo_t g_plugininfo;       // set '*pinfo' to &g_plugininfo in QMM_Query
extern eng_syscall_t g_syscall;         // set to 'engfunc' in QMM_Attach
extern mod_vmMain_t g_vmMain;           // set to 'modfunc' in QMM_Attach
extern pluginres_t* g_result;           // set to 'presult' in QMM_Attach
extern pluginfuncs_t* g_pluginfuncs;    // set to 'pluginfuncs' in QMM_Attach
extern pluginvars_t* g_pluginvars;      // set to 'pluginvars' in QMM_Attach

#define QMM_GIVE_PINFO() *pinfo = &g_plugininfo
#define QMM_SAVE_VARS() do { \
            g_syscall = engfunc; \
            g_vmMain = modfunc; \
            g_result = presult; \
            g_pluginfuncs = pluginfuncs; \
            g_pluginvars = pluginvars; \
        } while(0)

// prototypes for entry points in the plugin
C_DLLEXPORT void QMM_Query(plugininfo_t** pinfo);
C_DLLEXPORT int QMM_Attach(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, pluginvars_t* pluginvars);
C_DLLEXPORT void QMM_Detach();
C_DLLEXPORT void QMM_PluginMessage(plid_t from_plid, const char* message, void* buf, intptr_t buflen, int is_broadcast);
C_DLLEXPORT intptr_t QMM_vmMain(intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_vmMain_Post(intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_syscall(intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_syscall_Post(intptr_t cmd, intptr_t* args);

// Some helpful macros assuming you've stored entity/client info in G_LOCATE_GAME_DATA
#define ENT_FROM_NUM(index)     ((gentity_t*)((unsigned char*)g_gents + g_gentsize * (index)))                  // get a gentity_t* by entity number
#define NUM_FROM_ENT(ent)       ((int)((unsigned char*)(ent) - (unsigned char*)g_gents) / g_gentsize)           // get a entity number by gentity_t*
#define CLIENT_FROM_NUM(index)  ((gclient_t*)((unsigned char*)g_clients + g_clientsize * (index)))              // get a gclient_t* by client number
#define NUM_FROM_CLIENT(client) ((int)((unsigned char*)(client) - (unsigned char*)g_clients) / g_clientsize)    // get a client number by gclient_t*

#endif // QMM2_QMMAPI_H
