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

// plugins and internal use
#ifdef WIN32
 #define DLLEXPORT __declspec(dllexport)
#else
 #define DLLEXPORT 
#endif
#ifdef __cplusplus
 #define C_DLLEXPORT extern "C" DLLEXPORT
#else
 #define C_DLLEXPORT DLLEXPORT
#endif

typedef unsigned char byte;

typedef int (*eng_syscall_t)(int, ...);
typedef int (*mod_vmMain_t)(int, int, int, int, int, int, int, int, int, int, int, int, int);
typedef void (*mod_dllEntry_t)(eng_syscall_t);

// major interface version increases with change to QMM_Query, QMM_Attach or a pluginfunc_t signature
#define QMM_PIFV_MAJOR  2
// minor interface version increases with trailing addition to pluginfunc_t struct
#define QMM_PIFV_MINOR  0

// holds plugin info to pass back to QMM
typedef struct {
    char* name;     // name of plugin
    char* version;  // version of plugin
    char* desc;     // description of plugin
    char* author;   // author of plugin
    char* url;      // website of plugin

    int pifv_major; // major plugin interface version
    int pifv_minor; // minor plugin interface version
} plugininfo_t;


// prototype struct for QMM plugin util funcs
typedef struct pluginfuncs_s {
    int (*pfnWriteGameLog)(const char*, int);
    char* (*pfnVarArgs)(char*, ...);
    int (*pfnIsQVM)();
    const char* (*pfnEngMsgName)(int);
    const char* (*pfnModMsgName)(int);
    int (*pfnGetIntCvar)(const char*);
    const char* (*pfnGetStrCvar)(const char*);
} pluginfuncs_t;
// macros for QMM plugin util funcs
#define QMM_WRITEGAMELOG    (g_pluginfuncs->pfnWriteGameLog)
#define QMM_VARARGS         (g_pluginfuncs->pfnVarArgs)
#define QMM_ISQVM           (g_pluginfuncs->pfnIsQVM)
#define QMM_ENGMSGNAME      (g_pluginfuncs->pfnEngMsgName)
#define QMM_MODMSGNAME      (g_pluginfuncs->pfnModMsgName)
#define QMM_GETINTCVAR      (g_pluginfuncs->pfnGetIntCvar)
#define QMM_GETSTRCVAR      (g_pluginfuncs->pfnGetStrCvar)

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
typedef void (*plugin_query)(plugininfo_t**, const char*);
// QMM_Attach
typedef int (*plugin_attach)(eng_syscall_t, mod_vmMain_t, pluginres_t*, pluginfuncs_t*, int);
// QMM_Detach
typedef void (*plugin_detach)();
// QMM_syscall
typedef int (*plugin_syscall)(int, int, int, int, int, int, int, int, int, int, int, int, int, int);
// QMM_vmMain
typedef int (*plugin_vmmain)(int, int, int, int, int, int, int, int, int, int, int, int, int);

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

// prototypes for required entry points in the plugin
C_DLLEXPORT void QMM_Query(plugininfo_t** pinfo, const char* game_engine);
C_DLLEXPORT int QMM_Attach(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, int vmbase);
C_DLLEXPORT void QMM_Detach();
C_DLLEXPORT int QMM_vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11);
C_DLLEXPORT int QMM_syscall(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11, int arg12);
C_DLLEXPORT int QMM_vmMain_Post(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11);
C_DLLEXPORT int QMM_syscall_Post(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11, int arg12);

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
#define ENT_FROM_NUM(index)     ((gentity_t*)((byte*)g_gents + g_gentsize * (index)))
#define NUM_FROM_ENT(ent)       ((int)((byte*)(ent) - (byte*)g_gents) / g_gentsize)
#define CLIENT_FROM_NUM(index)  ((gclient_t*)((byte*)g_clients + g_clientsize * (index)))
#define NUM_FROM_CLIENT(ent)    ((int)((byte*)(ent) - (byte*)g_clients) / g_clientsize)

#endif // __QMM2_QMMAPI_H__
