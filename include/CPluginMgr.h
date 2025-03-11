/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_CPLUGINMGR_H__
#define __QMM2_CPLUGINMGR_H__

#include <vector>
#include "CPlugin.h"

class CPluginMgr {
	public:
		CPluginMgr();
		~CPluginMgr();

		int LoadPlugins();

		int LoadPlugin(const char*);

		void ListPlugins();
		const plugininfo_t* PluginInfo(int);

		int CallvmMain(int, int, int, int, int, int, int, int, int, int, int, int, int);
		int Callsyscall(int, int, int, int, int, int, int, int, int, int, int, int, int, int);

		static CPluginMgr* GetInstance();

	private:
		std::vector<CPlugin> plugins;

		static CPluginMgr* instance;
};
#endif // __QMM2_CPLUGINMGR_H__
