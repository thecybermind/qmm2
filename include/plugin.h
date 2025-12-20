/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

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
typedef void (*plugin_pluginmessage)(plid_t from_plid, const char* message, void* buf, intptr_t buflen);
// QMM_syscall
typedef intptr_t(*plugin_syscall)(intptr_t cmd, intptr_t* args);
// QMM_vmMain
typedef intptr_t(*plugin_vmmain)(intptr_t cmd, intptr_t* args);

typedef struct plugin_s {
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
} plugin_t;

typedef struct plugin_globals_s {
	pluginres_t plugin_result;
	intptr_t final_return;
	intptr_t orig_return;
	pluginres_t high_result;
} plugin_globals_t;

extern plugin_globals_t g_plugin_globals;

extern std::vector<plugin_t> g_plugins;

const char* plugin_result_to_str(pluginres_t res);
bool plugin_load(plugin_t& p, std::string file);
void plugin_unload(plugin_t& p);

#endif // __QMM2_PLUGIN_H__
