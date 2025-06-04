/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include "log.h"
#include "osdef.h"
#include "format.h"
#include "game_api.h"
#include "main.h"
#include "config.h"
#include "mod.h"
#include "qvm.h"
#include "util.h"

namespace mod {
	mod_t g_mod;

	static intptr_t s_mod_vmmain(intptr_t cmd, ...);
	static bool s_mod_load_qvm(mod_t& mod);
	static bool s_mod_load_vmmain(mod_t& mod);
	static bool s_mod_load_getgameapi(mod_t& mod);


	bool mod_load(mod_t& mod, std::string file) {
		// if this mod_t somehow already has a dll or qvm pointer, wipe it first
		if (mod.dll || !mod.qvm.memory.empty())
			mod_unload(mod);

		mod.path = file;

		std::string ext = util::path_baseext(file);

		// only allow qvm mods if the game engine supports it
		if (util::striequal(ext, EXT_QVM) && g_gameinfo.game->vmsyscall)
			return s_mod_load_qvm(mod);

		// if DLL
		else if (util::striequal(ext, EXT_DLL)) {
			// load DLL
			if (!(mod.dll = dlopen(file.c_str(), RTLD_NOW))) {
				LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL load failed: {}\n", file, dlerror());
				return false;
			}

			// if this DLL is the same as QMM, cancel
			if ((void*)mod.dll == g_gameinfo.qmm_module_ptr) {
				LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL is actually QMM?\n", file);
				mod_unload(mod);
				return false;
			}

			// pass off to engine-specific loading function
			if (g_gameinfo.game->apientry)
				return s_mod_load_getgameapi(mod);
			else
				return s_mod_load_vmmain(mod);
		}

		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unknown file format\n", file);
		return false;
	}


	void mod_unload(mod_t& mod) {
		qvm::qvm_unload(mod.qvm);
		if (mod.dll)
			dlclose(mod.dll);
		mod = mod_t();
		g_gameinfo.api_info.orig_export = nullptr;
	}


	// entry point to store in mod_t->pfnvmMain for qvm mods
	static intptr_t s_mod_vmmain(intptr_t cmd, ...) {
		// if qvm isn't loaded, we need to error
		if (g_mod.qvm.memory.empty()) {
			if (!g_shutdown) {
				g_shutdown = true;
				LOG(QMM_LOG_FATAL, "QMM") << fmt::format("s_mod_vmmain({}): QVM unloaded due to a run-time error\n", g_gameinfo.game->mod_msg_names(cmd));
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], "\n\n=========\nFatal QMM Error:\nThe QVM was unloaded due to a run-time error.\n=========\n");
			}
			return 0;
		}

		QMM_GET_VMMAIN_ARGS();

		// generate new int array from the intptr_t args, and also include cmd at the front
		int qvmargs[QMM_MAX_VMMAIN_ARGS + 1] = { (int)cmd };
		for (int i = 0; i < QMM_MAX_VMMAIN_ARGS; i++) {
			qvmargs[i + 1] = (int)args[i];
		}

		// pass array and size to qvm
		return qvm::qvm_exec(g_mod.qvm, QMM_MAX_VMMAIN_ARGS + 1, qvmargs);
	}


	// load a QVM mod
	static bool s_mod_load_qvm(mod_t& mod) {
		int fpk3;
		int filelen;
		std::vector<std::byte> filemem;
		int stacksize;
		bool verify_data;
		bool loaded;

		// load file using engine functions to read into pk3s if necessary
		filelen = (int)ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FOPEN_FILE], mod.path.c_str(), &fpk3, QMM_ENG_MSG[QMM_FS_READ]);
		if (filelen <= 0) {
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Could not open QVM for reading\n", mod.path);
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);
			goto fail;
		}
		filemem.resize(filelen);

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_READ], filemem.data(), filelen, fpk3);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);

		// load stack size from config
		stacksize = cfg_get_int(g_cfg, "stacksize", 1);

		// get data verification setting from config
		verify_data = cfg_get_bool(g_cfg, "qvmverifydata", true);

		// disable data verification for SOF2MP
		if (!strcmp(g_gameinfo.game->gamename_short, "SOF2MP"))
			verify_data = false;

		// attempt to load mod
		loaded = qvm::qvm_load(mod.qvm, filemem, g_gameinfo.game->vmsyscall, stacksize, verify_data);
		if (!loaded) {
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): QVM load failed\n", mod.path);
			goto fail;
		}

		mod.pfnvmMain = s_mod_vmmain;
		mod.vmbase = (intptr_t)mod.qvm.datasegment;

		return true;

	fail:
		mod_unload(mod);
		return false;
	}


	// load a GetGameAPI DLL mod
	static bool s_mod_load_getgameapi(mod_t& mod) {
		// look for GetGameAPI function
		mod_GetGameAPI_t GetGameAPI = (mod_GetGameAPI_t)dlsym(mod.dll, "GetGameAPI");

		if (!GetGameAPI) {
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"GetGameAPI\" function\n", mod.path);
			goto fail;
		}

		// pass the QMM-hooked import pointers to the mod
		// get the original export pointers from the mod
		g_gameinfo.api_info.orig_export = GetGameAPI(g_gameinfo.api_info.qmm_import);

		// handle unlikely case of export being null
		if (!g_gameinfo.api_info.orig_export) {
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): \"GetGameAPI\" function returned null\n", mod.path);
			goto fail;
		}

		// this is a pointer to a wrapper vmMain function that calls actual mod func from orig_export
		mod.pfnvmMain = g_gameinfo.api_info.orig_vmmain;
		mod.vmbase = 0;

		return true;

	fail:
		mod_unload(mod);
		return false;
	}


	// load a vmMain DLL mod
	static bool s_mod_load_vmmain(mod_t& mod) {
		mod_dllEntry_t dllEntry = nullptr;

		// look for dllEntry function
		if (!(dllEntry = (mod_dllEntry_t)dlsym(mod.dll, "dllEntry"))) {
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"dllEntry\" function\n", mod.path);
			goto fail;
		}

		// look for vmMain function
		if (!(mod.pfnvmMain = (mod_vmMain_t)dlsym(mod.dll, "vmMain"))) {
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"vmMain\" function\n", mod.path);
			goto fail;
		}

		// pass QMM's syscall to mod dllEntry function
		dllEntry(syscall);
		mod.vmbase = 0;

		return true;

	fail:
		mod_unload(mod);
		return false;
	}
}
