/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdarg.h>
#include "format.h"
#include "log.h"	 // brings in windows.h -_-
#include "config.h"
#include "main.h"
#include "game_api.h"
#include "qmmapi.h"
#include "plugin.h"
#include "mod.h"
#include "util.h"
#include "version.h"

game_info_t g_gameinfo;

/* About overall control flow:
   syscall (mod->engine) call flow for QVM mods only:
   1. mod calls <GAME>_vmsyscall function
   2. pointer arguments are converted (if not NULL, the QVM data segment base address is added)
   3. continue with next section as if it were a DLL mod
   
   syscall (mod->engine) call flow for DLL mods:
   1. call is handled by syscall()
   2. call is passed to plugins' QMM_syscall functions
   3. call is passed to engine syscall (unless at least 1 plugin uses the result QMM_SUPERCEDE)
   4. call is passed to plugins' QMM_syscall_Post functions
   5. engine syscall return value (or a value given by last plugin which uses result QMM_SUPERCEDE or QMM_OVERRIDE)
      is returned to mod
   
   vmMain (engine->mod) call flow:
   1. call is handled by vmMain()
   2. call is passed to plugins' QMM_vmMain functions
   3. call is passed to mod vmMain (unless at least 1 plugin uses the result QMM_SUPERCEDE)
   4. call is passed to plugins' QMM_vmMain_Post functions
   5. mod vmMain return value (or a value given by last plugin which uses result QMM_SUPERCEDE or QMM_OVERRIDE)
      is returned to engine
*/

/* About syscall/mod constants:
   Because some engine syscall and mod entry point constants might change between games, we store arrays of all the ones
   QMM uses internally in each game's support file (game_XYZ.cpp). When the game is determined (automatically or via config),
   that game's arrays are accessed with the QMM_ENG_MSG and QMM_MOD_MSG macros and they are indexed with a QMM_ constant,
   like: QMM_G_PRINT, QMM_GAME_CONSOLE_COMMAND, etc
*/


/* Entry point: engine->qmm
   This is the first function called when the DLL is loaded. The address of the engine's syscall callback is given,
   but it is not guaranteed to be initialized and ready for calling until vmMain() is called later. For now, all
   we can do is store the syscall, load the config file, and attempt to figure out what game engine we are in.
   This is either determined by the config file, or by getting the filename of the QMM DLL itself.
*/
C_DLLEXPORT void dllEntry(eng_syscall_t syscall) {
	main_detect_env();

	log_init(fmt::format("{}/qmm2.log", g_gameinfo.qmm_dir));

	LOG(INFO, "dllEntry") << "QMM v" QMM_VERSION " (" QMM_OS ") loaded!\n";
	LOG(INFO, "dllEntry") << fmt::format("QMM path: \"{}\"\n", g_gameinfo.qmm_path);
	LOG(INFO, "dllEntry") << fmt::format("Engine path: \"{}\"\n", g_gameinfo.exe_path);
	LOG(INFO, "dllEntry") << fmt::format("Mod directory (?): \"{}\"\n", g_gameinfo.moddir);

	//todo figure out logging standard and if i want to use logging for all the normal G_PRINTs once we're out of dllEntry

	// ???
	if (!syscall) {
		LOG(FATAL, "QMM") << "dllEntry(): syscall is NULL!\n";
		return;
	}

	// save syscall pointer
	g_gameinfo.pfnsyscall = syscall;

	main_load_config();

	main_detect_game();

	// failed to get engine information
	if (!g_gameinfo.game) {
		LOG(FATAL, "QMM") << fmt::format("dllEntry(): Unable to determine game engine using \"{}\"\n", cfg_get_string(g_cfg, "game", "auto"));
		return;
	}
}

#ifdef QMM_MOHAA_SUPPORT
/* Entry point: engine->qmm
   MOHAA: This is the first function called when the DLL is loaded. MOHAA uses a system closer to HalfLife, where a
   struct of function pointers is given from the engine to the mod, and the mod returns a struct of function pointers
   back to the engine.
   To best integrate this with QMM, game_mohaa.cpp/.h create an enum for each import (syscall) and export (vmMain) function/variable.
   A game_export_t is given to the engine which has lambdas for each pointer that calls QMM's vmMain(enum, arg0, ...).
   A game_import_t is given to the mod which has lambdas for each pointer that calls QMM's syscall(enum, arg0, ...).

   The original import/export tables are stored. When QMM and plugins need to call the mod or engine, g_mod.pfnvmMain or
   g_gameinfo.pfnsyscall point to functions which will take the cmd, and call the proper function pointer out of the struct.

   General flow:
   * engine->GetGameAPI(import):
     - load config file
     - detect game engine
   * GetGameAPI->MOHAAGetGameAPI(import):
     - copy import struct
	 - assign variables from import to qmm_import
	 - assign default values for variables in qmm_export
	 - return &qmm_export
   * GetGameAPI:
     - return &qmm_export to engine

   * engine:
     - stores qmm_export pointer, checks qmm_export->apiversion to match GAME_API_VERSION
	 - called qmm_export->Init
	 - this eventually hits our vmMain(GAME_INIT) and we load the mod
	 - store variables from mod's real export struct into our qmm_export
*/
C_DLLEXPORT void* GetGameAPI(void* import) {
	log_init("qmm2.log");

	main_detect_env();

	LOG(INFO, "init") << "[QMM] QMM v" QMM_VERSION " (" QMM_OS ") loaded!\n";
	LOG(INFO, "init") << fmt::format("[QMM] QMM path: \"{}\"\n", g_gameinfo.qmm_path);
	LOG(INFO, "init") << fmt::format("[QMM] Engine path: \"{}\"\n", g_gameinfo.exe_path);
	LOG(INFO, "init") << fmt::format("[QMM] Mod directory (?): \"{}\"\n", g_gameinfo.moddir);

	// ???
	if (!import) {
		fmt::print("[QMM] ERROR: GetGameAPI(): import is NULL!\n");
		return nullptr;
	}

	main_load_config();

	// main_detect_game();
	// for now we'll just assume it's MOHAA because it's the only supported GetGameAPI game
	g_gameinfo.game = &g_supportedgames[6];
	g_gameinfo.isautodetected = false;

	// we won't need these since we know what the game engine is at this point, but fix these anyway
	QMM_FAIL_G_ERROR = 4;
	QMM_FAIL_GAME_SHUTDOWN = 2;

	// failed to get engine information
	if (!g_gameinfo.game) {
		fmt::print("[QMM] ERROR: dllEntry(): Unable to determine game engine using \"{}\"\n", cfg_get_string(g_cfg, "game", "auto"));
		return nullptr;
	}

	// call the game-specific GetGameAPI function (e.g. MOHAA_GetGameAPI) which will set up the exports
	// for returning here back to the game engine, as well as save the imports in preparation of loading
	// the mod
	return g_gameinfo.game->apientry(import);
}
#endif // QMM_MOHAA_SUPPORT

void main_detect_env() {
	// save exe module path
	g_gameinfo.exe_path = path_get_modulepath(nullptr);
	g_gameinfo.exe_dir = path_dirname(g_gameinfo.exe_path);
	g_gameinfo.exe_file = path_basename(g_gameinfo.exe_path);

	// save qmm module path
	g_gameinfo.qmm_path = path_get_modulepath(dllEntry);
	g_gameinfo.qmm_dir = path_dirname(g_gameinfo.qmm_path);
	g_gameinfo.qmm_file = path_basename(g_gameinfo.qmm_path);

	// since we don't have the mod directory yet (can only officially get it using engine functions), we can
	// attempt to get the mod directory from the qmm path. this will be used only for config loading
	g_gameinfo.moddir = path_basename(g_gameinfo.qmm_dir);
}

// general code to load config file. called from dllEntry() and GetGameAPI()
void main_load_config() {
	// load config file, try the following locations in order:
	// "<qmmdir>/qmm2.json"
	// "<exedir>/<moddir>/qmm2.json"
	// "./<moddir>/qmm2.json"
	std::string try_paths[] = {
		fmt::format("{}/qmm2.json", g_gameinfo.qmm_dir),
		fmt::format("{}/{}/qmm2.json", g_gameinfo.exe_dir, g_gameinfo.moddir),
		fmt::format("./{}/qmm2.json", g_gameinfo.moddir)
	};
	for (auto& try_path : try_paths) {
		g_cfg = cfg_load(try_path);
		if (!g_cfg.empty()) {
			g_gameinfo.cfg_path = try_path;
			fmt::print("[QMM] Config file found! Path: \"{}\"\n", g_gameinfo.cfg_path);
			break;
		}
	}
	if (g_cfg.empty() || g_cfg.is_discarded()) {
		// a default constructed json object is a blank {}, so in case of load failure, we can still try to read from it and assume defaults
		fmt::print("[QMM] WARNING: Unable to load config file, all settings will use default values\n");
	}
}

void main_detect_game() {
	std::string cfg_game = cfg_get_string(g_cfg, "game", "auto");

	// find what game we are loaded in
	for (int i = 0; g_supportedgames[i].dllname; i++) {
		supportedgame_t& game = g_supportedgames[i];
		// if short name matches config option, we found it!
		if (str_striequal(cfg_game, game.gamename_short)) {
			fmt::print("[QMM] Found match for config option \"{}\"\n", cfg_game);
			g_gameinfo.game = &game;
			g_gameinfo.isautodetected = false;
			break;
		}
		// otherwise, if auto, we need to check matching dll names, with optional exe hint
		if (str_striequal(cfg_game, "auto")) {
			// dll name matches
			if (str_striequal(g_gameinfo.qmm_file, game.dllname)) {
				fmt::print("[QMM] Found match for dll name \"{}\" - {}\n", game.dllname, game.gamename_short);
				// if no hint, or hint exists and matches
				if (!game.exe_hint || (game.exe_hint && str_stristr(g_gameinfo.exe_file, game.exe_hint))) {
					fmt::print("[QMM] Found match for exe hint name \"{}\"\n", game.exe_hint ? game.exe_hint : "null");
					g_gameinfo.game = &game;
					g_gameinfo.isautodetected = true;
					break;
				}
			}
		}
	}
}

/* Entry point: engine->qmm
   This is the "vmMain" function called by the engine as an entry point into the mod. First thing, we check if the game info is not stored.
   This means that the engine could not be determined, so we fail with G_ERROR and tell the user to set the game in the config file.
   If the engine was determined, it performs some internal tasks on a few events, and then routes the function call according to the "overall control flow" comment above.

   The internal events we track:
   GAME_INIT (pre): load mod file, load plugins, and optionally execute a cfg file
   GAME_CONSOLE_COMMAND (pre): handle "qmm" server command
   GAME_CLIENT_COMMAND (pre): if "qmm_nocrash" cvar is set to 1, block client commands that are too long (msgboom bug)
   GAME_CLIENT_CONNECT (post): if "nogreeting" config option is not set to "true", post a simple QMM greeting message to clients
*/
C_DLLEXPORT int vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	// couldn't load engine info, so we will just call syscall(G_ERROR) to exit
	if (!g_gameinfo.game) {
		// calling G_ERROR triggers a vmMain(GAME_SHUTDOWN) call, so don't send G_ERROR in GAME_SHUTDOWN or it'll just recurse		
		if (cmd != QMM_FAIL_GAME_SHUTDOWN)
			ENG_SYSCALL(QMM_FAIL_G_ERROR, "\n\n=========\nCritical QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n=========\n");
		return 0;
	}

	if (cmd == QMM_MOD_MSG[QMM_GAME_INIT]) {
		LOG(INFO, "QMM") << "test\n";
		// get mod dir from engine
		g_gameinfo.moddir = util_get_str_cvar("fs_game");
		// RTCWSP returns "" for the mod, and others may too? grab the default mod dir from game info instead
		if (g_gameinfo.moddir.empty()) {
			g_gameinfo.moddir = g_gameinfo.game->moddir;
		}

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] QMM v" QMM_VERSION " (" QMM_OS ")\n");
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ModDir: {}\n", g_gameinfo.moddir).c_str());
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Config file: \"{}\" {}\n", g_gameinfo.cfg_path, g_cfg.is_discarded() ? "(error)": "").c_str());

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] URL: " QMM_URL "\n");

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Registering CVARs\n");

		// make version cvar
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_REGISTER], NULL, "qmm_version", QMM_VERSION, QMM_ENG_MSG[QMM_CVAR_ROM] | QMM_ENG_MSG[QMM_CVAR_SERVERINFO]);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_SET], "qmm_version", QMM_VERSION);

		// make game cvar
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_REGISTER], NULL, "qmm_game", g_gameinfo.game->gamename_short, QMM_ENG_MSG[QMM_CVAR_ROM]);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_SET], "qmm_game", g_gameinfo.game->gamename_short);

		// make nocrash cvar
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_REGISTER], NULL, "qmm_nocrash", "1", QMM_ENG_MSG[QMM_CVAR_ARCHIVE]);
		// don't set this, in case it is set in autoexec.cfg

		// load mod
		std::string cfg_mod = cfg_get_string(g_cfg, "mod", "auto");
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Attempting to find mod using \"{}\"\n", cfg_mod).c_str());
		// if config setting is an absolute path, just attempt to load it directly
		if (!path_is_relative(cfg_mod)) {
			if (!mod_load(&g_mod, cfg_mod)) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], fmt::format("[QMM] FATAL ERROR: Unable to load mod \"{}\"\n", cfg_mod).c_str());
				return 0;
			}
		}
		// if config setting is "auto", try the following locations in order:
		// "<qvmname>" (if the game engine supports it)
		// "<qmmdir>/qmm_<dllname>"
		// "<exedir>/<moddir>/qmm_<dllname>"
		// "<exedir>/<moddir>/<dllname>" (as long as this isn't same as qmm path)
		// "./<moddir>/qmm_<dllname>"
		else if (str_striequal(cfg_mod, "auto")) {
			std::string try_paths[] = {
				g_gameinfo.game->qvmname ? g_gameinfo.game->qvmname : "",	// (only if game engine supports it)
				fmt::format("{}/qmm_{}", g_gameinfo.qmm_dir, g_gameinfo.game->dllname),
				fmt::format("{}/{}/qmm_{}", g_gameinfo.exe_dir, g_gameinfo.moddir, g_gameinfo.game->dllname),
				fmt::format("{}/{}/{}", g_gameinfo.exe_dir, g_gameinfo.moddir, g_gameinfo.game->dllname),
				fmt::format("./{}/qmm_{}", g_gameinfo.moddir, g_gameinfo.game->dllname)
			};
			// try paths
			for (auto& try_path : try_paths) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Attempting to auto-load mod \"{}\"\n", try_path).c_str());
				// if this matches qmm's path, skip it
				if (str_striequal(try_path, g_gameinfo.qmm_path))
					continue;
				if (mod_load(&g_mod, try_path)) {
					break;
				}
			}
		}
		// if config setting is a relative path, try the following locations in order:
		// "<mod>"
		// "<qmmdir>/<mod>"
		// "<exedir>/<moddir>/<mod>"
		// "./<moddir>/<mod>"
		else {
			std::string try_paths[] = {
				cfg_mod,
				fmt::format("{}/{}", g_gameinfo.qmm_dir, cfg_mod),
				fmt::format("{}/{}/{}", g_gameinfo.exe_dir, g_gameinfo.moddir, cfg_mod),
				fmt::format("./{}/{}", g_gameinfo.moddir, cfg_mod)
			};
			// try paths
			for (auto& try_path : try_paths) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Attempting to load mod \"{}\"\n", try_path).c_str());
				// if this matches qmm's path, skip it
				if (str_striequal(try_path, g_gameinfo.qmm_path))
					continue;
				if (mod_load(&g_mod, try_path)) {
					break;
				}
			}
		}

		if (!mod_is_loaded(&g_mod)) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], fmt::format( "[QMM] FATAL ERROR: Unable to load mod using \"{}\"\n", cfg_mod).c_str());
			return 0;
		}
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Successfully loaded {} mod \"{}\"\n", g_mod.vmbase ? "VM" : "DLL", g_mod.path).c_str());

		// load plugins
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Attempting to load plugins\n");
		std::vector<std::string> plugin_paths = cfg_get_array(g_cfg, "plugins");
		for (auto plugin_path : plugin_paths) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Attempting to load plugin \"{}\"\n", plugin_path).c_str());
			plugin_t p;
			// absolute path, just attempt to load it directly
			if (!path_is_relative(plugin_path)) {
				if (plugin_load(&p, plugin_path))
					g_plugins.push_back(p);
				continue;
			}
			// relative path, try the following locations in order:
			// "<qmmdir>/<plugin>"
			// "<exedir>/<moddir>/<plugin>"
			// "./<moddir>/<plugin>"
			std::string try_paths[] = {
				fmt::format("{}/{}", g_gameinfo.qmm_dir, plugin_path),
				fmt::format("{}/{}/{}", g_gameinfo.exe_dir, g_gameinfo.moddir, plugin_path),
				fmt::format("./{}/{}", g_gameinfo.moddir, plugin_path)
			};
			for (auto& try_path : try_paths) {
				if (plugin_load(&p, try_path)) {
					g_plugins.push_back(p);
					break;
				}
			}
		}
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Successfully loaded {} plugin(s)\n", g_plugins.size()).c_str());

		// exec the qmmexec cfg
		std::string cfg_execcfg = cfg_get_string(g_cfg, "execcfg", "qmmexec.cfg");
		if (!cfg_execcfg.empty()) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Executing config file \"{}\"\n", cfg_execcfg).c_str());
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_SEND_CONSOLE_COMMAND], QMM_ENG_MSG[QMM_EXEC_APPEND], fmt::format("exec {}\n", cfg_execcfg).c_str());
		}

		// we're done!
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Startup successful!\n");
	}

	else if (cmd == QMM_MOD_MSG[QMM_GAME_CONSOLE_COMMAND]) {
		char arg0[5], arg1[8], arg2[10];
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], 0, arg0, sizeof(arg0));
		arg0[sizeof(arg0) - 1] = '\0';
		int argc = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGC]);

		if (str_striequal("qmm", arg0)) {
			if (argc > 1)
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], 1, arg1, sizeof(arg1));
			if (argc > 2)
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], 2, arg2, sizeof(arg2));
			arg1[sizeof(arg1) - 1] = '\0';
			arg2[sizeof(arg2) - 1] = '\0';
			if (argc == 1) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Usage: qmm <command> [params]\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Available commands:\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] qmm status - displays information about QMM\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] qmm list - displays information about loaded QMM plugins\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] qmm info <id> - outputs info on plugin with id\n");
			} else if (str_striequal("status", arg1)) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] QMM v" QMM_VERSION " (" QMM_OS ") loaded\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ModDir: {}\n", g_gameinfo.moddir).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Config file: \"{}\" {}\n", g_gameinfo.cfg_path, g_cfg.is_discarded() ? " (error)" : "").c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] URL: " QMM_URL "\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Plugin interface: {}:{}\n", QMM_PIFV_MAJOR, QMM_PIFV_MINOR).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] NoCrash: {}\n", util_get_int_cvar("qmm_nocrash") ? "on" : "off").c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Loaded mod file: {}\n", g_mod.path).c_str());
				if (g_mod.vmbase) {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] QVM byteswapped: {}\n", g_mod.qvm.swapped ? "yes" : "no").c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] QVM file size: {}\n", g_mod.qvm.filesize).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] QVM op count: {}\n", g_mod.qvm.header.numops).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] QVM memory offset: {}\n", (void*)g_mod.qvm.memory).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] QVM memory size: {}\n", g_mod.qvm.memorysize).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] QVM codeseg size: {}\n", g_mod.qvm.codeseglen).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] QVM dataseg size: {}\n", g_mod.qvm.dataseglen).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] QVM stack size: {}\n", g_mod.qvm.stackseglen).c_str());
				}
				else {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Mod vmMain() offset: {}\n", (void*)g_mod.pfnvmMain).c_str());
				}
			} else if (str_striequal("list", arg1)) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] id - plugin [version]\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] ---------------------\n");
				for (plugin_t& p : g_plugins) {
					int num = 1;
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] {:>2} - {} [{}] ({}) - {}\n", num, p.plugininfo->name, p.plugininfo->version).c_str());
					++num;
				}
			} else if (str_striequal("info", arg1)) {
				if (argc == 2) {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] qmm info <id> - outputs info on plugin with id\n");
					return 1;
				}
				unsigned int pid = atoi(arg2);
				if (pid > 0 && pid <= g_plugins.size()) {
					plugin_t& p = g_plugins[pid-1];
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Plugin info for #{}:\n", arg2).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Name: {}\n", p.plugininfo->name).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Version: {}\n", p.plugininfo->version).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] URL: {}\n", p.plugininfo->url).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Author: {}\n", p.plugininfo->author).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Desc: {}\n", p.plugininfo->desc).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Interface version: {}:{}\n", p.plugininfo->pifv_major, p.plugininfo->pifv_minor).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Path: {}\n", p.path).c_str());
				}
				else {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Unable to find plugin #{}\n", arg2).c_str());
				}
			}

			return 1;
		}
	}

	else if (cmd == QMM_MOD_MSG[QMM_GAME_CLIENT_COMMAND]) {
		if (util_get_int_cvar("qmm_nocrash")) {
			// pull all args and count total length of command (including spaces between args)
			int argc = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGC]);
			int len = 0;
			static char bigbuf[1024];
			for (int i = 0; i < argc; ++i) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], i, bigbuf, sizeof(bigbuf));
				bigbuf[sizeof(bigbuf) - 1] = '\0';
				len += (strlen(bigbuf) + 1);	// 1 for space
				if (len > 900)
					break;
			}
			--len;	// get rid of the last space added
			if (len >= 900) {
				std::string s = fmt::format("[QMM] NoCrash: Userid {} has attempted to execute a command longer than 900 chars\n", arg0);
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], s.c_str());
				log_write(s.c_str());
				return 1;
			}
		}
	}

	// store max result
	pluginres_t maxresult = QMM_UNUSED;
	// store return value to pass back to the engine (either real vmMain return value, or value given by a QMM_OVERRIDE/QMM_SUPERCEDE plugin)
	int final_ret = 0;
	// temp int for return values
	int ret = 0;
	// begin passing calls to plugins' QMM_vmMain functions
	for (plugin_t& p : g_plugins) {
		g_plugin_result = QMM_UNUSED;
		// call plugin's vmMain and store return value
		ret = p.QMM_vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
		// set new max result
		maxresult = util_max(g_plugin_result, maxresult);
		if (g_plugin_result == QMM_UNUSED)
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] WARNING: vmMain({}): Plugin \"{}\" did not set result flag\n", g_gameinfo.game->eng_msg_names(cmd), p.plugininfo->name).c_str());
		if (g_plugin_result == QMM_ERROR)
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: vmMain({}): Plugin \"{}\" resulted in ERROR\n", g_gameinfo.game->eng_msg_names(cmd), p.plugininfo->name).c_str());
		// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
		if (g_plugin_result >= QMM_OVERRIDE)
			final_ret = ret;
	}
	// call real vmMain function (unless a plugin resulted in QMM_SUPERCEDE)
	if (maxresult < QMM_SUPERCEDE) {
		ret = g_mod.pfnvmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
		// the return value for GAME_CLIENT_CONNECT is a char* so we have to modify the pointer value for VMs
		if (cmd == QMM_MOD_MSG[QMM_GAME_CLIENT_CONNECT] && g_mod.vmbase && ret /* don't bother if ret is NULL */) {
			ret += g_mod.vmbase;
		}
	}
	// if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual mod's return value back to the engine
	if (maxresult < QMM_OVERRIDE) 
		final_ret = ret;
	// pass calls to plugins' QMM_vmMain_Post functions (ignore return values and results)
	for (plugin_t& p : g_plugins) {
		p.QMM_vmMain_Post(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
	}

	// if user is connecting for the first time, user is not a bot, and "nogreeting" option is not set
	if (cmd == QMM_MOD_MSG[QMM_GAME_CLIENT_CONNECT] && arg1 && !arg2) {
		if (!cfg_get_bool(g_cfg, "nogreeting", false)) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_SEND_SERVER_COMMAND], arg0, "print \"^7This server is running ^4QMM^7 v^4" QMM_VERSION "^7\n\"");
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_SEND_SERVER_COMMAND], arg0, "print \"^7URL: ^4" QMM_URL "^7\n\"");
		}
	}
	// handle shut down (this is after the mod and plugins get called with GAME_SHUTDOWN)
	else if (cmd == QMM_MOD_MSG[QMM_GAME_SHUTDOWN]) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Shutting down mod\n");
		mod_unload(&g_mod);

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Shutting down plugins\n");
		for (plugin_t& p : g_plugins) {
			//unload each plugin (call QMM_Detach, and then dlclose)
			plugin_unload(&p);
		}
		g_plugins.clear();

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Finished shutting down\n");
	}

	return final_ret;
}

/* Entry point: mod->qmm
   This is the "syscall" function called by the mod as a way to pass info to or get info from the engine.
   It performs some internal tasks on a few events, and then routes the function call according to the "overall control flow" comment above.

   The internal events we track:
   G_FS_FCLOSE_FILE (pre): watch for the mod closing the log file handle
   G_SEND_SERVER_COMMAND (pre): if "qmm_nocrash" cvar is set to 1, block client commands that are too long (msgboom bug)
   G_FS_FOPEN_FILE (post): watch for the mod opening a handle for the log file (based on g_log cvar), and save it for logging
*/
int syscall(int cmd, ...) {
	va_list arglist;
	int args[13] = {};	// JK2 decided to mess it all up and have a single cmd with 13 args
	va_start(arglist, cmd);
	for (int i = 0; i < (sizeof(args)/sizeof(args[0])); ++i)
		args[i] = va_arg(arglist, int);
	va_end(arglist);

	// if this is a call to close a file, check the handle to see if it matches our existing log handle
	if (cmd == QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE]) {
		if (args[0] == log_get()) {
			// we have it, output final line and clear log file handle
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Detected close operation on g_log file handle, unhooking...\n");
			log_write("[QMM] Detected close operation on g_log file handle, unhooking...\n\n");
			log_set(-1);
		}
	}

	// integrated nocrash protection
	// vsay fix
	else if (cmd == QMM_ENG_MSG[QMM_G_SEND_SERVER_COMMAND]) {
		if (util_get_int_cvar("qmm_nocrash") && args[1] && strlen((char*)args[1]) >= 1022) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] NoCrash: A user has attempted to use the vsay exploit");
			log_write("[QMM] NoCrash: A user has attempted to use the vsay exploit", -1);
			return 1;
		}
	}

	// store max result
	pluginres_t maxresult = QMM_UNUSED;
	// store return value to pass back to the mod (either real syscall return value, or value given by a QMM_OVERRIDE/QMM_SUPERCEDE plugin)
	int final_ret = 0;
	// temp int for return values
	int ret = 0;
	// begin passing calls to plugins' QMM_syscall functions
	for (plugin_t& p : g_plugins) {
		g_plugin_result = QMM_UNUSED;
		// call plugin's syscall and store return value
		ret = p.QMM_syscall(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12]);
		// set new max result
		maxresult = util_max(g_plugin_result, maxresult);
		if (g_plugin_result == QMM_UNUSED)
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] WARNING: syscall({}): Plugin \"{}\" did not set result flag\n", g_gameinfo.game->mod_msg_names(cmd), p.plugininfo->name).c_str());
		if (g_plugin_result == QMM_ERROR)
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] ERROR: syscall({}): Plugin \"{}\" resulted in ERROR\n", g_gameinfo.game->mod_msg_names(cmd), p.plugininfo->name).c_str());
		// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
		if (g_plugin_result >= QMM_OVERRIDE)
			final_ret = ret;
	}
	// call real syscall function (unless a plugin resulted in QMM_SUPERCEDE)
	if (maxresult < QMM_SUPERCEDE)
		ret = g_gameinfo.pfnsyscall(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12]);
	// if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual engine's return value back to the mod
	if (maxresult < QMM_OVERRIDE)
		final_ret = ret;
	// pass calls to plugins' QMM_syscall_Post functions (ignore return values and results)
	for (plugin_t& p : g_plugins) {
		p.QMM_syscall_Post(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12]);
	}

	// if this is a call to open a file for APPEND or APPEND_SYNC
	if (cmd == QMM_ENG_MSG[QMM_G_FS_FOPEN_FILE] && args[1]) {
		if (args[2] == QMM_ENG_MSG[QMM_FS_APPEND] || args[2] == QMM_ENG_MSG[QMM_FS_APPEND_SYNC]) {
			// compare filename against g_log cvar
			if (str_striequal(util_get_str_cvar("g_log"), (char*)(args[0]))) {
				// we have it, save log file handle
				log_set(*(int*)(args[1]));
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Successfully hooked g_log file\n");
				log_write("[QMM] Successfully hooked g_log file\n");
				log_write("[QMM] QMM v" QMM_VERSION " (" QMM_OS ") loaded\n");
				log_write(fmt::format("[QMM] Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
				log_write(fmt::format("[QMM] Mod: {}\n", g_gameinfo.moddir).c_str());
				log_write("[QMM] Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
				log_write("[QMM] URL: " QMM_URL "\n");
			}
		}
	}

	return ret;
}
