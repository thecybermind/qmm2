/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include <vector>
#include <string>
#include "config.h"
#include "CPluginMgr.h"
#include "CPlugin.h"
#include "main.h"
#include "plugin.h"
#include "util.h"

CPluginMgr::CPluginMgr() {
}

CPluginMgr::~CPluginMgr() {
}

int CPluginMgr::LoadPlugins() {
	std::vector<std::string> plugin_files = cfg_get_array(g_cfg, "plugins");
	
	for (auto plugin : plugin_files) {
		this->LoadPlugin(plugin.c_str());
	}
	
	return this->plugins.size();
}

// this is just a non-MFP stub to pass to plugins
static int s_plugin_vmmain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	return MOD_VMMAIN(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
}

// file is the path relative to the mod directory (mod dir and homepath are added in CPlugin::LoadQuery)
int CPluginMgr::LoadPlugin(const char* file) {
	if (!file || !*file)
		return 0;

	CPlugin p;
	
	if (!p.LoadQuery(file)) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] ERROR: CPluginMgr::LoadPlugin(\"%s\"): Unable to load plugin due to previous errors\n", file));
		return 0;
	}
	ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] CPluginMgr::LoadPlugin(\"%s\"): Successfully queried plugin \"%s\"\n", file, p.PluginInfo()->name));

	if (!p.Attach(g_gameinfo.pfnsyscall, s_plugin_vmmain, get_pluginfuncs(), g_ModMgr->Mod()->GetBase())) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] CPluginMgr::LoadPlugin(\"%s\"): QMM_Attach() returned 0 for plugin \"%s\"\n", file, p.PluginInfo()->name));
		return 0;
	}
	ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] CPluginMgr::LoadPlugin(\"%s\"): Successfully attached plugin \"%s\"\n", file, p.PluginInfo()->name));

	this->plugins.push_back(std::move(p));
	return 1;
}

void CPluginMgr::ListPlugins() {
	ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] id - plugin\n");
	ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] ------------------------------------------------------------------------\n");
	int num = 0;
	for (CPlugin& p : this->plugins) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] %.2d - %s (%s)\n", num, p.PluginInfo()->name, p.PluginInfo()->version));
		++num;
	}
}

const plugininfo_t* CPluginMgr::PluginInfo(int num) {
	if (num < 0)
		return NULL;
	size_t unum = num;
	if (unum >= this->plugins.size())
		return NULL;
	return this->plugins[num].PluginInfo();
}

int CPluginMgr::CallvmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	if (!g_ModMgr->Mod())
		return 0;

	// store max result
	pluginres_t maxresult = QMM_UNUSED;

	// store return value to use for the vmMain call (for QMM_OVERRIDE)
	int final_ret = 0;

	// temp int for return value for each plugin
	int ret = 0;
	for (CPlugin& p : this->plugins) {
		// call plugin's vmMain and store return value
		ret = (p.vmMain())(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);

		// check plugin's result flag
		switch (p.Result()) {
			// unchanged: show warning
			case QMM_UNUSED:
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] WARNING: CPluginMgr::CallvmMain(%s): Plugin \"%s\" did not set result flag\n", ENG_MSGNAME(cmd), p.PluginInfo()->name));
				break;

			// error: show error
			case QMM_ERROR:
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] ERROR: CPluginMgr::CallvmMain(%s): Plugin \"%s\" resulted in ERROR\n", ENG_MSGNAME(cmd), p.PluginInfo()->name));
				break;
			
			// override: set maxresult and set final_ret to this return value (fall-through)
			case QMM_OVERRIDE:

			// supercede: set maxresult and set final_ret to this return value (fall-through)
			case QMM_SUPERCEDE:
				final_ret = ret;

			// ignored: set maxresult
			case QMM_IGNORED:
				// set new result if applicable
				if (maxresult < p.Result())
					maxresult = p.Result();
				break;

			default:
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] ERROR: CPluginMgr::CallvmMain(%s): Plugin \"%s\" set unknown result flag \"%d\"\n", ENG_MSGNAME(cmd), p.PluginInfo()->name, p.Result()));
				break;
		}

		// reset the plugin result
		p.ResetResult();
	}

	// call vmmain function based on maxresult rules (then call plugin's post func)
	switch(maxresult) {
		// run mod's vmMain and store result
		case QMM_UNUSED:
		case QMM_ERROR:
		case QMM_IGNORED:
		case QMM_OVERRIDE:
			ret = MOD_VMMAIN(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
			// the return value for GAME_CLIENT_CONNECT is a char* so we have to modify the pointer value for VMs
			if (cmd == QMM_MOD_MSG[QMM_GAME_CLIENT_CONNECT] && g_ModMgr->Mod()->IsVM() && ret /* dont bother if its NULL */)
				ret += g_ModMgr->Mod()->GetBase();

			if (maxresult != QMM_OVERRIDE)
				final_ret = ret;
		case QMM_SUPERCEDE:
		    for (CPlugin& p : this->plugins) {
				// call plugin's post func
				(p.vmMain_Post())(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
				if (p.Result() == QMM_ERROR)
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] ERROR: CPluginMgr::CallvmMain(%s): Plugin \"%s\" resulted in ERROR\n", ENG_MSGNAME(cmd), p.PluginInfo()->name));
				
				// ignore result flag, reset to UNUSED
				p.ResetResult();
			}

			return final_ret;
		// no idea, but just act like nothing happened *shifty eyes*
		default:
			return MOD_VMMAIN(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
	}
}

int CPluginMgr::Callsyscall(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11, int arg12) {
	// store max result
	pluginres_t maxresult = QMM_UNUSED;

	// store return value to use for the vmMain call (for QMM_OVERRIDE)
	int final_ret = 0;

	// temp int for return value for each plugin
	int ret = 0;
	for (CPlugin& p : this->plugins) {

		// call plugin's vmMain and store return value
		ret = (p.syscall())(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);

		// check plugin's result flag
		switch (p.Result()) {
			// unchanged: show warning
			case QMM_UNUSED:
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] WARNING: CPluginMgr::Callsyscall(%s): Plugin \"%s\" did not set result flag\n", ENG_MSGNAME(cmd), p.PluginInfo()->name));
				break;

			// error: show error
			case QMM_ERROR:
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] ERROR: CPluginMgr::Callsyscall(%s): Plugin \"%s\" resulted in ERROR\n", ENG_MSGNAME(cmd), p.PluginInfo()->name));
				break;
			
			// override: set maxresult and set final_ret to this return value (fall-through)
			case QMM_OVERRIDE:

			// supercede: set maxresult and set final_ret to this return value (fall-through)
			case QMM_SUPERCEDE:
				final_ret = ret;

			// ignored: set maxresult
			case QMM_IGNORED:
				// set new result if applicable
				if (maxresult < p.Result())
					maxresult = p.Result();
				break;

			default:
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] ERROR: CPluginMgr::Callsyscall(%s): Plugin \"%s\" set unknown result flag \"%d\"\n", ENG_MSGNAME(cmd), p.PluginInfo()->name, p.Result()));
				break;
		}

		// reset the plugin result
		p.ResetResult();
	}

	// call vmmain function based on maxresult rules (then call plugin's post func)
	switch(maxresult) {
		// run mod's vmMain and store result
		case QMM_UNUSED:
		case QMM_ERROR:
		case QMM_IGNORED:
		case QMM_OVERRIDE:
			ret = ENG_SYSCALL(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
			if (maxresult != QMM_OVERRIDE)
				final_ret = ret;
		case QMM_SUPERCEDE:
			for (CPlugin& p : this->plugins) {
				// call plugin's post func
				(p.syscall_Post())(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
				if (p.Result() == QMM_ERROR)
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] ERROR: CPluginMgr::Callsyscall(%s): Plugin \"%s\" resulted in ERROR\n", ENG_MSGNAME(cmd), p.PluginInfo()->name));
				
				// ignore result flag, reset to UNUSED
				p.ResetResult();
			}
			
			return final_ret;
		// no idea, but just act like nothing happened *shifty eyes*
		default:
			return ENG_SYSCALL(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
	}
}

CPluginMgr* CPluginMgr::GetInstance() {
	if (!CPluginMgr::instance)
		CPluginMgr::instance = new CPluginMgr;

	return CPluginMgr::instance;
}

CPluginMgr* CPluginMgr::instance = NULL;
