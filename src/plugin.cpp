/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include "osdef.h"
#include "format.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "CModMgr.h"
#include "main.h"
#include "plugin.h"
#include "util.h"

static int s_plugin_helper_WriteGameLog(const char* text, int len = -1) {
	return log_write(text, len);
}

static char* s_plugin_helper_VarArgs(char* format, ...) {
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
	return g_ModMgr->Mod()->IsVM();
}

static const char* s_plugin_helper_EngMsgName(int x) {
	return ENG_MSGNAME(x);
}

static const char* s_plugin_helper_ModMsgName(int x) {
	return MOD_MSGNAME(x);
}

static int s_plugin_helper_GetIntCvar(const char* cvar) {
	return get_int_cvar(cvar);
}

static const char* s_plugin_helper_GetStrCvar(const char* cvar) {
	return get_str_cvar(cvar);
}

pluginfuncs_t* get_pluginfuncs() {
	static pluginfuncs_t pluginfuncs = {
		&s_plugin_helper_WriteGameLog,
		&s_plugin_helper_VarArgs,
		&s_plugin_helper_IsQVM,
		&s_plugin_helper_EngMsgName,
		&s_plugin_helper_ModMsgName,
		&s_plugin_helper_GetIntCvar,
		&s_plugin_helper_GetStrCvar,
	};
	return &pluginfuncs;
}

std::vector<plugin_t> g_plugins;

// this is just a non-MFP stub to pass to plugins
static int s_plugin_vmmain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	return MOD_VMMAIN(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
}

bool plugin_load(plugin_t* p, std::string file) {
	if (p->dll) {
		plugin_unload(p);
	}
	p->path = file;

	if (!(p->dll = dlopen(file.c_str(), RTLD_NOW))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): DLL load failed for plugin: {}\n", file, dlerror()).c_str());
		return false;
	}
	if (!(p->QMM_Query = (plugin_query)dlsym(p->dll, "QMM_Query"))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Unable to find \"QMM_Query\" function\n", file).c_str());
		goto fail;
	}

	p->QMM_Query(&(p->plugininfo), g_gameinfo.game->gamename_short);
	if (!p->plugininfo) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): QMM_Query() returned NULL Plugininfo\n", file).c_str());
		goto fail;
	}

	// if the plugin's major version is higher, don't load and suggest to upgrade QMM
	if (p->plugininfo->pifv_major > QMM_PIFV_MAJOR) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Plugin's major interface version ({}) is greater than QMM's ({}), suggest upgrading QMM.\n", file, p->plugininfo->pifv_major, QMM_PIFV_MAJOR).c_str());
		goto fail;
	}
	// if the plugin's major version is lower, don't load and suggest to upgrade plugin
	if (p->plugininfo->pifv_major < QMM_PIFV_MAJOR) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Plugin's major interface version ({}) is less than QMM's ({}), suggest upgrading plugin.\n", file, p->plugininfo->pifv_major, QMM_PIFV_MAJOR).c_str());
		goto fail;
	}
	// if the plugin's minor version is higher, don't load and suggest to upgrade QMM
	if (p->plugininfo->pifv_minor > QMM_PIFV_MINOR) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Plugin's minor interface version ({}) is greater than QMM's ({}), suggest upgrading QMM.\n", file, p->plugininfo->pifv_minor, QMM_PIFV_MINOR).c_str());
		goto fail;
	}
	// if the plugin's minor version is lower, load, but suggest to upgrade plugin anyway
	if (p->plugininfo->pifv_minor < QMM_PIFV_MINOR) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] WARNING: plugin_load(\"{}\"): Plugin's minor interface version ({}) is less than QMM's ({}), suggest upgrading plugin.\n", file, p->plugininfo->pifv_minor, QMM_PIFV_MINOR).c_str());
	}

	// find remaining neccesary functions or fail
	if (!(p->QMM_Attach = (plugin_attach)dlsym(p->dll, "QMM_Attach"))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Unable to find \"QMM_Attach\" function\n", file).c_str());
		goto fail;
	}
	if (!(p->QMM_Detach = (plugin_detach)dlsym(p->dll, "QMM_Detach"))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Unable to find \"QMM_Detach\" function\n", file).c_str());
		goto fail;
	}
	if (!(p->QMM_vmMain = (plugin_vmmain)dlsym(p->dll, "QMM_vmMain"))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Unable to find \"QMM_vmMain\" function\n", file).c_str());
		goto fail;
	}
	if (!(p->QMM_syscall = (plugin_syscall)dlsym(p->dll, "QMM_syscall"))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Unable to find \"QMM_syscall\" function\n", file).c_str());
		goto fail;
	}
	if (!(p->QMM_vmMain_Post = (plugin_vmmain)dlsym(p->dll, "QMM_vmMain_Post"))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Unable to find \"QMM_vmMain_Post\" function\n", file).c_str());
		goto fail;
	}
	if (!(p->QMM_syscall_Post = (plugin_syscall)dlsym(p->dll, "QMM_syscall_Post"))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): Unable to find \"QMM_syscall_Post\" function\n", file).c_str());
		goto fail;
	}

	// call QMM_Attach. if it fails (returns 0), call QMM_Detach and unload DLL
	// QMM_Attach(engine syscall, mod vmmain, pointer to plugin result int, table of plugin helper functions, vmbase)
	if (!(p->QMM_Attach(ENG_SYSCALL, s_plugin_vmmain, &p->result, get_pluginfuncs(), g_ModMgr->Mod()->GetBase()))) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: plugin_load(\"{}\"): QMM_Attach() returned 0\n", file).c_str());
		p->QMM_Detach();
		goto fail;
	}

	return true;

	fail:
	if (p->dll)
		dlclose(p->dll);
	return false;
}

void plugin_unload(plugin_t* p) {
	if (!p->dll)
		return;
	if (p->QMM_Detach)
		p->QMM_Detach();
	dlclose(p->dll);
}
