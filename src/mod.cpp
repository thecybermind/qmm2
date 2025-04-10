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

mod_t g_mod;

static intptr_t s_mod_vmmain(intptr_t cmd, ...);
static bool s_mod_load_qvm(mod_t* mod);
static bool s_mod_load_getgameapi(mod_t* mod);
static bool s_mod_load_vmmain(mod_t* mod);

bool mod_load(mod_t* mod, std::string file) {
	if (!mod || mod->dll || mod->qvm.memory)
		return false;

	mod->path = file;

	std::string ext = path_baseext(file);

	// only allow qvm mods if the game engine supports it
	if (str_striequal(ext, EXT_QVM) && g_gameinfo.game->vmsyscall)
		return s_mod_load_qvm(mod);
	
	// if DLL
	else if (str_striequal(ext, EXT_DLL)) {
		// load DLL
		if (!(mod->dll = dlopen(file.c_str(), RTLD_NOW))) {
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): DLL load failed: {}\n", file, dlerror());
			return false;
		}

		// if this DLL is the same as QMM, cancel
		if ((void*)mod->dll == g_gameinfo.qmm_module_ptr) {
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

void mod_unload(mod_t* mod) {
	if (mod->qvm.memory)
		qvm_unload(&mod->qvm);
	if (mod->dll)
		dlclose(mod->dll);
	*mod = mod_t();
}

// entry point to store in mod_t->pfnvmMain for qvm mods
static intptr_t s_mod_vmmain(intptr_t cmd, ...) {
	// if qvm isn't loaded, we need to error
	if (!g_mod.qvm.memory) {
		// G_ERROR triggers a vmMain(GAME_SHUTDOWN) call, so don't do this if the message is GAME_SHUTDOWN as that will just recurse
		if (cmd != QMM_GAME_SHUTDOWN) {
			LOG(QMM_LOG_FATAL, "QMM") << fmt::format("s_mod_vmmain({}): QVM unloaded due to a run-time error\n", cmd);
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
	return qvm_exec(&g_mod.qvm, QMM_MAX_VMMAIN_ARGS + 1, qvmargs);
}

// load a QVM mod
static bool s_mod_load_qvm(mod_t* mod) {
	// load file using engine functions to read into pk3s if necessary
	int fpk3;
	int filelen = (int)ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FOPEN_FILE], mod->path.c_str(), &fpk3, QMM_ENG_MSG[QMM_FS_READ]);
	if (filelen <= 0) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Could not open QVM for reading\n", mod->path);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);
		mod_unload(mod);
		return false;
	}
	byte* filemem = (byte*)qvm_malloc(filelen);
	if (!filemem) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load({}): Unable to allocate memory for QVM file: {} bytes\n", mod->path, filelen);
		mod_unload(mod);
		return false;
	}
	ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_READ], filemem, filelen, fpk3);
	ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);

	// load stack size from config
	int stacksize = cfg_get_int(g_cfg, "stacksize", 1);

	// attempt to load mod
	bool loaded = qvm_load(&mod->qvm, filemem, filelen, g_gameinfo.game->vmsyscall, stacksize);
	qvm_free(filemem); // free regardless of qvm_load success
	if (!loaded) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): QVM load failed\n", mod->path);
		mod_unload(mod);
		return false;
	}

	mod->pfnvmMain = s_mod_vmmain;
	mod->vmbase = (intptr_t)mod->qvm.datasegment;

	return true;
}

// load a GetGameAPI DLL mod
static bool s_mod_load_getgameapi(mod_t* mod) {
	mod_GetGameAPI_t GetGameAPI = nullptr;

	// look for GetGameAPI function
	if (!(GetGameAPI = (mod_GetGameAPI_t)dlsym(mod->dll, "GetGameAPI"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"GetGameAPI\" function\n", mod->path);
		mod_unload(mod);
		return false;
	}

	// pass the QMM-hooked import pointers to the mod
	// get the original export pointers from the mod
	g_gameinfo.api_info.orig_export = GetGameAPI(g_gameinfo.api_info.qmm_import);

	// handle unlikely case of export being null
	if (!g_gameinfo.api_info.orig_export) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): \"GetGameAPI\" function returned null\n", mod->path);
		mod_unload(mod);
		return false;
	}

	// this is a pointer to a wrapper vmMain function that calls actual mod func from orig_export
	mod->pfnvmMain = g_gameinfo.api_info.orig_vmmain;
	mod->vmbase = 0;

	return true;
}

// load a vmMain DLL mod
static bool s_mod_load_vmmain(mod_t* mod) {
	mod_dllEntry_t dllEntry = nullptr;

	// look for dllEntry function
	if (!(dllEntry = (mod_dllEntry_t)dlsym(mod->dll, "dllEntry"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"dllEntry\" function\n", mod->path);
		mod_unload(mod);
		return false;
	}

	// look for vmMain function
	if (!(mod->pfnvmMain = (mod_vmMain_t)dlsym(mod->dll, "vmMain"))) {
		LOG(QMM_LOG_ERROR, "QMM") << fmt::format("mod_load(\"{}\"): Unable to find \"vmMain\" function\n", mod->path);
		mod_unload(mod);
		return false;
	}

	// pass syscall to mod dllEntry function
	dllEntry(g_gameinfo.pfnsyscall);
	mod->vmbase = 0;

	return true;
}
