/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_PLUGIN_H__
#define __QMM2_PLUGIN_H__

#include <vector>
#include <string>
#include "qmmapi.h"

// QMM_Query
typedef void (*plugin_query)(plugininfo_t** pinfo);
// QMM_Attach
typedef int (*plugin_attach)(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, pluginvars_t* pluginvars);
// QMM_Detach
typedef void (*plugin_detach)();
// QMM_PluginMessage
typedef void (*plugin_pluginmessage)(plid_t from_plid, const char* message, void* buf, intptr_t buflen, int is_broadcast);
// QMM_syscall
typedef intptr_t(*plugin_syscall)(intptr_t cmd, intptr_t* args);
// QMM_vmMain
typedef intptr_t(*plugin_vmmain)(intptr_t cmd, intptr_t* args);

struct plugin_t {
    void* dll = nullptr;
    std::string path;
    plugin_query QMM_Query = nullptr;
    plugin_attach QMM_Attach = nullptr;
    plugin_detach QMM_Detach = nullptr;
    plugin_vmmain QMM_vmMain = nullptr;
    plugin_vmmain QMM_vmMain_Post = nullptr;
    plugin_syscall QMM_syscall = nullptr;
    plugin_syscall QMM_syscall_Post = nullptr;
    plugin_pluginmessage QMM_PluginMessage = nullptr;
    plugininfo_t* plugininfo = nullptr;
};

struct plugin_globals_t {
    intptr_t final_return = 0;
    intptr_t orig_return = 0;
    pluginres_t high_result = QMM_UNUSED;
    pluginres_t plugin_result = QMM_UNUSED;
};

extern plugin_globals_t g_plugin_globals;

extern std::vector<plugin_t> g_plugins;

const char* plugin_result_to_str(pluginres_t res);
// returns: -1 if failed to load and don't continue, 0 if failed to load and continue, 1 if loaded
int plugin_load(plugin_t& p, std::string file);
void plugin_unload(plugin_t& p);

#endif // __QMM2_PLUGIN_H__
