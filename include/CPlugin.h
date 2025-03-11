/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_CPLUGIN_H__
#define __QMM2_CPLUGIN_H__

#include <string>
#include "CDLL.h"
#include "qmmapi.h"

class CPlugin {
	public:
		CPlugin();
		~CPlugin();

		int LoadQuery(std::string);
		int Attach(eng_syscall_t, mod_vmMain_t, pluginfuncs_t*, int);

		plugin_vmmain vmMain();
		plugin_vmmain vmMain_Post();
		plugin_syscall syscall();
		plugin_syscall syscall_Post();

		const plugininfo_t* PluginInfo();

		pluginres_t Result();
		void ResetResult();

	private:
		CDLL dll;
		plugin_query QMM_Query;
		plugin_attach QMM_Attach;
		plugin_detach QMM_Detach;
		plugin_vmmain QMM_vmMain;
		plugin_vmmain QMM_vmMain_Post;
		plugin_syscall QMM_syscall;
		plugin_syscall QMM_syscall_Post;
		plugininfo_t* plugininfo;
		pluginres_t result;
};
#endif // __QMM2_CPLUGIN_H__
