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

// A QMM plugin
struct Plugin {
    // QMM_Query signature
    using plugin_query = void (*)(plugin_info** pinfo);
    // QMM_Attach signature
    using plugin_attach = int (*)(eng_syscall engfunc, mod_vmMain modfunc, plugin_res* presult, plugin_funcs* pluginfuncs, plugin_vars* pluginvars);
    // QMM_Detach signature
    using plugin_detach = void (*)();
    // QMM_vmMain(_Post) + QMM_syscall(_Post) signatures
    using plugin_callback = intptr_t(*)(intptr_t cmd, intptr_t* args);
    // QMM_PluginMessage signature
    using plugin_pluginmessage = void (*)(plugin_id from_plid, const char* message, void* buf, intptr_t buflen, int is_broadcast);
    // QMM_QVMHandler signature
    using plugin_qvmhandler = int (*)(int cmd, int* args);

    void* dll = nullptr;                                // Plugin DLL handle
    std::string path;                                   // Plugin path
    plugin_query QMM_Query = nullptr;                   // QMM_Query function pointer
    plugin_attach QMM_Attach = nullptr;                 // QMM_Attach function pointer
    plugin_detach QMM_Detach = nullptr;                 // QMM_Detach function pointer
    plugin_callback QMM_vmMain = nullptr;               // QMM_vmMain function pointer
    plugin_callback QMM_vmMain_Post = nullptr;          // QMM_vmMain_Post function pointer
    plugin_callback QMM_syscall = nullptr;              // QMM_syscall function pointer
    plugin_callback QMM_syscall_Post = nullptr;         // QMM_syscall_Post function pointer
    plugin_pluginmessage QMM_PluginMessage = nullptr;   // QMM_PluginMessage function pointer (optional)
    plugin_qvmhandler QMM_QVMHandler = nullptr;         // QMM_QVMHandler function pointer (optional)
    plugin_info* plugininfo = nullptr;                  // Plugin-provided info

    // todo: move plugin_load and plugin_unload to member functions.
    // also to "own" the dll resource, we need to implement rule of 5
};

// This holds global variables that are available to plugins via helper functions.
struct plugin_globals {
    intptr_t final_return = 0;
    intptr_t orig_return = 0;
    plugin_res high_result = QMM_UNUSED;
    plugin_res plugin_result = QMM_UNUSED;
};

// This holds global variables that are available to plugins via helper functions.
extern plugin_globals g_plugin_globals;

// List of QMM plugins
extern std::vector<Plugin> g_plugins;

constexpr int QMM_QVM_FUNC_STARTING_ID = 10000;
// This holds pseudo-syscall IDs. They are registered to a given plugin, and when the QVM interpreter executes the
// syscall ID, the plugin's QMM_QVMHandler function is called.
extern std::map<int, Plugin*> g_registered_qvm_funcs;

/**
* @brief Convert a plugin result flag to string
*
* @param res Plugin result to convert
* @return Converted plugin result
*/
const char* plugin_result_to_str(plugin_res res);

/**
* @brief Load a QMM plugin
*
* @param p Reference to Plugin object to load
* @param file Path to Plugin pDLL
* @return >1 if load successful, 0 if file load failed, <0 if plugin loaded but QMM_Attach returned 0
*/
int plugin_load(Plugin& p, std::string file);

/**
* @brief Unload a QMM plugin
*
* @param p Reference to Plugin object to load
*/
void plugin_unload(Plugin& p);

#endif // QMM2_PLUGIN_H
