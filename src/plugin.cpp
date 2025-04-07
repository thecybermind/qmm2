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

static void s_plugin_helper_WriteQMMLog(const char* text, int severity, const char* tag);
static char* s_plugin_helper_VarArgs(const char* format, ...);
static int s_plugin_helper_IsQVM();
static const char* s_plugin_helper_EngMsgName(intptr_t msg);
static const char* s_plugin_helper_ModMsgName(intptr_t msg);
static intptr_t s_plugin_helper_GetIntCvar(const char* cvar);
static const char* s_plugin_helper_GetStrCvar(const char* cvar);
static const char* s_plugin_helper_GetGameEngine();
static void s_plugin_helper_Argv(intptr_t argn, char* buf, intptr_t buflen);
static const char* s_plugin_helper_InfoValueForKey(const char* userinfo, const char* key);

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
	s_plugin_helper_InfoValueForKey
};

pluginres_t g_plugin_result = QMM_UNUSED;
std::vector<plugin_t> g_plugins;

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

bool plugin_load(plugin_t* p, std::string file) {
	if (p->dll)
		return false;

	if (!(p->dll = dlopen(file.c_str(), RTLD_NOW))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): DLL load failed for plugin: {}\n", file, dlerror());
		return false;
	}
	if (!(p->QMM_Query = (plugin_query)dlsym(p->dll, "QMM_Query"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Query\" function\n", file);
		goto fail;
	}

	// call initial plugin entry point, get interface version
	p->QMM_Query(&(p->plugininfo));
	if (!p->plugininfo) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): QMM_Query() returned NULL Plugininfo\n", file);
		goto fail;
	}

	// if the plugin's major interface version is lower, load, but suggest to upgrade plugin
	if (p->plugininfo->pifv_major < QMM_PIFV_MAJOR) {
		LOG(QMM_LOG_WARNING, "QMM") << fmt::format("plugin_load(\"{}\"): Plugin's interface version ({}:{}) is less than QMM's ({}:{}), suggest upgrading plugin.\n", file, p->plugininfo->pifv_major, p->plugininfo->pifv_minor, QMM_PIFV_MAJOR, QMM_PIFV_MINOR);
	}
	// if the plugin's interface version is higher, don't load and suggest to upgrade QMM
	else if (p->plugininfo->pifv_major > QMM_PIFV_MAJOR || p->plugininfo->pifv_minor > QMM_PIFV_MINOR) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Plugin's interface version ({}:{}) is greater than QMM's ({}:{}), suggest upgrading QMM.\n", file, p->plugininfo->pifv_major, p->plugininfo->pifv_minor, QMM_PIFV_MAJOR, QMM_PIFV_MINOR);
		goto fail;
	}
	// don't care if plugin's minor interface version is lower with same major

	// find remaining QMM api functions or fail
	if (!(p->QMM_Attach = (plugin_attach)dlsym(p->dll, "QMM_Attach"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Attach\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_Detach = (plugin_detach)dlsym(p->dll, "QMM_Detach"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_Detach\" function\n", file);
		goto fail;
	}

	// find hook functions
	if (!(p->QMM_vmMain = (plugin_vmmain)dlsym(p->dll, "QMM_vmMain"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_vmMain\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_syscall = (plugin_syscall)dlsym(p->dll, "QMM_syscall"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_syscall\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_vmMain_Post = (plugin_vmmain)dlsym(p->dll, "QMM_vmMain_Post"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_vmMain_Post\" function\n", file);
		goto fail;
	}
	if (!(p->QMM_syscall_Post = (plugin_syscall)dlsym(p->dll, "QMM_syscall_Post"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): Unable to find \"QMM_syscall_Post\" function\n", file);
		goto fail;
	}

	// call QMM_Attach. if it fails (returns 0), call QMM_Detach and unload DLL
	// QMM_Attach(engine syscall, mod vmmain, pointer to plugin result int, table of plugin helper functions, vmbase, reserved)
	if (!(p->QMM_Attach(g_gameinfo.pfnsyscall, g_mod.pfnvmMain, &g_plugin_result, &s_pluginfuncs, g_mod.vmbase, 0))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): QMM_Attach() returned 0\n", file);
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

static void s_plugin_helper_WriteQMMLog(const char* text, int severity, const char* tag) {
	if (severity < QMM_LOG_TRACE || severity > QMM_LOG_FATAL)
		severity = QMM_LOG_INFO;
	LOG(severity, tag) << text;
}

static char* s_plugin_helper_VarArgs(const char* format, ...) {
	va_list	argptr;
	static char str[8][1024];
	static int index = 0;

	index = (index + 1) & 7;

	va_start(argptr, format);
	vsnprintf(str[index], sizeof(str[index]), format, argptr);
	va_end(argptr);

	return str[index];
}

static int s_plugin_helper_IsQVM() {
	return g_mod.vmbase != 0;
}

static const char* s_plugin_helper_EngMsgName(intptr_t msg) {
	return g_gameinfo.game->eng_msg_names(msg);
}

static const char* s_plugin_helper_ModMsgName(intptr_t msg) {
	return g_gameinfo.game->mod_msg_names(msg);
}

static intptr_t s_plugin_helper_GetIntCvar(const char* cvar) {
	if (!cvar || !*cvar)
		return -1;

	return ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_INTEGER_VALUE], cvar);
}

#define MAX_CVAR_LEN	1024	// most common cvar buffer size in SDK when calling G_CVAR_VARIABLE_STRING_BUFFER
// this uses a cycling array of strings so the return value does not need to be stored locally
static const char* s_plugin_helper_GetStrCvar(const char* cvar) {
	static char str[8][MAX_CVAR_LEN];
	static int index = 0;

	if (!cvar || !*cvar)
		return nullptr;

	// cycle rotating buffer and store string
	index = (index + 1) & 7;
	ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_STRING_BUFFER], cvar, str[index], (intptr_t)sizeof(str[index]));
	return str[index];
}

static const char* s_plugin_helper_GetGameEngine() {
	return g_gameinfo.game->gamename_short;
}

// get a given argument with G_ARGV, based on game engine type
static void s_plugin_helper_Argv(intptr_t argn, char* buf, intptr_t buflen) {
	// syscall-based games don't return pointers because of QVM interaction, so if this returns anything but
	// null (or true?), we probably are in an api game, and need to get the arg from the return value instead
	intptr_t ret = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], argn, buf, buflen);
	if (ret > 1)
		strncpy(buf, (char*)ret, buflen);

	buf[buflen - 1] = '\0';
}

// same as the SDK's Info_ValueForKey function
static const char* s_plugin_helper_InfoValueForKey(const char* userinfo, const char* key) {
	static std::string value[8];
	static int index = 0;

	if (!userinfo || !key)
		return "";

	std::string s = userinfo;

	// userinfo strings are "\key\value\key\value\"
	// so search for "\key\" and then get everything up to the next "\"
	std::string fkey = fmt::format("\\{}\\", key);
	size_t keypos = s.find(fkey);
	if (keypos == std::string::npos)	// key not found
		return "";

	// find next "\"
	size_t valpos = keypos + fkey.size();
	size_t valend = s.find('\\', valpos);
	if (valend == std::string::npos)	// handle case(?) where final value does not end with a "\"
		valend = s.size();

	// get everything between "\key\" and "\"
	std::string fval = s.substr(valpos, valend - valpos);

	// cycle rotating buffer and store string
	index = (index + 1) & 7;
	value[index] = fval;
	return value[index].c_str();
}