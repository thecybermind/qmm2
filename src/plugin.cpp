/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS
#include "log.h"
#include "format.h"
#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "mod.h"
#include "plugin.h"
#include "util.h"

#define NUM_PLUGIN_STR_BUFFERS 12

static plugin_t* s_plugin_find_by_id(plid_t plid);

static void s_plugin_helper_WriteQMMLog(plid_t plid, const char* text, int severity);
static char* s_plugin_helper_VarArgs(plid_t plid, const char* format, ...);
static int s_plugin_helper_IsQVM(plid_t plid);
static const char* s_plugin_helper_EngMsgName(plid_t plid, intptr_t msg);
static const char* s_plugin_helper_ModMsgName(plid_t plid, intptr_t msg);
static intptr_t s_plugin_helper_GetIntCvar(plid_t plid, const char* cvar);
static const char* s_plugin_helper_GetStrCvar(plid_t plid, const char* cvar);
static const char* s_plugin_helper_GetGameEngine(plid_t plid);
static void s_plugin_helper_Argv(plid_t plid, intptr_t argn, char* buf, intptr_t buflen);
static const char* s_plugin_helper_InfoValueForKey(plid_t plid, const char* userinfo, const char* key);
static const char* s_plugin_helper_ConfigGetStr(plid_t plid, const char* key);
static int s_plugin_helper_ConfigGetInt(plid_t plid, const char* key);
static int s_plugin_helper_ConfigGetBool(plid_t plid, const char* key);
static const char** s_plugin_helper_ConfigGetArrayStr(plid_t plid, const char* key);
static int* s_plugin_helper_ConfigGetArrayInt(plid_t plid, const char* key);
static void s_plugin_helper_GetConfigString(plid_t plid, intptr_t argn, char* buf, intptr_t buflen);

static pluginfuncs_t s_pluginfuncs = {
	s_plugin_helper_WriteQMMLog,
	s_plugin_helper_VarArgs,
	s_plugin_helper_IsQVM,
	s_plugin_helper_EngMsgName,
	s_plugin_helper_ModMsgName,
	s_plugin_helper_GetIntCvar,
	s_plugin_helper_GetStrCvar,
	s_plugin_helper_GetGameEngine,
	s_plugin_helper_Argv,
	s_plugin_helper_InfoValueForKey,
	s_plugin_helper_ConfigGetStr,
	s_plugin_helper_ConfigGetInt,
	s_plugin_helper_ConfigGetBool,
	s_plugin_helper_ConfigGetArrayStr,
	s_plugin_helper_ConfigGetArrayInt,
	s_plugin_helper_GetConfigString,
};

// struct to store all the globals available to plugins
plugin_globals_t g_plugin_globals = {
	QMM_UNUSED,	// plugin_result
	0,			// api_return
	0,			// orig_return
	QMM_UNUSED,	// high_result
};

std::vector<plugin_t> g_plugins;

static pluginvars_t s_pluginvars = {
	0,				// vmbase, set in plugin_load
	&g_plugin_globals.api_return,
	&g_plugin_globals.orig_return,
	&g_plugin_globals.high_result,
};


const char* plugin_result_to_str(pluginres_t res) {
	switch (res) {
		GEN_CASE(QMM_UNUSED);
		GEN_CASE(QMM_ERROR);
		GEN_CASE(QMM_IGNORED);
		GEN_CASE(QMM_OVERRIDE);
		GEN_CASE(QMM_SUPERCEDE);
	default:
		return "unknown";
	};
}


bool plugin_load(plugin_t& p, std::string file) {
	// if this plugin_t somehow already has a dll pointer, wipe it first
	if (p.dll)
		plugin_unload(p);

	// load DLL
	if (!(p.dll = dlopen(file.c_str(), RTLD_NOW))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): DLL load failed for plugin: {}\n", file, dlerror());
		goto fail;
	}

	// if this DLL is the same as QMM, cancel
	if ((void*)p.dll == g_gameinfo.qmm_module_ptr) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): DLL is actually QMM?\n", file);
		goto fail;
	}

	if (!(p.QMM_Query = (plugin_query)dlsym(p.dll, "QMM_Query"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Query\" function\n", file);
		goto fail;
	}

	// call initial plugin entry point, get interface version
	p.QMM_Query(&p.plugininfo);
	if (!p.plugininfo) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): QMM_Query() returned NULL Plugininfo\n", file);
		goto fail;
	}

	// if the plugin's major interface version is very high, it is likely an old plugin (pifv < 3:0) and it's actually the name string pointer
	// so we can just grab the pifv values from reserved1 and reserved2 (the old slots) and let the next checks handle it
	if (p.plugininfo->pifv_major > 999) {
		p.plugininfo->pifv_major = p.plugininfo->reserved1;
		p.plugininfo->pifv_minor = p.plugininfo->reserved2;
	}

	// if the plugin's major interface version is lower, don't load and suggest to upgrade plugin
	if (p.plugininfo->pifv_major < QMM_PIFV_MAJOR) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Plugin's interface version ({}:{}) is less than QMM's ({}:{}), suggest upgrading plugin.\n", file, p.plugininfo->pifv_major, p.plugininfo->pifv_minor, QMM_PIFV_MAJOR, QMM_PIFV_MINOR);
		goto fail;
	}
	// if the plugin's interface version is higher, don't load and suggest to upgrade QMM
	else if (p.plugininfo->pifv_major > QMM_PIFV_MAJOR || p.plugininfo->pifv_minor > QMM_PIFV_MINOR) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Plugin's interface version ({}:{}) is greater than QMM's ({}:{}), suggest upgrading QMM.\n", file, p.plugininfo->pifv_major, p.plugininfo->pifv_minor, QMM_PIFV_MAJOR, QMM_PIFV_MINOR);
		goto fail;
	}
	// at this point, major versions match and the plugin's minor version is less than or equal to QMM's

	// find remaining QMM api functions or fail
	if (!(p.QMM_Attach = (plugin_attach)dlsym(p.dll, "QMM_Attach"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Attach\" function\n", file);
		goto fail;
	}
	if (!(p.QMM_Detach = (plugin_detach)dlsym(p.dll, "QMM_Detach"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Detach\" function\n", file);
		goto fail;
	}

	// find hook functions
	if (!(p.QMM_vmMain = (plugin_vmmain)dlsym(p.dll, "QMM_vmMain"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_vmMain\" function\n", file);
		goto fail;
	}
	if (!(p.QMM_syscall = (plugin_syscall)dlsym(p.dll, "QMM_syscall"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_syscall\" function\n", file);
		goto fail;
	}
	if (!(p.QMM_vmMain_Post = (plugin_vmmain)dlsym(p.dll, "QMM_vmMain_Post"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_vmMain_Post\" function\n", file);
		goto fail;
	}
	if (!(p.QMM_syscall_Post = (plugin_syscall)dlsym(p.dll, "QMM_syscall_Post"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_syscall_Post\" function\n", file);
		goto fail;
	}

	// set some pluginvars only available at run-time (this will get repeated for every plugin, but that's ok)
	s_pluginvars.vmbase = g_mod.vmbase;

	// call QMM_Attach. if it fails (returns 0), call QMM_Detach and unload DLL
	// QMM_Attach(engine syscall, mod vmmain, pointer to plugin result int, table of plugin helper functions, table of plugin variables)
	if (!(p.QMM_Attach(g_gameinfo.pfnsyscall, g_mod.pfnvmMain, &g_plugin_globals.plugin_result, &s_pluginfuncs, &s_pluginvars))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): QMM_Attach() returned 0\n", file);
		// treat this failure specially. since this is a valid plugin, but decided on its own that it shouldn't be loaded,
		// we return "true" so that QMM will not try to load the plugin again on a different path
		plugin_unload(p);
		return true;
	}

	p.path = file;
	return true;

fail:
	plugin_unload(p);
	return false;
}


void plugin_unload(plugin_t& p) {
	if (p.dll) {
		if (p.QMM_Detach)
			p.QMM_Detach();
		dlclose(p.dll);
	}

	p = plugin_t();
}


static plugin_t* s_plugin_find_by_id(plid_t plid) {
	for (plugin_t& p : g_plugins) {
		if (p.plugininfo == (plugininfo_t*)plid)
			return &p;
	}

	return nullptr;
}


static void s_plugin_helper_WriteQMMLog(plid_t plid, const char* text, int severity) {
	if (severity < QMM_LOG_TRACE || severity > QMM_LOG_FATAL)
		severity = QMM_LOG_INFO;
	plugininfo_t* plinfo = (plugininfo_t*)plid;
	const char* logtag = plinfo->logtag;
	if (!logtag || !*logtag)
		logtag = plinfo->name;
	LOG(severity, str_toupper(logtag)) << text;
}


static char* s_plugin_helper_VarArgs(plid_t plid, const char* format, ...) {
	va_list	argptr;
	static char str[NUM_PLUGIN_STR_BUFFERS][1024];
	static int index = 0;

	// cycle rotating buffer and store string
	index = (index + 1) % NUM_PLUGIN_STR_BUFFERS;

	va_start(argptr, format);
	vsnprintf(str[index], sizeof(str[index]), format, argptr);
	va_end(argptr);

	char* ret = str[index];

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnVarArgs(\"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, format, ret);

	return ret;
}


static int s_plugin_helper_IsQVM(plid_t plid) {
	int ret = g_mod.vmbase != 0;
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnIsQVM(\"{}\") = {}\n", ((plugininfo_t*)plid)->name, ret);
	return ret;
}


static const char* s_plugin_helper_EngMsgName(plid_t plid, intptr_t msg) {
	const char* ret = g_gameinfo.game->eng_msg_names(msg);
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnEngMsgName(\"{}\", {}) = \"{}\"\n", ((plugininfo_t*)plid)->name, msg, ret);
	return ret;
}


static const char* s_plugin_helper_ModMsgName(plid_t plid, intptr_t msg) {
	const char* ret = g_gameinfo.game->mod_msg_names(msg);
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnModMsgName(\"{}\", {}) = \"{}\"\n", ((plugininfo_t*)plid)->name, msg, ret);
	return ret;
}


static intptr_t s_plugin_helper_GetIntCvar(plid_t plid, const char* cvar) {
	intptr_t ret = 0;
	if (cvar && *cvar)
		ret = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_INTEGER_VALUE], cvar);
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnGetIntCvar(\"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, cvar, ret);
	return ret;
}


#define MAX_CVAR_LEN	1024	// most common cvar buffer size in SDK when calling G_CVAR_VARIABLE_STRING_BUFFER
static const char* s_plugin_helper_GetStrCvar(plid_t plid, const char* cvar) {
	static char str[NUM_PLUGIN_STR_BUFFERS][MAX_CVAR_LEN];
	static int index = 0;

	const char* ret = "";

	if (cvar && *cvar) {
		// cycle rotating buffer and store string
		index = (index + 1) % NUM_PLUGIN_STR_BUFFERS;
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_STRING_BUFFER], cvar, str[index], (intptr_t)sizeof(str[index]));
		ret = str[index];
	}
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnGetStrCvar(\"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, cvar, ret);
	return ret;
}


static const char* s_plugin_helper_GetGameEngine(plid_t plid) {
	const char* ret = g_gameinfo.game->gamename_short;
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnGetGameEngine(\"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, ret);
	return ret;
}


static void s_plugin_helper_Argv(plid_t plid, intptr_t argn, char* buf, intptr_t buflen) {
	qmm_argv(argn, buf, buflen);
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnArgv(\"{}\", {}) = \"{}\"\n", ((plugininfo_t*)plid)->name, argn, buf);
}


// same as the SDK's Info_ValueForKey function
static const char* s_plugin_helper_InfoValueForKey(plid_t plid, const char* userinfo, const char* key) {
	static std::string value[NUM_PLUGIN_STR_BUFFERS];
	static int index = 0;

	const char* ret = "";

	if (userinfo && key) {
		std::string s = userinfo;

		// userinfo strings are "\key\value\key\value\"
		// so search for "\key\" and then get everything up to the next "\"
		std::string fkey = fmt::format("\\{}\\", key);
		size_t keypos = s.find(fkey);
		if (keypos != std::string::npos) {	// key found
			// find next "\"
			size_t valpos = keypos + fkey.size();
			size_t valend = s.find('\\', valpos);
			if (valend == std::string::npos)	// handle case(?) where final value does not end with a "\"
				valend = s.size();

			// get everything between "\key\" and "\"
			std::string fval = s.substr(valpos, valend - valpos);

			// cycle rotating buffer and store string
			index = (index + 1) % NUM_PLUGIN_STR_BUFFERS;
			value[index] = fval;
			ret = value[index].c_str();
		}
	}
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnInfoValueForKey(\"{}\", \"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, userinfo, key, ret);
	return ret;
}


static nlohmann::json s_plugin_cfg_get_node(plid_t plid, std::string key) {
	if (key[0] == '/')
		key = key.substr(1);

	nlohmann::json node = g_cfg;

	size_t sep = key.find('/');
	while (sep != std::string::npos) {
		std::string segment = key.substr(0, sep);
		node = cfg_get_object(node, segment);
		key = key.substr(sep + 1);
		sep = key.find('/', sep + 1);
	}

	return node;
}

static const char* s_plugin_helper_ConfigGetStr(plid_t plid, const char* key) {
	static std::string value[NUM_PLUGIN_STR_BUFFERS];
	static int index = 0;

	nlohmann::json node = s_plugin_cfg_get_node(plid, key);
	
	// cycle rotating buffer and store string
	index = (index + 1) % NUM_PLUGIN_STR_BUFFERS;
	value[index] = cfg_get_string(node, path_basename(key));
	const char* ret = value[index].c_str();
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnConfigGetStr(\"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, key, ret);
	return ret;
}


static int s_plugin_helper_ConfigGetInt(plid_t plid, const char* key) {
	nlohmann::json node = s_plugin_cfg_get_node(plid, key);
	int ret = cfg_get_int(node, path_basename(key));
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnConfigGetInt(\"{}\", \"{}\") = {}\n", ((plugininfo_t*)plid)->name, key, ret);
	return ret;
}


static int s_plugin_helper_ConfigGetBool(plid_t plid, const char* key) {
	nlohmann::json node = s_plugin_cfg_get_node(plid, key);
	int ret = (int)cfg_get_bool(node, path_basename(key));
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnConfigGetBool(\"{}\", \"{}\") = {}\n", ((plugininfo_t*)plid)->name, key, ret);
	return ret;
}


static const char** s_plugin_helper_ConfigGetArrayStr(plid_t plid, const char* key) {
	static std::vector<std::string> value[NUM_PLUGIN_STR_BUFFERS];
	// plugin API needs to be C-compatible, so this vector stores the .c_str() of each string in the value vector
	static std::vector<const char*> valuep[NUM_PLUGIN_STR_BUFFERS];
	static int index = 0;

	nlohmann::json node = s_plugin_cfg_get_node(plid, key);

	// cycle rotating buffer and store array
	index = (index + 1) % NUM_PLUGIN_STR_BUFFERS;
	value[index] = cfg_get_array_str(node, path_basename(key));
	// fill valuep with const char*s from value
	valuep[index].clear();
	for (std::string& s : value[index]) {
		valuep[index].push_back(s.c_str());
	}
	valuep[index].push_back(nullptr);	// null-terminate the array
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin ConfigGetArrayStr(\"{}\", \"{}\") = [{} items]\n", ((plugininfo_t*)plid)->name, key, value[index].size());
	return valuep[index].data();
}


static int* s_plugin_helper_ConfigGetArrayInt(plid_t plid, const char* key) {
	static std::vector<int> value[NUM_PLUGIN_STR_BUFFERS];
	static int index = 0;

	nlohmann::json node = s_plugin_cfg_get_node(plid, key);

	// cycle rotating buffer and store array
	index = (index + 1) % NUM_PLUGIN_STR_BUFFERS;
	value[index] = cfg_get_array_int(node, path_basename(key));
	// insert length of the array as the first element
	value[index].insert(value[index].begin(), (int)value[index].size());
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin ConfigGetArrayInt(\"{}\", \"{}\") = [{} items]\n", ((plugininfo_t*)plid)->name, key, value[index].size());
	return value[index].data();
}


// get a configstring with G_GET_CONFIGSTRING, based on game engine type
static void s_plugin_helper_GetConfigString(plid_t plid, intptr_t argn, char* buf, intptr_t buflen) {
	// char* (*getConfigstring)(int index);
	// void trap_GetConfigstring(int num, char* buffer, int bufferSize);
	// some games don't return pointers because of QVM interaction, so if this returns anything but null
	// (or true?), we probably are in an api game, and need to get the configstring from the return value
	// instead
	intptr_t ret = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_GET_CONFIGSTRING], argn, buf, buflen);
	if (ret > 1)
		strncpyz(buf, (const char*)ret, buflen);
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnGetConfigString(\"{}\", {}) = \"{}\"\n", ((plugininfo_t*)plid)->name, argn, buf);
}

