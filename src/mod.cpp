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
#include "game_api.h"
#include "main.h"
#include "config.h"
#include "mod.h"
#include "qvm.h"
#include "util.h"

mod_t g_mod;

// entry point to store in mod_t->pfnvmMain for qvm mods
static int s_mod_vmmain(int cmd, ...) {
	// if qvm isn't loaded (and this isn't GAME_SHUTDOWN), we need to error
	if (!g_mod.qvm.memory && cmd != QMM_GAME_SHUTDOWN) {
		LOG(FATAL, "QMM") << fmt::format("s_mod_vmmain({}): QVM unloaded due to a run-time error\n", cmd);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], "\n\n=========\nFatal QMM Error:\nThe QVM was unloaded due to a run-time error.\n=========\n");
		return 0;
	}

	QMM_GET_VMMAIN_ARGS();

	// generate new array to also include cmd at the front
	int qvmargs[QMM_MAX_VMMAIN_ARGS + 1] = { cmd };
	memcpy(&qvmargs[1], args, sizeof(args));

	// pass array and size to qvm
	return qvm_exec(&g_mod.qvm, qvmargs, QMM_MAX_VMMAIN_ARGS + 1);
}

bool mod_load(mod_t* mod, std::string file) {
	if (!mod || mod->dll || mod->qvm.memory)
		return false;

	mod->path = file;

	std::string ext = path_baseext(file);

	// only allow qvm mods if the game engine supports it
	if (str_striequal(ext, EXT_QVM) && g_gameinfo.game->vmsyscall) {
		// load file using engine functions to read into pk3s if necessary
		int fpk3;
		int filelen = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FOPEN_FILE], file.c_str(), &fpk3, QMM_ENG_MSG[QMM_FS_READ]);
		if (filelen <= 0) {
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Could not open QVM for reading\n", file);
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);
			return false;
		}
		byte* filemem = (byte*)qvm_malloc(filelen);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_READ], filemem, filelen, fpk3);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);

		// load stack size from config
		int stacksize = cfg_get_int(g_cfg, "stacksize", 1);
		
		// attempt to load mod
		if (!qvm_load(&mod->qvm, filemem, filelen, g_gameinfo.game->vmsyscall, stacksize)) {
			qvm_unload(&mod->qvm);
			qvm_free(filemem);
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): QVM load failed\n", file);
			return false;
		}
		mod->pfnvmMain = s_mod_vmmain;
		mod->vmbase = (int)mod->qvm.datasegment;
		qvm_free(filemem);

		return true;
	}
#ifdef QMM_GETGAMEAPI_SUPPORT
	// only allow GetGameAPI-style mod if the game engine supports it
	else if (str_striequal(ext, EXT_DLL) && g_gameinfo.game->apientry) {
		if (!(mod->dll = dlopen(file.c_str(), RTLD_NOW))) {
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): GetGameAPI-style DLL load failed: {}\n", file, dlerror());
			return false;
		}

		mod_GetGameAPI_t GetGameAPI = nullptr;

		// if this DLL is the same as QMM, cancel
		if ((void*)mod->dll == g_gameinfo.qmm_module_ptr) {
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): GetGameAPI-style DLL is actually QMM?\n", file);
			goto api_dll_fail;
		}

		// look for GetGameAPI function
		if (!(GetGameAPI = (mod_GetGameAPI_t)dlsym(mod->dll, "GetGameAPI"))) {
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"GetGameAPI\" function\n", file);
			goto api_dll_fail;
		}

		// pass the QMM-hooked import pointers to the mod
		// get the original export pointers from the mod
		g_gameinfo.api_info.orig_export = GetGameAPI(g_gameinfo.api_info.qmm_import);

		// this is a pointer to a wrapper vmMain function that calls actual mod func from orig_export
		mod->pfnvmMain = g_gameinfo.api_info.orig_vmmain;
		mod->vmbase = 0;

		return true;

		api_dll_fail:
		if (mod->dll)
			dlclose(mod->dll);
		return false;

	}
#endif // QMM_GETGAMEAPI_SUPPORT
	// load DLL
	else if (str_striequal(ext, EXT_DLL)) {
		if (!(mod->dll = dlopen(file.c_str(), RTLD_NOW))) {
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL load failed: {}\n", file, dlerror());
			return false;
		}

		mod_dllEntry_t dllEntry = nullptr;

		// if this DLL is the same as QMM, cancel
		if ((void*)mod->dll == g_gameinfo.qmm_module_ptr) {
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL is actually QMM?\n", file);
			goto dll_fail;
		}

		// look for dllEntry function
		if (!(dllEntry = (mod_dllEntry_t)dlsym(mod->dll, "dllEntry"))) {
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"dllEntry\" function\n", file);
			goto dll_fail;
		}

		// look for vmMain function
		if (!(mod->pfnvmMain = (mod_vmMain_t)dlsym(mod->dll, "vmMain"))) {
			LOG(ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"vmMain\" function\n", file);
			goto dll_fail;
		}

		dllEntry(g_gameinfo.pfnsyscall);
		mod->vmbase = 0;

		return true;

		dll_fail:
		if (mod->dll)
			dlclose(mod->dll);
		return false;
	}

	return false;
}

void mod_unload(mod_t* mod) {
	if (mod->qvm.memory) {
		qvm_unload(&mod->qvm);
	}
	else {
		dlclose(mod->dll);
	}
	*mod = mod_t();
}
