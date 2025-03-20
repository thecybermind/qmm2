/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include "osdef.h"
#include "log.h"
#include "format.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "mod.h"
#include "plugin.h"
#include "util.h"

static int s_plugin_helper_WriteGameLog(const char* text, int len = -1) {
	return log_write(text, len);
}

static char* s_plugin_helper_VarArgs(const char* format, ...) {
	va_list	argptr;
	static char str[8][1024];
	static int index = 0;
	int i = index;

	va_start(argptr, format);
	vsnprintf(str[i], sizeof(str[i]), format, argptr);
	va_end(argptr);

	index = (index + 1) & 7;
	return str[i];
}

static int s_plugin_helper_IsQVM() {
	return g_mod.vmbase != 0;
}

static const char* s_plugin_helper_EngMsgName(int msg) {
	return g_gameinfo.game->eng_msg_names(msg);
}

static const char* s_plugin_helper_ModMsgName(int msg) {
	return g_gameinfo.game->mod_msg_names(msg);
}

static int s_plugin_helper_GetIntCvar(const char* cvar) {
	return util_get_int_cvar(cvar);
}

static const char* s_plugin_helper_GetStrCvar(const char* cvar) {
	return util_get_str_cvar(cvar);
}

static const char* s_plugin_helper_GetGameEngine() {
	return g_gameinfo.game->gamename_short;
}

static pluginfuncs_t s_pluginfuncs = {
	&s_plugin_helper_WriteGameLog,
	&s_plugin_helper_VarArgs,
	&s_plugin_helper_IsQVM,
	&s_plugin_helper_EngMsgName,
	&s_plugin_helper_ModMsgName,
	&s_plugin_helper_GetIntCvar,
	&s_plugin_helper_GetStrCvar,
	&s_plugin_helper_GetGameEngine
};

pluginres_t g_plugin_result = QMM_UNUSED;
std::vector<plugin_t> g_plugins;

bool plugin_load(plugin_t* p, std::string file) {
	if (p->dll)
		return false;

	if (!(p->dll = dlopen(file.c_str(), RTLD_NOW))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): DLL load failed for plugin: {}\n", file, dlerror());
		return false;
	}
	if (!(p->QMM_Query = (plugin_query)dlsym(p->dll, "QMM_Query"))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Query\" function\n", file);
		goto fail;
	}

	p->QMM_Query(&(p->plugininfo));
	if (!p->plugininfo) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): QMM_Query() returned NULL Plugininfo\n", file);
		goto fail;
	}

	// if the plugin's interface version is higher, don't load and suggest to upgrade QMM
	if (p->plugininfo->pifv_major > QMM_PIFV_MAJOR || p->plugininfo->pifv_minor > QMM_PIFV_MINOR) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Plugin's interface version ({}:{}) is greater than QMM's ({}:{}), suggest upgrading QMM.\n", file, p->plugininfo->pifv_major, p->plugininfo->pifv_minor, QMM_PIFV_MAJOR, QMM_PIFV_MINOR);
		goto fail;
	}
	// if the plugin's major version is lower, load, but suggest to upgrade plugin
	if (p->plugininfo->pifv_major < QMM_PIFV_MAJOR) {
		LOG(WARNING, "QMM") << fmt::format("plugin_load(\"{}\"): Plugin's interface version ({}:{}) is less than QMM's ({}:{}), suggest upgrading plugin.\n", file, p->plugininfo->pifv_major, p->plugininfo->pifv_minor, QMM_PIFV_MAJOR, QMM_PIFV_MINOR);
	}
	// don't care if plugin's minor version is lower

	// find remaining neccesary functions or fail
	if (!(p->QMM_Attach = (plugin_attach)dlsym(p->dll, "QMM_Attach"))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Attach\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_Detach = (plugin_detach)dlsym(p->dll, "QMM_Detach"))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Detach\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_vmMain = (plugin_vmmain)dlsym(p->dll, "QMM_vmMain"))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_vmMain\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_syscall = (plugin_syscall)dlsym(p->dll, "QMM_syscall"))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_syscall\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_vmMain_Post = (plugin_vmmain)dlsym(p->dll, "QMM_vmMain_Post"))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_vmMain_Post\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_syscall_Post = (plugin_syscall)dlsym(p->dll, "QMM_syscall_Post"))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_syscall_Post\" function\n", file);
		goto fail;
	}

	// call QMM_Attach. if it fails (returns 0), call QMM_Detach and unload DLL
	// QMM_Attach(engine syscall, mod vmmain, pointer to plugin result int, table of plugin helper functions, vmbase, reserved)
	if (!(p->QMM_Attach(g_gameinfo.pfnsyscall, g_mod.pfnvmMain, &g_plugin_result, &s_pluginfuncs, g_mod.vmbase, 0))) {
		LOG(ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): QMM_Attach() returned 0\n", file);
		p->QMM_Detach(0); // int arg is reserved, previously iscmd
		goto fail;
	}

	p->path = file;
	return true;

	fail:
	if (p->dll)
		dlclose(p->dll);
	*p = plugin_t();
	return false;
}

void plugin_unload(plugin_t* p) {
	if (p->dll) {
		if (p->QMM_Detach)
			p->QMM_Detach(0); // int arg is reserved, previously iscmd
		dlclose(p->dll);
	}

	*p = plugin_t();
}
