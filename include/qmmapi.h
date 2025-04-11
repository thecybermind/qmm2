/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_QMMAPI_H__
#define __QMM2_QMMAPI_H__

#include <stdint.h>

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

typedef intptr_t (*eng_syscall_t)(intptr_t cmd, ...);
typedef intptr_t (*mod_vmMain_t)(intptr_t cmd, ...);

// major interface version increases with change to the signature of QMM_Query, QMM_Attach, QMM_Detach, pluginfunc_t, or plugininfo_t
#define QMM_PIFV_MAJOR  3
// minor interface version increases with trailing addition to pluginfunc_t or plugininfo_t structs
#define QMM_PIFV_MINOR  0

// holds plugin info to pass back to QMM
typedef struct {
    intptr_t pifv_major;	// major plugin interface version
    intptr_t pifv_minor;	// minor plugin interface version
    const char* name;		// name of plugin
    const char* version;	// version of plugin
    const char* desc;		// description of plugin
    const char* author;		// author of plugin
    const char* url;		// website of plugin
    intptr_t reserved1;	    // reserved
    intptr_t reserved2;		// reserved
    intptr_t reserved3;		// reserved
} plugininfo_t;

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

// only set IGNORED, OVERRIDE, and SUPERCEDE
// UNUSED and ERROR are for internal use only
typedef enum pluginres_e {
    QMM_UNUSED = -2,
    QMM_ERROR = -1,
    QMM_IGNORED = 0,
    QMM_OVERRIDE,
    QMM_SUPERCEDE
} pluginres_t;

// prototype struct for QMM plugin util funcs
typedef struct {
    void (*pfnWriteQMMLog)(const char* text, int severity, const char* tag);
    char* (*pfnVarArgs)(const char* format, ...);
    int (*pfnIsQVM)();
    const char* (*pfnEngMsgName)(intptr_t msg);
    const char* (*pfnModMsgName)(intptr_t msg);
    intptr_t (*pfnGetIntCvar)(const char* cvar);
    const char* (*pfnGetStrCvar)(const char* cvar);
    const char* (*pfnGetGameEngine)();
    void (*pfnArgv)(intptr_t argn, char* buf, intptr_t buflen);
    const char* (*pfnInfoValueForKey)(const char* userinfo, const char* key);
} pluginfuncs_t;

// struct of vars for QMM plugin utils
typedef struct {
    intptr_t vmbase;				// base address of the QVM memory block (automatically added to pointer args in syscalls)
    intptr_t* preturn;				// pointer to an int that holds the return value from a function call (used in QMM_x_Post)
} pluginvars_t;

// macros for QMM plugin util funcs
#define QMM_WRITEQMMLOG     (g_pluginfuncs->pfnWriteQMMLog)     // write to the QMM log
#define QMM_VARARGS         (g_pluginfuncs->pfnVarArgs)         // simple vsprintf helper
#define QMM_ISQVM           (g_pluginfuncs->pfnIsQVM)           // returns 1 if the mod is QVM
#define QMM_ENGMSGNAME      (g_pluginfuncs->pfnEngMsgName)      // get the string name of a syscall code
#define QMM_MODMSGNAME      (g_pluginfuncs->pfnModMsgName)      // get the string name of a vmMain code
#define QMM_GETINTCVAR      (g_pluginfuncs->pfnGetIntCvar)      // get the int value of a cvar
#define QMM_GETSTRCVAR      (g_pluginfuncs->pfnGetStrCvar)      // get the str value of a cvar
#define QMM_GETGAMEENGINE   (g_pluginfuncs->pfnGetGameEngine)   // return the QMM short code for the game engine
#define QMM_ARGV            (g_pluginfuncs->pfnArgv)            // call G_ARGV, but can handle both engine styles
#define QMM_INFOVALUEFORKEY (g_pluginfuncs->pfnInfoValueForKey) // same as SDK's Info_ValueForKey

// macros for QMM plugin vars
#define QMM_GET_RETURN(x)   ((x)*(g_pluginvars->preturn))       // get the actual return value of a call while inside a QMM_x_Post call, with given cast

// QMM_Query
typedef void (*plugin_query)(plugininfo_t** pinfo);
// QMM_Attach
typedef int (*plugin_attach)(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, pluginvars_t* pluginvars);
// QMM_Detach
typedef void (*plugin_detach)();
// QMM_syscall
typedef intptr_t (*plugin_syscall)(intptr_t cmd, intptr_t* args);
// QMM_vmMain
typedef intptr_t (*plugin_vmmain)(intptr_t cmd, intptr_t* args);

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

// prototypes for required entry points in the plugin
C_DLLEXPORT void QMM_Query(plugininfo_t** pinfo);
C_DLLEXPORT int QMM_Attach(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, pluginvars_t* pluginvars);
C_DLLEXPORT void QMM_Detach();
C_DLLEXPORT intptr_t QMM_vmMain(intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_vmMain_Post(intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_syscall(intptr_t cmd, intptr_t* args);
C_DLLEXPORT intptr_t QMM_syscall_Post(intptr_t cmd, intptr_t* args);

// macros to help set the plugin result value
#define QMM_RETURN(x, y)		return (*g_result = (pluginres_t)(x), (y))
#define QMM_SET_RESULT(x)		*g_result = (pluginres_t)(x)
#define QMM_RET_ERROR(x)		QMM_RETURN(QMM_ERROR, (x))
#define QMM_RET_IGNORED(x)		QMM_RETURN(QMM_IGNORED, (x))
#define QMM_RET_OVERRIDE(x)		QMM_RETURN(QMM_OVERRIDE, (x))
#define QMM_RET_SUPERCEDE(x)	QMM_RETURN(QMM_SUPERCEDE, (x))

// These are macros to convert between VM pointers and real pointers.
// These are generally only needed for pointers inside objects, like gent->parent.
// As the objects are generally tracked through syscall arguments, which are already
// converted in plugin QMM_syscall functions. SETPTR should only be used to refer to
// objects that already exist within the QVM, but you have a real pointer to
#define GETPTR(x,y)     (x ? (y)((intptr_t)(x) + g_pluginvars->vmbase) : NULL)
#define SETPTR(x,y)     (x ? (y)((intptr_t)(x) - g_pluginvars->vmbase) : NULL)

// Some helpful macros assuming you've stored these in G_LOCATE_GAME_DATA
#define ENT_FROM_NUM(index)     ((gentity_t*)((unsigned char*)g_gents + g_gentsize * (index)))
#define NUM_FROM_ENT(ent)       ((int)((unsigned char*)(ent) - (unsigned char*)g_gents) / g_gentsize)
#define CLIENT_FROM_NUM(index)  ((gclient_t*)((unsigned char*)g_clients + g_clientsize * (index)))
#define NUM_FROM_CLIENT(ent)    ((int)((unsigned char*)(ent) - (unsigned char*)g_clients) / g_clientsize)

#endif // __QMM2_QMMAPI_H__
