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

#include <stdarg.h>

// plugins and internal use
#ifdef _WIN32
 #define DLLEXPORT __declspec(dllexport)
#elif defined(__linux__)
 #define DLLEXPORT __attribute__((visibility("default")))
#else
 #define DLLEXPORT
#endif
#ifdef __cplusplus
 #define C_DLLEXPORT extern "C" DLLEXPORT
#else
 #define C_DLLEXPORT DLLEXPORT
#endif

typedef int (*eng_syscall_t)(int cmd, ...);
typedef int (*mod_vmMain_t)(int cmd, ...);

// major interface version increases with change to the signature of QMM_Query, QMM_Attach, QMM_Detach, pluginfunc_t, or plugininfo_t
#define QMM_PIFV_MAJOR  1
// minor interface version increases with trailing addition to pluginfunc_t or plugininfo_t structs
#define QMM_PIFV_MINOR  1

// holds plugin info to pass back to QMM
typedef struct {
    const char* name;		// name of plugin
    const char* version;	// version of plugin
    const char* desc;		// description of plugin
    const char* author;		// author of plugin
    const char* url;		// website of plugin
    int reserved1;			// unused (old - can this plugin be paused?)
    int reserved2;			// unused (old - can this plugin be loaded via cmd)
    int reserved3;			// unused (old - can this plugin be unloaded via cmd)
    int pifv_major;			// major plugin interface version
    int pifv_minor;			// minor plugin interface version
} plugininfo_t;


// prototype struct for QMM plugin util funcs
typedef struct {
    int (*pfnWriteGameLog)(const char* text, int len);
    char* (*pfnVarArgs)(const char* format, ...);
    int (*pfnIsQVM)();
    const char* (*pfnEngMsgName)(int msg);
    const char* (*pfnModMsgName)(int msg);
    int (*pfnGetIntCvar)(const char* cvar);
    const char* (*pfnGetStrCvar)(const char* cvar);
    const char* (*pfnGetGameEngine)();
} pluginfuncs_t;
// macros for QMM plugin util funcs
#define QMM_WRITEGAMELOG    (g_pluginfuncs->pfnWriteGameLog)
#define QMM_VARARGS         (g_pluginfuncs->pfnVarArgs)
#define QMM_ISQVM           (g_pluginfuncs->pfnIsQVM)
#define QMM_ENGMSGNAME      (g_pluginfuncs->pfnEngMsgName)
#define QMM_MODMSGNAME      (g_pluginfuncs->pfnModMsgName)
#define QMM_GETINTCVAR      (g_pluginfuncs->pfnGetIntCvar)
#define QMM_GETSTRCVAR      (g_pluginfuncs->pfnGetStrCvar)
#define QMM_GETGAMEENGINE   (g_pluginfuncs->pfnGetGameEngine)

// only set IGNORED, OVERRIDE, and SUPERCEDE
// UNUSED and ERROR are for internal use only
typedef enum pluginres_e {
    QMM_UNUSED = -2,
    QMM_ERROR = -1,
    QMM_IGNORED = 0,
    QMM_OVERRIDE,
    QMM_SUPERCEDE
} pluginres_t;

// QMM_Query
typedef void (*plugin_query)(plugininfo_t** pinfo);
// QMM_Attach
typedef int (*plugin_attach)(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, int vmbase, int reserved);
// QMM_Detach
typedef void (*plugin_detach)(int reserved);
// QMM_syscall
typedef int (*plugin_syscall)(int cmd, ...);
// QMM_vmMain
typedef int (*plugin_vmmain)(int cmd, ...);

// plugin use only
extern plugininfo_t g_plugininfo;       // set '*pinfo' to &g_plugininfo in QMM_Query
extern eng_syscall_t g_syscall;         // set to 'engfunc' in QMM_Attach
extern mod_vmMain_t g_vmMain;           // set to 'modfunc' in QMM_Attach
extern pluginres_t* g_result;           // set to 'result' in QMM_Attach
extern pluginfuncs_t* g_pluginfuncs;    // set to 'pluginfuncs' in QMM_Attach
extern int g_vmbase;                    // set to 'vmbase' in QMM_Attach

#define QMM_GIVE_PINFO() *pinfo = &g_plugininfo
#define QMM_SAVE_VARS() do { \
            g_syscall = engfunc; \
            g_vmMain = modfunc; \
            g_result = presult; \
            g_pluginfuncs = pluginfuncs; \
            g_vmbase = vmbase; \
        } while(0)

#define QMM_MAX_VMMAIN_ARGS     9

#define QMM_GET_VMMAIN_ARGS()   va_list arglist; \
                                int args[QMM_MAX_VMMAIN_ARGS] = {}; \
                                va_start(arglist, cmd); \
                                for (int i = 0; i < QMM_MAX_VMMAIN_ARGS; ++i) \
                                    args[i] = va_arg(arglist, int); \
                                va_end(arglist)

#define QMM_MAX_SYSCALL_ARGS    17

#define QMM_GET_SYSCALL_ARGS()  va_list arglist; \
                                int args[QMM_MAX_SYSCALL_ARGS] = {}; \
                                va_start(arglist, cmd); \
                                for (int i = 0; i < QMM_MAX_SYSCALL_ARGS; ++i) \
                                    args[i] = va_arg(arglist, int); \
                                va_end(arglist)

// prototypes for required entry points in the plugin
C_DLLEXPORT void QMM_Query(plugininfo_t** pinfo);
C_DLLEXPORT int QMM_Attach(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, int vmbase, int reserved);
C_DLLEXPORT void QMM_Detach(int reserved);
C_DLLEXPORT int QMM_vmMain(int cmd, ...);
C_DLLEXPORT int QMM_vmMain_Post(int cmd, ...);
C_DLLEXPORT int QMM_syscall(int cmd, ...);
C_DLLEXPORT int QMM_syscall_Post(int cmd, ...);

// macros to help set the plugin result value
#define QMM_RETURN(x, y)        return (*g_result = (pluginres_t)(x), (y))
#define QMM_SET_RESULT(x)       *g_result = (pluginres_t)(x)
#define QMM_RET_ERROR(x)        QMM_RETURN(QMM_ERROR, (x))
#define QMM_RET_IGNORED(x)      QMM_RETURN(QMM_IGNORED, (x))
#define QMM_RET_OVERRIDE(x)     QMM_RETURN(QMM_OVERRIDE, (x))
#define QMM_RET_SUPERCEDE(x)	QMM_RETURN(QMM_SUPERCEDE, (x))

// macros to convert between VM pointers and real pointers 
// modified to not add/subtract g_vmbase if x is NULL
#define GETPTR(x,y)     (x ? (y)((int)(x) + g_vmbase) : NULL)
#define SETPTR(x,y)     (x ? (y)((int)(x) - g_vmbase) : NULL)

// some helpful macros
#define ENT_FROM_NUM(index)     ((gentity_t*)((unsigned char*)g_gents + g_gentsize * (index)))
#define NUM_FROM_ENT(ent)       ((int)((unsigned char*)(ent) - (unsigned char*)g_gents) / g_gentsize)
#define CLIENT_FROM_NUM(index)  ((gclient_t*)((unsigned char*)g_clients + g_clientsize * (index)))
#define NUM_FROM_CLIENT(ent)    ((int)((unsigned char*)(ent) - (unsigned char*)g_clients) / g_clientsize)

#endif // __QMM2_QMMAPI_H__
