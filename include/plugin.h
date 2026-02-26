/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_PLUGIN_H
#define QMM2_PLUGIN_H

#include <vector>
#include <map>
#include <string>
#include "qmmapi.h"

// QMM_Query
using plugin_query = void (*)(plugininfo_t** pinfo);
// QMM_Attach
using plugin_attach = int (*)(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, pluginvars_t* pluginvars);
// QMM_Detach
using plugin_detach = void (*)();
// QMM_vmMain
using plugin_vmmain = intptr_t(*)(intptr_t cmd, intptr_t* args);
// QMM_syscall
using plugin_syscall = intptr_t(*)(intptr_t cmd, intptr_t* args);
// QMM_PluginMessage
using plugin_pluginmessage = void (*)(plid_t from_plid, const char* message, void* buf, intptr_t buflen, int is_broadcast);
// QMM_QVMHandler
using plugin_qvmhandler = int (*)(int cmd, int* args);

struct plugin {
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
    plugin_qvmhandler QMM_QVMHandler = nullptr;
    plugininfo_t* plugininfo = nullptr;
};

struct plugin_globals {
    intptr_t final_return = 0;
    intptr_t orig_return = 0;
    pluginres_t high_result = QMM_UNUSED;
    pluginres_t plugin_result = QMM_UNUSED;
};

extern plugin_globals g_plugin_globals;

extern std::vector<plugin> g_plugins;

// this ID is like the various syscall values. they go through the following function y=-x-1 when used as QVM function pointers
// once the handler function is called, this is already undone with another y=-x-1 to get a positive number again
#define QMM_QVM_FUNC_STARTING_ID 10000
extern std::map<int, plugin*> g_registered_qvm_funcs;

const char* plugin_result_to_str(pluginres_t res);

// returns: -1 if failed to load and don't continue, 0 if failed to load and continue, 1 if loaded
int plugin_load(plugin& p, std::string file);

void plugin_unload(plugin& p);

#endif // QMM2_PLUGIN_H
