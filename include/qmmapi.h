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
#else
 #error Unknown OS?
#endif
#ifdef __cplusplus
 #define C_DLLEXPORT extern "C" DLLEXPORT
#else
 #define C_DLLEXPORT DLLEXPORT
#endif

/* Various function pointer types */

// Call engine syscall with varargs.
typedef intptr_t (*eng_syscall)(intptr_t, ...);
// Call engine syscall with arg array.
typedef intptr_t (*eng_syscall_args)(intptr_t, intptr_t*);
// Call mod vmMain with varargs.
typedef intptr_t (*mod_vmMain)(intptr_t, ...);
// Call mod vmMain with arg array.
typedef intptr_t (*mod_vmMain_args)(intptr_t, intptr_t*);
// Entry point for some gamedlls.
typedef void (*mod_dllEntry)(eng_syscall);
// Entry point for some gamedlls.
typedef void* (*mod_GetGameAPI)(void*, void*);

// Major interface version increases with breaking changes to signatures, types, etc
#define QMM_PIFV_MAJOR  6
// Minor interface version increases with trailing addition to plugin_func or plugin_info structs
#define QMM_PIFV_MINOR  0

/* PIFV changes
2:0
- removed canpause, loadcmd, unloadcmd from plugininfo_t
- renamed old pause/cmd args to QMM_ functions (iscmd, etc) to "reserved"
2:1
- added preturn to QMM_Attach for the current hook return value
3:0
- changed plugininfo_t to move pifv_major and pifv_minor to the beginning
- added pluginvars pointer to QMM_Attach
- removed reserved args to QMM_ function
3:1
- introduced pfnConfigGetStr, pfnConfigGetInt, pfnConfigGetBool, pfnConfigGetArrayStr, and pfnConfigGetArrayInt
4:0
- introduced the PLID arg to all plugin functions
- added logtag to plugininfo_t
- added porigreturn and phighresult to pluginvars_t
- added PluginMessage function to broadcast a message to all plugins' QMM_PluginMessage function
- renamed QMM_GET_RETURN to QMM_VAR_RETURN
4:1
- added QMM_PLUGIN_SEND
- added is_broadcast argument to QMM_PluginMessage
- QMM_PLUGIN_BROADCAST now returns int with how many plugins were called
4:2
- added QMM_QVM_REGISTER_FUNC, QMM_QVM_EXEC_FUNC, QMM_ARGV2, and QMM_GETCONFIGSTRING2
- added optional QMM_QVMHandler callback function
- pluginfunc macros now hide PLID arg
4:3
- changed type names away from those ending with "_t" - added typedefs for old names marked deprecated. define QMM_USE_DEPRECATED_TYPES to use old types without warning
- added QMM_MODDIR
4:4
- swapped order of severity and text in QMM_WRITEQMMLOG and also made it vararg. string construction is ignored if log won't write
- changed QMM_VARARGS return to const char*. this will require change to code, but existing compiled plugins will continue to work with no issue
5:0
- pass result vars to each callback function
- add syscall_args and vmMain_args function pointers to QMM_Attach
6:0
- pluginvars are now stored in the plugin, and a pointer is given to QMM
*/

// Plugin info to pass back to QMM
typedef struct {
    intptr_t pifv_major;    // Major plugin interface version
    intptr_t pifv_minor;    // Minor plugin interface version
    const char* name;       // Name of plugin (shows in "qmm list")
    const char* version;    // Version of plugin (shows in "qmm list")
    const char* desc;       // Description of plugin (shows in "qmm plugininfo #")
    const char* author;     // Author of plugin (shows in "qmm plugininfo #")
    const char* url;        // Website of plugin (shows in "qmm plugininfo #")
    const char* logtag;     // Log tag (shows in qmm2.log)
    intptr_t reserved1;     // Reserved
    intptr_t reserved2;     // Reserved
} plugin_info;

// "Opaque" plugin info pointer to use as an identifier for plugin funcs.
typedef plugin_info* plugin_id;
// Macro for passing plugin_id to plugin funcs.
#define PLID ((plugin_id)&g_plugininfo)

/* Severity level for QMM_WRITEQMMLOG */
enum {
    QMMLOG_TRACE,   // Lowest level, only for detailed debugging
    QMMLOG_DEBUG,   // Debugging information
    QMMLOG_INFO,    // Normal information, displayed to game console
    QMMLOG_NOTICE,  // Something of note
    QMMLOG_WARNING, // Warning, possible problems
    QMMLOG_ERROR,   // Error, maybe recoverable
    QMMLOG_FATAL,   // Fatal, unrecoverable
};

/* Prototype struct for QMM plugin utility functions */
typedef struct {
    void (*pfnWriteQMMLog)(plugin_id plid, int severity, const char* fmt, ...);                               // Write to the QMM log
    const char* (*pfnVarArgs)(plugin_id plid, const char* fmt, ...);                                          // Simple vsprintf helper with rotating buffer
    int (*pfnIsQVM)(plugin_id plid);                                                                          // Returns 1 if the mod is QVM
    const char* (*pfnEngMsgName)(plugin_id plid, intptr_t msg);                                               // Get the string name of a syscall code
    const char* (*pfnModMsgName)(plugin_id plid, intptr_t msg);                                               // Get the string name of a vmMain code
    intptr_t (*pfnGetIntCvar)(plugin_id plid, const char* cvar);                                              // Get the int value of a cvar
    const char* (*pfnGetStrCvar)(plugin_id plid, const char* cvar);                                           // Get the str value of a cvar
    const char* (*pfnGetGameEngine)(plugin_id plid);                                                          // Return the QMM short code for the game engine
    void (*pfnArgv)(plugin_id plid, intptr_t argn, char* buf, intptr_t buflen);                               // Call G_ARGV, but can handle both engine styles
    const char* (*pfnInfoValueForKey)(plugin_id plid, const char* userinfo, const char* key);                 // Same as SDK's Info_ValueForKey
    const char* (*pfnConfigGetStr)(plugin_id plid, const char* key);                                          // Get a string config entry
    int (*pfnConfigGetInt)(plugin_id plid, const char* key);                                                  // Get an int config entry
    int (*pfnConfigGetBool)(plugin_id plid, const char* key);                                                 // Get a bool config entry
    const char** (*pfnConfigGetArrayStr)(plugin_id plid, const char* key);                                    // Get an array-of-strings config entry (array terminated with a null pointer)
    int* (*pfnConfigGetArrayInt)(plugin_id plid, const char* key);                                            // Get an array-of-ints config entry (array starts with remaining length)
    void (*pfnGetConfigString)(plugin_id plid, intptr_t index, char* buf, intptr_t buflen);                   // Call G_GET_CONFIGSTRING, but can handle both engine styles
    int (*pfnPluginBroadcast)(plugin_id plid, const char* message, void* buf, intptr_t buflen);               // Broadcast a message to plugins' QMM_PluginMessage functions
    int (*pfnPluginSend)(plugin_id plid, plugin_id to_plid, const char* message, void* buf, intptr_t buflen); // Send a message to a specific plugin's QMM_PluginMessage function
    int (*pfnQVMRegisterFunc)(plugin_id plid);                                                                // Register a new QVM function ID to the calling plugin (0 if unsuccessful)
    int (*pfnQVMExecFunc)(plugin_id plid, int funcid, int argc, int* argv);                                   // Exec a given QVM function function ID
    const char* (*pfnArgv2)(plugin_id plid, intptr_t argn);                                                   // Same as pfnArgv except returns value
    const char* (*pfnGetConfigString2)(plugin_id plid, intptr_t index);                                       // Same as pfnGetConfigString except returns value
    const char* (*pfnModDir)(plugin_id plid);                                                                 // Return loaded mod directory
} plugin_funcs;

#define QMM_WRITEQMMLOG(sev, fmt, ...)          (g_pluginfuncs.pfnWriteQMMLog)(PLID, sev, fmt, ## __VA_ARGS__) // Write to the QMM log
#define QMM_VARARGS(fmt, ...)                   (g_pluginfuncs.pfnVarArgs)(PLID, fmt, __VA_ARGS__)             // Simple vsprintf helper
#define QMM_ISQVM()                             (g_pluginfuncs.pfnIsQVM)(PLID)                                 // Returns 1 if the mod is QVM
#define QMM_ENGMSGNAME(cmd)                     (g_pluginfuncs.pfnEngMsgName)(PLID, cmd)                       // Get the string name of a syscall code
#define QMM_MODMSGNAME(cmd)                     (g_pluginfuncs.pfnModMsgName)(PLID, cmd)                       // Get the string name of a vmMain code
#define QMM_GETINTCVAR(cvar)                    (g_pluginfuncs.pfnGetIntCvar)(PLID, cvar)                      // Get the int value of a cvar
#define QMM_GETSTRCVAR(cvar)                    (g_pluginfuncs.pfnGetStrCvar)(PLID, cvar)                      // Get the str value of a cvar
#define QMM_GETGAMEENGINE()                     (g_pluginfuncs.pfnGetGameEngine)(PLID)                         // Return the QMM short code for the game engine
#define QMM_ARGV(argn, buf, buflen)             (g_pluginfuncs.pfnArgv)(PLID, argn, buf, buflen)               // Call G_ARGV, but can handle both engine styles
#define QMM_INFOVALUEFORKEY(userinfo, key)      (g_pluginfuncs.pfnInfoValueForKey)(PLID, userinfo, key)        // Same as SDK's Info_ValueForKey
#define QMM_CFG_GETSTR(key)                     (g_pluginfuncs.pfnConfigGetStr)(PLID, key)                     // Get a string config entry
#define QMM_CFG_GETINT(key)                     (g_pluginfuncs.pfnConfigGetInt)(PLID, key)                     // Get an int config entry
#define QMM_CFG_GETBOOL(key)                    (g_pluginfuncs.pfnConfigGetBool)(PLID, key)                    // Get a bool config entry
#define QMM_CFG_GETARRAYSTR(key)                (g_pluginfuncs.pfnConfigGetArrayStr)(PLID, key)                // Get an array-of-strings config entry (array terminated with a null pointer)
#define QMM_CFG_GETARRAYINT(key)                (g_pluginfuncs.pfnConfigGetArrayInt)(PLID, key)                // Get an array-of-ints config entry (array starts with remaining length)
#define QMM_GETCONFIGSTRING(index, buf, buflen) (g_pluginfuncs.pfnGetConfigString)(PLID, index, buf, buflen)   // Call G_GET_CONFIGSTRING, but can handle both engine styles
#define QMM_PLUGIN_BROADCAST(msg, buf, buflen)  (g_pluginfuncs.pfnPluginBroadcast)(PLID, msg, buf, buflen)     // Broadcast a message to plugins' QMM_PluginMessage functions
#define QMM_PLUGIN_SEND(to, msg, buf, buflen)   (g_pluginfuncs.pfnPluginSend)(PLID, to, msg, buf, buflen)      // Send a message to a specific plugin's QMM_PluginMessage function
#define QMM_QVM_REGISTER_FUNC()                 (g_pluginfuncs.pfnQVMRegisterFunc)(PLID)                       // Register a new QVM function ID to the calling plugin (0 if unsuccessful)
#define QMM_QVM_EXEC_FUNC(funcid, argc, argv)   (g_pluginfuncs.pfnQVMExecFunc)(PLID, funcid, argc, argv)       // Exec a given QVM function function ID
#define QMM_ARGV2(argn)                         (g_pluginfuncs.pfnArgv2)(PLID, argn)                           // Same as QMM_ARGV except returns value
#define QMM_GETCONFIGSTRING2(index)             (g_pluginfuncs.pfnGetConfigString2)(PLID, index)               // Same as QMM_GETCONFIGSTRING except returns value
#define QMM_MODDIR()                            (g_pluginfuncs.pfnModDir)(PLID)                                // Return loaded mod directory

/* Struct of vars for QMM plugins */
typedef struct {
    // Base address of the QVM memory block (automatically added to pointer args in syscalls). See GETPTR and SETPTR macros.
    intptr_t vmbase;
    // This is the function pointer used to call into the engine with a given G_ value and optional arguments. 
    eng_syscall syscall;
    // This is the function pointer used to call into the engine with a given G_ value and optional arguments packed in an intptr_t array.
    eng_syscall_args syscall_args;
    // This is the function pointer used to call into the mod with a given GAME_ value and optional arguments.
    mod_vmMain vMmain;
    // This is the function pointer used to call into the mod with a given GAME_ value and optional arguments packed in an intptr_t array.
    mod_vmMain_args vmMain_args;
} plugin_vars;

/* Result flags a plugin can give to QMM */
typedef enum {
    QMM_UNUSED = -2,        // Do not explicitly set. This is the default value and using it will output a warning message in logs.
    QMM_ERROR = -1,         // Output an error message in logs, but otherwise functions like QMM_IGNORED.
    QMM_IGNORED = 0,        // This plugin callback doesn't need special handling.
    QMM_OVERRIDE,           // This plugin callback has overridden the return value.
    QMM_SUPERCEDE,          // This plugin callback has overridden the return value AND wants the real function to not be called.
} plugin_res;

/* Struct of vars for QMM plugin results */
typedef struct {
    // Result variable that this callback sets.
    plugin_res result;
    // Highest result so far (only tracks results from Pre hooks).
    plugin_res high_result;
    // Integer that holds the current value to be returned from a function call (updated by QMM_OVERRIDE/QMM_SUPERCEDE in all hooks).
    intptr_t final_return;
    // Integer that holds the real mod/engine return value (if called, only available in _Post hooks).
    intptr_t real_return;
} result_vars;

/* Macros to help set the plugin result value */

// Set result flag to "res"
#define QMM_SET_RESULT(res)         results->result = (plugin_res)(res)
// Set result flag to "res" and return "ret"
#define QMM_RETURN(res, ret)        return (QMM_SET_RESULT(res), (ret))
// Set result flag to QMM_ERROR (output an error message in logs, but otherwise functions like QMM_IGNORED), and return "ret"
#define QMM_RET_ERROR(ret)          QMM_RETURN(QMM_ERROR, (ret))
// Set result flag to QMM_IGNORED, and return "ret"
#define QMM_RET_IGNORED(ret)        QMM_RETURN(QMM_IGNORED, (ret))
// Set result flag to QMM_OVERRIDE (replace the return value of the real call), and return "ret"
#define QMM_RET_OVERRIDE(ret)       QMM_RETURN(QMM_OVERRIDE, (ret))
// Set result flag to QMM_SUPERCEDE (block the real call and replace the return value of the real call), and return "ret"
#define QMM_RET_SUPERCEDE(ret)      QMM_RETURN(QMM_SUPERCEDE, (ret))

/* Macros to help get QMM plugin results */

// Get the current highest QMM result value while inside a QMM_x (pre) call
#define QMM_VAR_HIGH_RES()          (results->high_result)
// Get the value to be passed to the caller (from override/supercede or original call), with given cast
#define QMM_VAR_FINAL_RETURN(cast)  ((cast)(results->final_return))
// Get the actual return value of a real call while inside a QMM_x_Post call, with given cast
#define QMM_VAR_REAL_RETURN(cast)   ((cast)(results->real_return))


// Convert "ptr" from a QVM pointer to a real pointer, and cast to "cast" type. Generally only needed for pointers inside objects,
// like gent->client, as the pointers passed directly as syscall arguments (e.g. ent) are already converted by QMM.
#define GETPTR(ptr, cast)     ((ptr) ? (cast)((intptr_t)(ptr) + g_pluginvars.vmbase) : NULL)
// Convert "ptr" from a real pointer to a QVM pointer, and cast to "cast" type. This should only be used to refer to an object
// that already exists within the QVM, but you have a real pointer to (e.g. from GETPTR()). Giving the address of a real object
// to the QVM will likely result in a QVM run-time error due to access protection.
#define SETPTR(ptr, cast)     ((ptr) ? (cast)((intptr_t)(ptr) - g_pluginvars.vmbase) : NULL)

// Call the engine with varargs
#define QMM_SYSCALL         (g_pluginvars.syscall)
// Call the engine with intptr_t array of args
#define QMM_SYSCALL_ARGS    (g_pluginvars.syscall_args)
// Call the mod with varargs
#define QMM_VMMAIN          (g_pluginvars.vmMain)
// Call the mod with intptr_t array of args
#define QMM_VMMAIN_ARGS     (g_pluginvars.vmMain_args)

/* Plugin use only */

// Set '*plugininfo' to &g_plugininfo in QMM_Query
extern plugin_info g_plugininfo;
// Set '*pluginfuncs` to &g_pluginfuncs in QMM_Query
extern plugin_funcs g_pluginfuncs;
// Set '*pluginvars` to &g_pluginvars in QMM_Query
extern plugin_vars g_pluginvars;

// Pass pointers to QMM in QMM_Query
#define QMM_GIVE_PINFO() do { \
            *plugininfo = &g_plugininfo; \
            *pluginfuncs = &g_pluginfuncs; \
            *pluginvars = &g_pluginvars; \
        } while (0)

/* Prototypes for entry points in the plugin */
C_DLLEXPORT void QMM_Query(plugin_info** plugininfo, plugin_funcs** pluginfuncs, plugin_vars** pluginvars);
C_DLLEXPORT int QMM_Attach();
C_DLLEXPORT void QMM_Detach();
C_DLLEXPORT intptr_t QMM_vmMain(result_vars* results, intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_vmMain_Post(result_vars* results, intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_syscall(result_vars* results, intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_syscall_Post(result_vars* results, intptr_t cmd, intptr_t* args);
C_DLLEXPORT void QMM_PluginMessage(plugin_id from_plid, const char* message, void* buf, intptr_t buflen, int is_broadcast);
C_DLLEXPORT int QMM_QVMHandler(int func, int* args);

/* Some helpful macros assuming you've stored entity/client info in a G_LOCATE_GAME_DATA hook */

// Get a gentity_t* by entity number (check g_gents for NULL first)
#define ENT_FROM_NUM(index)     ((gentity_t*)((unsigned char*)g_gents + g_gentsize * (index)))
// Get a entity number by gentity_t* (check g_gents for NULL and g_gentsize for 0 first)
#define NUM_FROM_ENT(ent)       ((int)((unsigned char*)(ent) - (unsigned char*)g_gents) / g_gentsize)
// Get a gclient_t* by client number (check g_clients for NULL first)
#define CLIENT_FROM_NUM(index)  ((gclient_t*)((unsigned char*)g_clients + g_clientsize * (index)))
// Get a client number by gclient_t* (check g_clients for NULL and g_clientsize for 0 first)
#define NUM_FROM_CLIENT(client) ((int)((unsigned char*)(client) - (unsigned char*)g_clients) / g_clientsize)

#endif // QMM2_QMMAPI_H
