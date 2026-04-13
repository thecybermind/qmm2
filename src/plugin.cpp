/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS
#include <cstdarg>
#include <vector>
#include <string>
#include <filesystem>
#include "qmmapi.h"
#include "gameapi.hpp"
#include "log.hpp"
#include "config.hpp"
#include "gameinfo.hpp"
#include "main.hpp"     // ArgV
#include "mod.hpp"
#include "plugin.hpp"
#include "qvm.h"
#include "util.hpp"

constexpr int ROTATING_BUFFER_NUM = 16;  // must be power of 2
constexpr int ROTATING_BUFFER_MASK = ROTATING_BUFFER_NUM - 1;
constexpr int ROTATING_BUFFER_SIZE = 1024;

static void s_plugin_helper_WriteQMMLog(plugin_id plid, int severity, const char* fmt, ...);
static char* s_plugin_helper_VarArgs(plugin_id plid [[maybe_unused]], const char* fmt, ...);
static int s_plugin_helper_IsQVM(plugin_id plid [[maybe_unused]]);
static const char* s_plugin_helper_EngMsgName(plugin_id plid [[maybe_unused]], intptr_t msg);
static const char* s_plugin_helper_ModMsgName(plugin_id plid [[maybe_unused]], intptr_t msg);
static intptr_t s_plugin_helper_GetIntCvar(plugin_id plid [[maybe_unused]], const char* cvar);
static const char* s_plugin_helper_GetStrCvar(plugin_id plid [[maybe_unused]], const char* cvar);
static const char* s_plugin_helper_GetGameEngine(plugin_id plid [[maybe_unused]]);
static void s_plugin_helper_Argv(plugin_id plid [[maybe_unused]], intptr_t argn, char* buf, intptr_t buflen);
static const char* s_plugin_helper_InfoValueForKey(plugin_id plid [[maybe_unused]], const char* userinfo, const char* key);
static const char* s_plugin_helper_ConfigGetStr(plugin_id plid [[maybe_unused]], const char* key);
static int s_plugin_helper_ConfigGetInt(plugin_id plid [[maybe_unused]], const char* key);
static int s_plugin_helper_ConfigGetBool(plugin_id plid [[maybe_unused]], const char* key);
static const char** s_plugin_helper_ConfigGetArrayStr(plugin_id plid [[maybe_unused]], const char* key);
static int* s_plugin_helper_ConfigGetArrayInt(plugin_id plid [[maybe_unused]], const char* key);
static void s_plugin_helper_GetConfigString(plugin_id plid [[maybe_unused]], intptr_t index, char* buf, intptr_t buflen);
static int s_plugin_helper_PluginBroadcast(plugin_id plid, const char* message, void* buf, intptr_t buflen);
static int s_plugin_helper_PluginSend(plugin_id plid, plugin_id to_plid, const char* message, void* buf, intptr_t buflen);
static int s_plugin_helper_QVMRegisterFunc(plugin_id plid);
static int s_plugin_helper_QVMExecFunc(plugin_id plid [[maybe_unused]], int funcid, int argc, int* argv);
static const char* s_plugin_helper_Argv2(plugin_id plid [[maybe_unused]], intptr_t argn);
static const char* s_plugin_helper_GetConfigString2(plugin_id plid [[maybe_unused]], intptr_t index);
static const char* s_plugin_helper_ModDir(plugin_id plid [[maybe_unused]]);

// Struct of plugin helper functions
static plugin_funcs s_pluginfuncs = {
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
    s_plugin_helper_Argv2,
    s_plugin_helper_GetConfigString2,
    s_plugin_helper_ModDir,
};

// This holds global variables that are available to plugins via helper functions.
plugin_globals g_plugin_globals = {
    0,			// final_return
    0,			// orig_return
    QMM_UNUSED,	// high_result
    QMM_UNUSED,	// plugin_result
};

// List of QMM plugins
std::vector<Plugin> g_plugins;

// This holds pseudo-syscall IDs. They are registered to a given plugin, and when the QVM interpreter executes the
// syscall ID, the plugin's QMM_QVMHandler function is called.
std::map<int, Plugin*> g_registered_qvm_funcs;

// This is the next pseudo-syscall ID to return to plugins.
static int s_next_qvm_func = QMM_QVM_FUNC_STARTING_ID;

// Struct of variables to pass to plugins' QMM_Attach
static plugin_vars s_pluginvars = {
    0,				// vmbase, set in plugin_load
    &g_plugin_globals.final_return,
    &g_plugin_globals.orig_return,
    &g_plugin_globals.high_result,
};


// Wrapper syscall function to pass to plugins
static intptr_t s_plugin_game_syscall(intptr_t cmd, ...) {
    QMM_GET_SYSCALL_ARGS();
    return gameinfo.game->syscall(cmd, QMM_PUT_SYSCALL_ARGS());
}


// Wrapper vmMain function to pass to plugins
static intptr_t s_plugin_game_vmMain(intptr_t cmd, ...) {
    QMM_GET_VMMAIN_ARGS();
    return gameinfo.game->vmMain(cmd, QMM_PUT_VMMAIN_ARGS());
}


Plugin::Plugin() : dll(nullptr), QMM_Query(nullptr), QMM_Attach(nullptr), QMM_Detach(nullptr),
    QMM_vmMain(nullptr), QMM_vmMain_Post(nullptr), QMM_syscall(nullptr), QMM_syscall_Post(nullptr),
    QMM_PluginMessage(nullptr), QMM_QVMHandler(nullptr), plugininfo(nullptr)
{
}


Plugin::~Plugin() {
    this->Unload();
}


Plugin::Plugin(Plugin&& other) noexcept : Plugin() {
    if (this == &other)
        return;

    *this = std::move(other);
}


Plugin& Plugin::operator=(Plugin&& other) noexcept {
    if (this == &other)
        return *this;

    std::swap(this->dll, other.dll);
    this->path = other.path;
    this->QMM_Query = other.QMM_Query;
    this->QMM_Attach = other.QMM_Attach;
    this->QMM_Detach = other.QMM_Detach;
    this->QMM_vmMain = other.QMM_vmMain;
    this->QMM_vmMain_Post = other.QMM_vmMain_Post;
    this->QMM_syscall = other.QMM_syscall;
    this->QMM_syscall_Post = other.QMM_syscall_Post;
    this->QMM_PluginMessage = other.QMM_PluginMessage;
    this->QMM_QVMHandler = other.QMM_QVMHandler;
    this->plugininfo = other.plugininfo;

    return *this;
}


int Plugin::Load(std::string file) {
    // if this plugin somehow already has a dll pointer, cancel
    if (this->dll)
        return 0;

    // load DLL
    if (!(this->dll = dll_load(file.c_str()))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << file << "\"): DLL load failed for plugin: " << dll_error() << "\n";
        return 0;
    }

    // if this DLL is the same as QMM, cancel
    if (this->dll == gameinfo.qmm_module_ptr) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): DLL is actually QMM?\n";
        // treat this failure specially. this is a valid DLL, but it is QMM
        return -1;
    }

    // if this DLL is the same as another loaded plugin, cancel
    for (Plugin& t : g_plugins) {
        if (this->dll == t.dll) {
            QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): DLL is already loaded as plugin\n";
            // treat this failure specially. this is a valid plugin, but it is already loaded
            return -1;
        }
    }

    if (!(this->QMM_Query = (Plugin::plugin_query)dll_symbol(this->dll, "QMM_Query"))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Unable to find \"QMM_Query\" function\n";
        return 0;
    }

    // call initial plugin entry point, get interface version
    this->QMM_Query(&this->plugininfo);
    if (!this->plugininfo) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): QMM_Query() returned NULL Plugininfo\n";
        return 0;
    }

    // if the plugin's major interface version is very high, it is likely an old plugin (pifv < 3:0) and it's actually
    // the name string pointer, so we can just grab the pifv values from reserved1 and reserved2 (the old slots) and
    // let the next checks handle it
    if (this->plugininfo->pifv_major > 999) {
        this->plugininfo->pifv_major = this->plugininfo->reserved1;
        this->plugininfo->pifv_minor = this->plugininfo->reserved2;
    }

    // if the plugin's major interface version is lower, don't load and suggest to upgrade plugin
    if (this->plugininfo->pifv_major < QMM_PIFV_MAJOR) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Plugin's interface version (" << this->plugininfo->pifv_major << ":" << this->plugininfo->pifv_minor << ") is less than QMM's (" STRINGIFY(QMM_PIFV_MAJOR) ":" STRINGIFY(QMM_PIFV_MINOR) "), suggest upgrading plugin.\n";
        return 0;
    }
    // if the plugin's interface version is higher, don't load and suggest to upgrade QMM
    else if (this->plugininfo->pifv_major > QMM_PIFV_MAJOR || this->plugininfo->pifv_minor > QMM_PIFV_MINOR) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Plugin's interface version (" << this->plugininfo->pifv_major << ":" << this->plugininfo->pifv_minor << ") is greater than QMM's (" STRINGIFY(QMM_PIFV_MAJOR) ":" STRINGIFY(QMM_PIFV_MINOR) "), suggest upgrading QMM.\n";
        return 0;
    }
    // at this point, major versions match and the plugin's minor version is less than or equal to QMM's

    // find remaining QMM api functions or fail
    if (!(this->QMM_Attach = (Plugin::plugin_attach)dll_symbol(this->dll, "QMM_Attach"))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Unable to find \"QMM_Attach\" function\n";
        return 0;
    }
    if (!(this->QMM_Detach = (Plugin::plugin_detach)dll_symbol(this->dll, "QMM_Detach"))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Unable to find \"QMM_Detach\" function\n";
        return 0;
    }

    // find hook callback functions
    if (!(this->QMM_vmMain = (Plugin::plugin_callback)dll_symbol(this->dll, "QMM_vmMain"))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Unable to find \"QMM_vmMain\" function\n";
        return 0;
    }
    if (!(this->QMM_syscall = (Plugin::plugin_callback)dll_symbol(this->dll, "QMM_syscall"))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Unable to find \"QMM_syscall\" function\n";
        return 0;
    }
    if (!(this->QMM_vmMain_Post = (Plugin::plugin_callback)dll_symbol(this->dll, "QMM_vmMain_Post"))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Unable to find \"QMM_vmMain_Post\" function\n";
        return 0;
    }
    if (!(this->QMM_syscall_Post = (Plugin::plugin_callback)dll_symbol(this->dll, "QMM_syscall_Post"))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): Unable to find \"QMM_syscall_Post\" function\n";
        return 0;
    }

    // set some pluginvars only available at run-time (this will get repeated for every plugin, but that's ok)
    s_pluginvars.vmbase = (intptr_t)g_mod.vm.memory;

    // call QMM_Attach. if it fails (returns 0), destructor will call QMM_Detach and unload DLL
    // QMM_Attach(engine syscall, mod vmmain, pointer to plugin result int, table of plugin helper functions, table of plugin variables)
    if (!(this->QMM_Attach(s_plugin_game_syscall, s_plugin_game_vmMain, &g_plugin_globals.plugin_result, &s_pluginfuncs, &s_pluginvars))) {
        QMMLOG(QMM_LOG_ERROR, "QMM") << "plugin_load(\"" << path_basename(file) << "\"): QMM_Attach() returned 0\n";
        // treat this failure specially. this is a valid plugin, but it decided on its own that it shouldn't be loaded
        return -1;
    }

    this->path = file;
    this->QMM_PluginMessage = (Plugin::plugin_pluginmessage)dll_symbol(this->dll, "QMM_PluginMessage");
    this->QMM_QVMHandler = (Plugin::plugin_qvmhandler)dll_symbol(this->dll, "QMM_QVMHandler");

    return 1;
}


void Plugin::Unload() {
    if (this->dll) {
        if (this->QMM_Detach)
            this->QMM_Detach();
        dll_close(this->dll);
    }
    this->dll = nullptr;
    this->path.clear();
    this->QMM_Query = nullptr;
    this->QMM_Attach = nullptr;
    this->QMM_Detach = nullptr;
    this->QMM_vmMain = nullptr;
    this->QMM_vmMain_Post = nullptr;
    this->QMM_syscall = nullptr;
    this->QMM_syscall_Post = nullptr;
    this->QMM_PluginMessage = nullptr;
    this->QMM_QVMHandler = nullptr;
    this->plugininfo = nullptr;
}


const char* Plugin::plugin_result_to_str(plugin_res res) {
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


/**
* @brief Write to the QMM log.
*
* @param plid Plugin ID of the calling plugin
* @param severity Log message severity
* @param fmt Format string
* @param ... Format arguments
*/
static void s_plugin_helper_WriteQMMLog(plugin_id plid, int severity, const char* fmt, ...) {
    if (!fmt) {
        QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called WriteQMMLog() with null fmt\n";
        return;
    }

    if (severity < QMM_LOG_TRACE || severity > QMM_LOG_FATAL)
        severity = QMM_LOG_INFO;

    // if log severity is below thresholds, don't log
    if (!log_level_match(severity))
        return;

    // get log tag from plugin
    plugin_info* plinfo = (plugin_info*)plid;
    const char* logtag = plinfo->logtag;
    if (!logtag || !*logtag)
        logtag = plinfo->name;

    va_list	argptr;
    static char buf[1024];

    va_start(argptr, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argptr);
    va_end(argptr);

    // not QMMLOG since we already checked log_level_match()
    LOG(severity, str_toupper(logtag)) << buf;
}


/**
* @brief Construct a string from format string.
*
* @param plid Plugin ID of the calling plugin
* @param fmt FOrmat string
* @param ... Format arguments
* @return Pointer to the constructed string
*/
static char* s_plugin_helper_VarArgs(plugin_id plid [[maybe_unused]], const char* fmt, ...) {
    va_list	argptr;
    static char str[ROTATING_BUFFER_NUM][ROTATING_BUFFER_SIZE];
    static int index = 0;

    // cycle rotating buffer and store string
    index = (index + 1) & ROTATING_BUFFER_MASK;

    va_start(argptr, fmt);
    vsnprintf(str[index], sizeof(str[index]), fmt, argptr);
    va_end(argptr);

    char* ret = str[index];

    // QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called VarArgs(\"" << format << "\") = \"" << ret << "\"\n";
    return ret;
}


/**
* @brief Is the mod a QVM?
*
* @param plid Plugin ID of the calling plugin
* @return 0 if the mod is not a QVM, !0 otherwise
*/
static int s_plugin_helper_IsQVM(plugin_id plid [[maybe_unused]]) {
    int ret = !!g_mod.vm.memory;

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called IsQVM() = " << ret << "\n";

    return ret;
}


/**
* @brief Convert engine message value to string.
*
* @param plid Plugin ID of the calling plugin
* @param msg Engine message to convert
* @return String name of engine message
*/
static const char* s_plugin_helper_EngMsgName(plugin_id plid [[maybe_unused]], intptr_t msg) {
    const char* ret = gameinfo.game->EngMsgName(msg);

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called EngMsgName(" << msg << ") = \"" << ret << "\"\n";

    return ret;
}


/**
* @brief Convert mod message value to string.
*
* @param plid Plugin ID of the calling plugin
* @param msg Mod message to convert
* @return String name of mod message
*/
static const char* s_plugin_helper_ModMsgName(plugin_id plid [[maybe_unused]], intptr_t msg) {
    const char* ret = gameinfo.game->ModMsgName(msg);

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called ModMsgName(" << msg << ") = \"" << ret << "\"\n";

    return ret;
}


/**
* @brief Get integer value of a cvar.
*
* @param plid Plugin ID of the calling plugin
* @param cvar Name of cvar
* @return Integer value of cvar
*/
static intptr_t s_plugin_helper_GetIntCvar(plugin_id plid [[maybe_unused]], const char* cvar) {
    intptr_t ret = 0;
    if (cvar && *cvar)
        ret = ENG_SYSCALL(QMM_ENG_MSG(QMM_G_CVAR_VARIABLE_INTEGER_VALUE), cvar);

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called GetIntCvar(\"" << cvar << "\") = " << ret << "\n";

    return ret;
}


/**
* @brief Get string value of a cvar.
*
* @param plid Plugin ID of the calling plugin
* @param cvar Name of cvar
* @return Pointer to string value of cvar
*/
static const char* s_plugin_helper_GetStrCvar(plugin_id plid [[maybe_unused]], const char* cvar) {
    static char str[ROTATING_BUFFER_NUM][ROTATING_BUFFER_SIZE];
    static int index = 0;

    const char* ret = "";

    if (cvar && *cvar) {
        // cycle rotating buffer and store string
        index = (index + 1) & ROTATING_BUFFER_MASK;
        ENG_SYSCALL(QMM_ENG_MSG(QMM_G_CVAR_VARIABLE_STRING_BUFFER), cvar, str[index], (intptr_t)sizeof(str[index]));
        ret = str[index];
    }

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called GetStrCvar(\"" << cvar << "\") = \"" << ret << "\"\n";

    return ret;
}


/**
* @brief Returns the QMM short code for the active engine.
*
* @param plid Plugin ID of the calling plugin
* @return Pointer to string representing the active game engine
*/
static const char* s_plugin_helper_GetGameEngine(plugin_id plid [[maybe_unused]]) {
    const char* ret = gameinfo.game->GameCode();

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called GetGameEngine() = \"" << ret << "\"\n";

    return ret;
}


/**
* @brief Fill buffer with the desired command argument with G_ARGV.
*
* This function automatically handles both types of G_ARGV: one which fills a buffer and one which returns the value.
*
* @param plid Plugin ID of the calling plugin
* @param argn Argument number to retrieve
* @param buf Buffer to store result in
* @param buflen Length of buf
*/ 
static void s_plugin_helper_Argv(plugin_id plid [[maybe_unused]], intptr_t argn, char* buf, intptr_t buflen) {
    if (buf && buflen)
        ArgV(argn, buf, buflen);

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called Argv(" << argn << ") = \"" << buf << "\"\n";
}


/**
* @brief Same as the SDK's Info_ValueForKey function.
*
* @param plid Plugin ID of the calling plugin
* @param userinfo Userinfo string to search
* @param key Key to find value for
* @return Pointer to string containing value ("" if not found)
*/
static const char* s_plugin_helper_InfoValueForKey(plugin_id plid [[maybe_unused]], const char* userinfo, const char* key) {
    static std::string value[ROTATING_BUFFER_NUM];
    static int index = 0;

    const char* ret = "";

    if (userinfo && key) {
        std::string s = userinfo;

        // userinfo strings are "\key\value\key\value\"
        // so search for "\key\" and then get everything up to the next "\"
        std::string fkey = "\\" + std::string(key) + "\\";
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
            index = (index + 1) & ROTATING_BUFFER_MASK;
            value[index] = fval;
            ret = value[index].c_str();
        }
    }

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called InfoValueForKey(\"" << userinfo << "\", \"" << key << "\") = \"" << ret << "\"\n";

    return ret;
}


/**
* @brief Retrieve a node from the QMM configuration file.
*
* @param key Slash-separated node to find
* @return JSON object representing the node (or an empty node if not found)
*/
static nlohmann::json s_plugin_cfg_get_node(std::string key) {
    if (key[0] == '/')
        key = key.substr(1);

    nlohmann::json node = g_cfg;

    std::filesystem::path keypath = key;
    for (auto& segment : keypath.parent_path()) {
        node = cfg_get_object(node, segment.u8string());
    }

    return node;
}


/**
* @brief Retrieve a string from the QMM configuration file.
*
* @param plid Plugin ID of the calling plugin
* @param key Slash-separated node to find
* @return Pointer to string representing the node (or "" if not found)
*/
static const char* s_plugin_helper_ConfigGetStr(plugin_id plid [[maybe_unused]], const char* key) {
    static std::string value[ROTATING_BUFFER_NUM];
    static int index = 0;

    nlohmann::json node = s_plugin_cfg_get_node(key);

    // cycle rotating buffer and store string
    index = (index + 1) & ROTATING_BUFFER_MASK;
    value[index] = cfg_get_string(node, path_basename(key));
    const char* ret = value[index].c_str();

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called ConfigGetStr(\"" << key << "\") = \"" << ret << "\"\n";

    return ret;
}


/**
* @brief Retrieve an integer from the QMM configuration file.
*
* @param plid Plugin ID of the calling plugin
* @param key Slash-separated node to find
* @return Integer value of node (or -1 if not found)
*/
static int s_plugin_helper_ConfigGetInt(plugin_id plid [[maybe_unused]], const char* key) {
    nlohmann::json node = s_plugin_cfg_get_node(key);
    int ret = cfg_get_int(node, path_basename(key));

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called ConfigGetInt(\"" << key << "\") = " << ret << "\n";

    return ret;
}


/**
* @brief Retrieve a boolean value from the QMM configuration file.
*
* @param plid Plugin ID of the calling plugin
* @param key Slash-separated node to find
* @return Boolean value of node (or false if not found)
*/
static int s_plugin_helper_ConfigGetBool(plugin_id plid [[maybe_unused]], const char* key) {
    nlohmann::json node = s_plugin_cfg_get_node(key);
    int ret = (int)cfg_get_bool(node, path_basename(key));

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called ConfigGetBool(\"" << key << "\") = " << ret << "\n";

    return ret;
}


/**
* @brief Retrieve a list of strings from the QMM configuration file.
*
* @param plid Plugin ID of the calling plugin
* @param key Slash-separated node to find
* @return Pointer to a null-terminated array of strings representing the values of node
*/
static const char** s_plugin_helper_ConfigGetArrayStr(plugin_id plid [[maybe_unused]], const char* key) {
    static std::vector<std::string> value[ROTATING_BUFFER_NUM];
    // plugin API needs to be C-compatible, so this vector stores the .c_str() of each string in the value vector
    static std::vector<const char*> valuep[ROTATING_BUFFER_NUM];
    static int index = 0;

    nlohmann::json node = s_plugin_cfg_get_node(key);

    // cycle rotating buffer and store array
    index = (index + 1) & ROTATING_BUFFER_MASK;
    value[index] = cfg_get_array_str(node, path_basename(key));
    // fill valuep with const char*s from value
    valuep[index].clear();
    for (std::string& s : value[index]) {
        valuep[index].push_back(s.c_str());
    }
    valuep[index].push_back(nullptr);	// null-terminate the array

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called ConfigGetArrayStr(\"" << key << "\") = [" << value[index].size() << " items]\n";

    return valuep[index].data();
}


/**
* @brief Retrieve a list of strings from the QMM configuration file.
*
* @param plid Plugin ID of the calling plugin
* @param key Slash-separated node to find
* @return Pointer to an array of ints representing the values of node (the first index is the number of remaining indexes)
*/
static int* s_plugin_helper_ConfigGetArrayInt(plugin_id plid [[maybe_unused]], const char* key) {
    static std::vector<int> value[ROTATING_BUFFER_NUM];
    static int index = 0;

    nlohmann::json node = s_plugin_cfg_get_node(key);

    // cycle rotating buffer and store array
    index = (index + 1) & ROTATING_BUFFER_MASK;
    value[index] = cfg_get_array_int(node, path_basename(key));
    // insert length of the array as the first element
    value[index].insert(value[index].begin(), (int)value[index].size());

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called ConfigGetArrayInt(\"" << key << "\") = [" << value[index].size() - 1 << " items]\n";

    return value[index].data();
}


/**
* @brief Get a configstring with G_GET_CONFIGSTRING.
* 
* This function automatically handles both types of G_GET_CONFIGSTRING: one which fills a buffer and one which returns
* the value.
*
* @param plid Plugin ID of the calling plugin
* @param index Configstring index to retrieve
* @param buf Buffer to store value in
* @param buflen Length of buf
*/
static void s_plugin_helper_GetConfigString(plugin_id plid [[maybe_unused]], intptr_t index, char* buf, intptr_t buflen) {
    // char* (*getConfigstring)(int index);
    // void trap_GetConfigstring(int num, char* buffer, int bufferSize);
    // some games don't return pointers because of QVM interaction, so if this returns anything but null
    // (or true?), we need to get the configstring from the return value
    // instead
    if (buf && buflen) {
        intptr_t ret = ENG_SYSCALL(QMM_ENG_MSG(QMM_G_GET_CONFIGSTRING), index, buf, buflen);
        if (ret > 1)
            strncpyz(buf, (const char*)ret, (size_t)buflen);
    }

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called GetConfigString(" << index << ") = \"" << buf << "\"\n";
}


/**
* @brief Broadcast a message to each plugin's QMM_PluginMessage() function.
*
* @param plid Plugin ID of the calling plugin
* @param message A pointer to string that is passed to QMM_PluginMessage
* @param buf A generic pointer that is passed to QMM_PluginMessage
* @param buflen Length of buf
* @return Number of plugins that received the message
*/
static int s_plugin_helper_PluginBroadcast(plugin_id plid, const char* message, void* buf, intptr_t buflen) {
    // count how many plugins were called
    int total = 0;
    for (Plugin& p : g_plugins) {
        // skip the calling plugin
        if (p.plugininfo == (plugin_info*)plid)
            continue;
        // skip if the plugin doesn't have the function
        if (!p.QMM_PluginMessage)
            continue;
        p.QMM_PluginMessage(plid, message, buf, buflen, 1); // 1 = is_broadcast
        total++;
    }

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called PluginBroadcast(\"" << message << "\") = " << total << " plugins called\n";

    return total;
}


/**
* @brief Send a message to a specific plugin's QMM_PluginMessage() function.
*
* @param plid Plugin ID of the calling plugin
* @param to_plid Plugin ID to receive the message
* @param message A pointer to string that is passed to QMM_PluginMessage
* @param buf A generic pointer that is passed to QMM_PluginMessage
* @param buflen Length of buf
* @return 0 if unsuccessful, !0 if successful
*/
static int s_plugin_helper_PluginSend(plugin_id plid, plugin_id to_plid, const char* message, void* buf, intptr_t buflen) {
    // don't let a plugin call itself
    if (plid == to_plid)
        return 0;

    for (Plugin& p : g_plugins) {
        // if this is the destination plugin
        if (p.plugininfo == (plugin_info*)to_plid) {
            // if the plugin doesn't have the message function
            if (!p.QMM_PluginMessage)
                return 0;
            p.QMM_PluginMessage(plid, message, buf, buflen, 0); // 0 = is_broadcast

            QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called PluginSend(\"" << message << "\")\n";

            return 1;
        }
    }
    return 0;
}


/**
* @brief Register a new QVM function ID to the calling plugin
*
* @param plid Plugin ID of the calling plugin
* @return QVM function ID (0 if unsuccessful)
*/
static int s_plugin_helper_QVMRegisterFunc(plugin_id plid) {
    int ret = 0;

    // find the calling plugin
    for (Plugin& p : g_plugins) {
        // found it
        if (p.plugininfo == (plugin_info*)plid) {
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

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called QVMRegisterFunc() = " << ret << "\n";

    return ret;
}


/**
* @brief Execute a given QVM function
*
* @param plid Plugin ID of the calling plugin
* @param instruction QVM function ID to execute
* @param argc Number of arguments
* @param argv Arguments to pass to function
* @return Return value of QVM function
*/
static int s_plugin_helper_QVMExecFunc(plugin_id plid [[maybe_unused]], int instruction, int argc, int* argv) {
    int ret = qvm_exec_ex(&g_mod.vm, (size_t)instruction, argc, argv);

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called QVMExecFunc(" << instruction << ", " << argc << ") = " << ret << "\n";

    return ret;
}


/**
* @brief Fill buffer with the desired command argument with G_ARGV.
*
* This function automatically handles both types of G_ARGV: one which fills a buffer and one which returns the value.
*
* @param plid Plugin ID of the calling plugin
* @param argn Argument number to retrieve
* @return Pointer to string with command argument
*/
static const char* s_plugin_helper_Argv2(plugin_id plid [[maybe_unused]], intptr_t argn) {
    static char str[ROTATING_BUFFER_NUM][ROTATING_BUFFER_SIZE];
    static int index = 0;

    // cycle rotating buffer and store string
    index = (index + 1) & ROTATING_BUFFER_MASK;

    ArgV(argn, str[index], sizeof(str[index]));

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called Argv2(" << argn << ") = \"" << str[index] << "\"\n";

    return str[index];
}


/**
* @brief Get a configstring with G_GET_CONFIGSTRING.
*
* This function automatically handles both types of G_GET_CONFIGSTRING: one which fills a buffer and one which returns
* the value.
*
* @param plid Plugin ID of the calling plugin
* @param configindex Configstring index to retrieve
* @return Pointer to string with configstring
*/
static const char* s_plugin_helper_GetConfigString2(plugin_id plid [[maybe_unused]], intptr_t configindex) {
    static char str[ROTATING_BUFFER_NUM][ROTATING_BUFFER_SIZE];
    static int index = 0;

    // cycle rotating buffer and store string
    index = (index + 1) & ROTATING_BUFFER_MASK;

    // char* (*getConfigstring)(int index);
    // void trap_GetConfigstring(int num, char* buffer, int bufferSize);
    // some games don't return pointers because of QVM interaction, so if this returns anything but null
    // (or true?), we probably are in an api game, and need to get the configstring from the return value
    // instead
    intptr_t ret = ENG_SYSCALL(QMM_ENG_MSG(QMM_G_GET_CONFIGSTRING), configindex, str[index], sizeof(str[index]));
    if (ret > 1)
        strncpyz(str[index], (const char*)ret, sizeof(str[index]));

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called GetConfigString2(" << configindex << ") = \"" << str[index] << "\"\n";

    return str[index];
}


/**
* @brief Get the mod directory.
*
* @param plid Plugin ID of the calling plugin
* @return Pointer to string with mod directory
*/
static const char* s_plugin_helper_ModDir(plugin_id plid [[maybe_unused]]) {
    const char* ret = gameinfo.mod_dir.c_str();

    QMMLOG(QMM_LOG_TRACE, "QMM") << "Plugin \"" << ((plugin_info*)plid)->name << " called ModDir() = \"" << ret << "\"\n";

    return ret;
}
