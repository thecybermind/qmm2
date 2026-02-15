/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS
#include "osdef.h"
#include <cstdarg>
#include <cstdio>
#include <vector>
#include <string>
#include "qmmapi.h"
#include "game_api.h"
#include "log.h"
#include "format.h"
#include "config.h"
#include "main.h"
#include "mod.h"
#include "plugin.h"
#include "util.h"

constexpr int NUM_PLUGIN_STR_BUFFERS = 16;  // must be power of 2
constexpr int NUM_PLUGIN_STR_BUFFER_MASK = NUM_PLUGIN_STR_BUFFERS - 1;

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
static int s_plugin_helper_PluginBroadcast(plid_t plid, const char* message, void* buf, intptr_t buflen);
static int s_plugin_helper_PluginSend(plid_t plid, plid_t to_plid, const char* message, void* buf, intptr_t buflen);
static int s_plugin_helper_QVMRegisterFunc(plid_t plid);
static int s_plugin_helper_QVMExecFunc(plid_t plid, int funcid, int argc, int* argv);

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
    s_plugin_helper_PluginBroadcast,
    s_plugin_helper_PluginSend,
    s_plugin_helper_QVMRegisterFunc,
    s_plugin_helper_QVMExecFunc,
};

// struct to store all the globals available to plugins
plugin_globals_t g_plugin_globals = {
    0,			// final_return
    0,			// orig_return
    QMM_UNUSED,	// high_result
    QMM_UNUSED,	// plugin_result
};

std::vector<plugin_t> g_plugins;

// store registered QVM function IDs for plugins
std::map<int, plugin_t*> g_registered_qvm_funcs;
static int s_next_qvm_func = QMM_QVM_FUNC_STARTING_ID;

static pluginvars_t s_pluginvars = {
    0,				// vmbase, set in plugin_load
    &g_plugin_globals.final_return,
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


// returns: -1 if failed to load and don't continue, 0 if failed to load and continue, 1 if loaded
int plugin_load(plugin_t& p, std::string file) {
    int ret = 0;

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
        // treat this failure specially. this is a valid DLL, but it is QMM
        ret = -1;
        goto fail;
    }

    // if this DLL is the same as another loaded plugin, cancel
    for (plugin_t& t : g_plugins) {
        if (p.dll == t.dll) {
            LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): DLL is already loaded as plugin\n", file);
            // treat this failure specially. this is a valid plugin, but it is already loaded
            ret = -1;
            goto fail;
        }
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

    // find optional plugin functions
    p.QMM_PluginMessage = (plugin_pluginmessage)dlsym(p.dll, "QMM_PluginMessage");
    p.QMM_QVMHandler = (plugin_qvmhandler)dlsym(p.dll, "QMM_QVMHandler");

    // set some pluginvars only available at run-time (this will get repeated for every plugin, but that's ok)
    s_pluginvars.vmbase = g_mod.vmbase;

    // call QMM_Attach. if it fails (returns 0), call QMM_Detach and unload DLL
    // QMM_Attach(engine syscall, mod vmmain, pointer to plugin result int, table of plugin helper functions, table of plugin variables)
    if (!(p.QMM_Attach(g_gameinfo.pfnsyscall, g_gameinfo.pfnvmMain, &g_plugin_globals.plugin_result, &s_pluginfuncs, &s_pluginvars))) {
        LOG(QMM_LOG_ERROR, "QMM") << fmt::format("plugin_load(\"{}\"): QMM_Attach() returned 0\n", file);
        // treat this failure specially. this is a valid plugin, but it decided on its own that it shouldn't be loaded
        ret = -1;
        goto fail;
    }

    p.path = file;
    ret = 1;
    return ret;

fail:
    plugin_unload(p);
    return ret;
}


void plugin_unload(plugin_t& p) {
    if (p.dll) {
        if (p.QMM_Detach)
            p.QMM_Detach();
        dlclose(p.dll);
    }

    p = plugin_t();
}


static void s_plugin_helper_WriteQMMLog(plid_t plid [[maybe_unused]], const char* text, int severity) {
    if (severity < QMM_LOG_TRACE || severity > QMM_LOG_FATAL)
        severity = QMM_LOG_INFO;
    plugininfo_t* plinfo = (plugininfo_t*)plid;
    const char* logtag = plinfo->logtag;
    if (!logtag || !*logtag)
        logtag = plinfo->name;
    LOG(severity, str_toupper(logtag)) << text;
}


static char* s_plugin_helper_VarArgs(plid_t plid [[maybe_unused]], const char* format, ...) {
    va_list	argptr;
    static char str[NUM_PLUGIN_STR_BUFFERS][1024];
    static int index = 0;

    // cycle rotating buffer and store string
    index = (index + 1) & NUM_PLUGIN_STR_BUFFER_MASK;

    va_start(argptr, format);
    vsnprintf(str[index], sizeof(str[index]), format, argptr);
    va_end(argptr);

    char* ret = str[index];

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnVarArgs(\"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, format, ret);
#endif
    return ret;
}


static int s_plugin_helper_IsQVM(plid_t plid [[maybe_unused]]) {
    int ret = g_mod.vmbase != 0;
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnIsQVM(\"{}\") = {}\n", ((plugininfo_t*)plid)->name, ret);
#endif
    return ret;
}


static const char* s_plugin_helper_EngMsgName(plid_t plid [[maybe_unused]], intptr_t msg) {
    const char* ret = g_gameinfo.game->eng_msg_names(msg);
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnEngMsgName(\"{}\", {}) = \"{}\"\n", ((plugininfo_t*)plid)->name, msg, ret);
#endif
    return ret;
}


static const char* s_plugin_helper_ModMsgName(plid_t plid [[maybe_unused]], intptr_t msg) {
    const char* ret = g_gameinfo.game->mod_msg_names(msg);
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnModMsgName(\"{}\", {}) = \"{}\"\n", ((plugininfo_t*)plid)->name, msg, ret);
#endif
    return ret;
}


static intptr_t s_plugin_helper_GetIntCvar(plid_t plid [[maybe_unused]], const char* cvar) {
    intptr_t ret = 0;
    if (cvar && *cvar)
        ret = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_INTEGER_VALUE], cvar);
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnGetIntCvar(\"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, cvar, ret);
#endif
    return ret;
}


#define MAX_CVAR_LEN	1024	// most common cvar buffer size in SDK when calling G_CVAR_VARIABLE_STRING_BUFFER
static const char* s_plugin_helper_GetStrCvar(plid_t plid [[maybe_unused]], const char* cvar) {
    static char str[NUM_PLUGIN_STR_BUFFERS][MAX_CVAR_LEN];
    static int index = 0;

    const char* ret = "";

    if (cvar && *cvar) {
        // cycle rotating buffer and store string
        index = (index + 1) & NUM_PLUGIN_STR_BUFFER_MASK;
        ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_STRING_BUFFER], cvar, str[index], (intptr_t)sizeof(str[index]));
        ret = str[index];
    }
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnGetStrCvar(\"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, cvar, ret);
#endif
    return ret;
}


static const char* s_plugin_helper_GetGameEngine(plid_t plid [[maybe_unused]]) {
    const char* ret = g_gameinfo.game->gamename_short;
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnGetGameEngine(\"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, ret);
#endif
    return ret;
}


static void s_plugin_helper_Argv(plid_t plid [[maybe_unused]], intptr_t argn, char* buf, intptr_t buflen) {
    qmm_argv(argn, buf, buflen);
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnArgv(\"{}\", {}) = \"{}\"\n", ((plugininfo_t*)plid)->name, argn, buf);
#endif
}


// same as the SDK's Info_ValueForKey function
static const char* s_plugin_helper_InfoValueForKey(plid_t plid [[maybe_unused]], const char* userinfo, const char* key) {
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
            index = (index + 1) & NUM_PLUGIN_STR_BUFFER_MASK;
            value[index] = fval;
            ret = value[index].c_str();
        }
    }
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnInfoValueForKey(\"{}\", \"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, userinfo, key, ret);
#endif
    return ret;
}


static nlohmann::json s_plugin_cfg_get_node(plid_t plid [[maybe_unused]], std::string key) {
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

static const char* s_plugin_helper_ConfigGetStr(plid_t plid [[maybe_unused]], const char* key) {
    static std::string value[NUM_PLUGIN_STR_BUFFERS];
    static int index = 0;

    nlohmann::json node = s_plugin_cfg_get_node(plid, key);

    // cycle rotating buffer and store string
    index = (index + 1) & NUM_PLUGIN_STR_BUFFER_MASK;
    value[index] = cfg_get_string(node, path_basename(key));
    const char* ret = value[index].c_str();
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnConfigGetStr(\"{}\", \"{}\") = \"{}\"\n", ((plugininfo_t*)plid)->name, key, ret);
    return ret;
}


static int s_plugin_helper_ConfigGetInt(plid_t plid [[maybe_unused]], const char* key) {
    nlohmann::json node = s_plugin_cfg_get_node(plid, key);
    int ret = cfg_get_int(node, path_basename(key));
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnConfigGetInt(\"{}\", \"{}\") = {}\n", ((plugininfo_t*)plid)->name, key, ret);
    return ret;
}


static int s_plugin_helper_ConfigGetBool(plid_t plid [[maybe_unused]], const char* key) {
    nlohmann::json node = s_plugin_cfg_get_node(plid, key);
    int ret = (int)cfg_get_bool(node, path_basename(key));
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnConfigGetBool(\"{}\", \"{}\") = {}\n", ((plugininfo_t*)plid)->name, key, ret);
    return ret;
}


static const char** s_plugin_helper_ConfigGetArrayStr(plid_t plid [[maybe_unused]], const char* key) {
    static std::vector<std::string> value[NUM_PLUGIN_STR_BUFFERS];
    // plugin API needs to be C-compatible, so this vector stores the .c_str() of each string in the value vector
    static std::vector<const char*> valuep[NUM_PLUGIN_STR_BUFFERS];
    static int index = 0;

    nlohmann::json node = s_plugin_cfg_get_node(plid, key);

    // cycle rotating buffer and store array
    index = (index + 1) & NUM_PLUGIN_STR_BUFFER_MASK;
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


static int* s_plugin_helper_ConfigGetArrayInt(plid_t plid [[maybe_unused]], const char* key) {
    static std::vector<int> value[NUM_PLUGIN_STR_BUFFERS];
    static int index = 0;

    nlohmann::json node = s_plugin_cfg_get_node(plid, key);

    // cycle rotating buffer and store array
    index = (index + 1) & NUM_PLUGIN_STR_BUFFER_MASK;
    value[index] = cfg_get_array_int(node, path_basename(key));
    // insert length of the array as the first element
    value[index].insert(value[index].begin(), (int)value[index].size());
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin ConfigGetArrayInt(\"{}\", \"{}\") = [{} items]\n", ((plugininfo_t*)plid)->name, key, value[index].size());
    return value[index].data();
}


// get a configstring with G_GET_CONFIGSTRING, based on game engine type
static void s_plugin_helper_GetConfigString(plid_t plid [[maybe_unused]], intptr_t argn, char* buf, intptr_t buflen) {
    // char* (*getConfigstring)(int index);
    // void trap_GetConfigstring(int num, char* buffer, int bufferSize);
    // some games don't return pointers because of QVM interaction, so if this returns anything but null
    // (or true?), we probably are in an api game, and need to get the configstring from the return value
    // instead
    intptr_t ret = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_GET_CONFIGSTRING], argn, buf, buflen);
    if (ret > 1)
        strncpyz(buf, (const char*)ret, (size_t)buflen);
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnGetConfigString(\"{}\", {}) = \"{}\"\n", ((plugininfo_t*)plid)->name, argn, buf);
#endif
}


// broadcast a message to plugins' QMM_PluginMessage() functions
static int s_plugin_helper_PluginBroadcast(plid_t plid [[maybe_unused]], const char* message, void* buf, intptr_t buflen) {
    // count how many plugins were called
    int total = 0;
    for (plugin_t& p : g_plugins) {
        // skip the calling plugin
        if (p.plugininfo == (plugininfo_t*)plid)
            continue;
        // skip if the plugin doesn't have the function
        if (!p.QMM_PluginMessage)
            continue;
        p.QMM_PluginMessage(plid, message, buf, buflen, 1); // 1 = is_broadcast
        total++;
    }
#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnPluginBroadcast(\"{}\", \"{}\", {}, {}) = {} plugins called\n", ((plugininfo_t*)plid)->name, message, buf, buflen, total);
#endif
    return total;
}


// send a message to a specific plugin's QMM_PluginMessage() functions
static int s_plugin_helper_PluginSend(plid_t plid [[maybe_unused]], plid_t to_plid, const char* message, void* buf, intptr_t buflen) {
    // don't let a plugin call itself
    if (plid == to_plid)
        return 0;

    for (plugin_t& p : g_plugins) {
        // if this is the destination plugin
        if (p.plugininfo == (plugininfo_t*)to_plid) {
            // if the plugin doesn't have the message function
            if (!p.QMM_PluginMessage)
                return 0;
            p.QMM_PluginMessage(plid, message, buf, buflen, 0); // 0 = is_broadcast
#ifdef _DEBUG
            LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnPluginSend(\"{}\", \"{}\", \"{}\", {}, {}) called\n", ((plugininfo_t*)plid)->name, ((plugininfo_t*)to_plid)->name, message, buf, buflen);
#endif
            return 1;
        }
    }
    return 0;
}


// register a new QVM function ID to the calling plugin (0 if unsuccessful)
static int s_plugin_helper_QVMRegisterFunc(plid_t plid [[maybe_unused]]) {
    int ret = 0;

    // find the plugin_t for this plugin
    for (plugin_t& p : g_plugins) {
        // found it
        if (p.plugininfo == (plugininfo_t*)plid) {
            // make sure the plugin actually has a QVM handler func
            if (!p.QMM_QVMHandler)
                break;

            // associate plugin with ID
            g_registered_qvm_funcs[s_next_qvm_func] = &p;

            // return negative-1 form of ID for storing in a QVM function pointer
            ret = -s_next_qvm_func - 1;

            s_next_qvm_func++;
            break;
        }
    }

    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnRegisterQVMFunc(\"{}\") = {}\n", ((plugininfo_t*)plid)->name, ret);

    return ret;
}


// exec a given QVM function function ID
static int s_plugin_helper_QVMExecFunc(plid_t plid [[maybe_unused]], int instruction, int argc, int* argv) {
    int ret = qvm_exec_ex(&g_mod.qvm, (size_t)instruction, argc, argv);

#ifdef _DEBUG
    LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("Plugin pfnQVMExecFunc(\"{}\", {}, {}) = {}\n", ((plugininfo_t*)plid)->name, funcid, argc, ret);
#endif

    return ret;
}
