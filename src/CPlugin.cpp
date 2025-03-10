/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include <string>
#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/format.h"
#include "CPlugin.h"
#include "qmm.h"
#include "main.h"
#include "util.h"

CPlugin::CPlugin() {
	this->QMM_Query = NULL;
	this->QMM_Attach = NULL;
	this->QMM_Detach = NULL;
	this->QMM_vmMain = NULL;
	this->QMM_vmMain_Post = NULL;
	this->QMM_syscall = NULL;
	this->QMM_syscall_Post = NULL;
	this->plugininfo = NULL;
	this->result = QMM_UNUSED;
}

CPlugin::~CPlugin() {
	if (this->QMM_Detach)
		(this->QMM_Detach)();

	//unload DLL
	this->dll.Unload();
}


//load the given file, and call the QMM_Query function
// - file is the path relative to the mod directory
int CPlugin::LoadQuery(std::string file) {
	if (file.empty())
		return 0;

	//load DLL
	int x = this->dll.Load(fmt::format("{}/{}", g_GameInfo.qmm_dir, file));
	if (!x) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): DLL load failed for plugin: {}\n", file, dlerror()).c_str());
		return 0;
	}

	//find QMM_Query() or fail
	if ((this->QMM_Query = (plugin_query)this->dll.GetProc("QMM_Query")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Unable to find \"QMM_Query\" function in plugin\n", file).c_str());
		return 0;
	}

	//call QMM_Query() func to get the plugininfo
	(this->QMM_Query)(&(this->plugininfo));

	if (!this->plugininfo) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Plugininfo NULL for plugin", file).c_str());
		return 0;
	}

	//check for plugin interface versions

	//if the plugin's major version is higher, don't load and suggest to upgrade QMM
	if (this->plugininfo->pifv_major > QMM_PIFV_MAJOR) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Plugin's major interface version ({}) is greater than QMM's ({}), suggest upgrading QMM.\n", file, this->plugininfo->pifv_major, QMM_PIFV_MAJOR).c_str());
		return 0;
	}
	//if the plugin's major version is lower, don't load and suggest to upgrade plugin
	if (this->plugininfo->pifv_major < QMM_PIFV_MAJOR) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Plugin's major interface version ({}) is less than QMM's ({}), suggest upgrading plugin.\n", file, this->plugininfo->pifv_major, QMM_PIFV_MAJOR).c_str());
		return 0;
	}
	//if the plugin's minor version is higher, don't load and suggest to upgrade QMM
	if (this->plugininfo->pifv_minor > QMM_PIFV_MINOR) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Plugin's minor interface version ({}) is greater than QMM's ({}), suggest upgrading QMM.\n", file, this->plugininfo->pifv_minor, QMM_PIFV_MINOR).c_str());
		return 0;
	}
	//if the plugin's minor version is lower, load, but suggest to upgrade plugin anyway
	if (this->plugininfo->pifv_minor < QMM_PIFV_MINOR)
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] WARNING: CPlugin::LoadQuery(\"{}\"): Plugin's minor interface version ({}) is less than QMM's ({}), suggest upgrading plugin.\n", file, this->plugininfo->pifv_minor, QMM_PIFV_MINOR).c_str());

	//find remaining neccesary functions or fail
	if ((this->QMM_Attach = (plugin_attach)this->dll.GetProc("QMM_Attach")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Unable to find \"QMM_Attach\" function in plugin\n", file).c_str());
		return 0;
	}
	if ((this->QMM_Detach = (plugin_detach)this->dll.GetProc("QMM_Detach")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Unable to find \"QMM_Detach\" function in plugin\n", file).c_str());
		return 0;
	}
	if ((this->QMM_vmMain = (plugin_vmmain)this->dll.GetProc("QMM_vmMain")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Unable to find \"QMM_vmMain\" function in plugin\n", file).c_str());
		return 0;
	}
	if ((this->QMM_syscall = (plugin_syscall)this->dll.GetProc("QMM_syscall")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Unable to find \"QMM_syscall\" function in plugin\n", file).c_str());
		return 0;
	}
	if ((this->QMM_vmMain_Post = (plugin_vmmain)this->dll.GetProc("QMM_vmMain_Post")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Unable to find \"QMM_vmMain_Post\" function in plugin\n", file).c_str());
		return 0;
	}
	if ((this->QMM_syscall_Post = (plugin_syscall)this->dll.GetProc("QMM_syscall_Post")) == NULL) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: CPlugin::LoadQuery(\"{}\"): Unable to find \"QMM_syscall_Post\" function in plugin\n", file).c_str());
		return 0;
	}

	return 1;
}

//call plugin's QMM_Attach() function, pass real engine syscall, mod vmMain, pluginfuncs, and vmbase
int CPlugin::Attach(eng_syscall_t eng_syscall, mod_vmMain_t mod_vmMain, pluginfuncs_t* pluginfuncs, int vmbase) {
	//call QMM_Attach() func with the engine syscall, mod vmMain, result ptr, pluginfuncs, and vmbase
	return (this->QMM_Attach)(eng_syscall, mod_vmMain, &this->result, pluginfuncs, vmbase);
}

plugin_vmmain CPlugin::vmMain() {
	return this->QMM_vmMain;
}

plugin_vmmain CPlugin::vmMain_Post() {
	return this->QMM_vmMain_Post;
}

plugin_syscall CPlugin::syscall() {
	return this->QMM_syscall;
}

plugin_syscall CPlugin::syscall_Post() {
	return this->QMM_syscall_Post;
}

const plugininfo_t* CPlugin::PluginInfo() {
	return this->plugininfo;
}

pluginres_t CPlugin::Result() {
	return this->result;
}

void CPlugin::ResetResult() {
	this->result = QMM_UNUSED;
}
