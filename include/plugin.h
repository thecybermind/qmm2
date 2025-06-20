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
	plugininfo_t* plugininfo = nullptr;
} plugin_t;

extern std::vector<plugin_t> g_plugins;
extern pluginres_t g_plugin_result;
extern intptr_t g_api_return;

const char* plugin_result_to_str(pluginres_t res);
bool plugin_load(plugin_t& p, std::string file);
void plugin_unload(plugin_t& p);


#endif // __QMM2_PLUGIN_H__
