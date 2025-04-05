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
#include "osdef.h"
#include "format.h"
#include "log.h"
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
   that game-specific arrays are accessed with the QMM_ENG_MSG and QMM_MOD_MSG macros and they are indexed with a QMM_
   constant, like: QMM_G_PRINT, QMM_GAME_CONSOLE_COMMAND, etc.
*/


/* Entry point: engine->qmm
   This is the first function called when a vmMain DLL is loaded. The address of the engine's syscall callback is given,
   but it is not guaranteed to be initialized and ready for calling until vmMain() is called later. For now, all
   we can do is store the syscall, load the config file, and attempt to figure out what game engine we are in.
   This is either determined by the config file, or by getting the filename of the QMM DLL itself.
*/
C_DLLEXPORT void dllEntry(eng_syscall_t syscall) {
	main_detect_env();

	log_init(fmt::format("{}/qmm2.log", g_gameinfo.qmm_dir));

	LOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " SUF_DLL ") (dllEntry) loaded!\n";
	LOG(QMM_LOG_INFO, "QMM") << fmt::format("QMM path: \"{}\"\n", g_gameinfo.qmm_path);
	LOG(QMM_LOG_INFO, "QMM") << fmt::format("Engine path: \"{}\"\n", g_gameinfo.exe_path);
	LOG(QMM_LOG_INFO, "QMM") << fmt::format("Mod directory (?): \"{}\"\n", g_gameinfo.moddir);

	// ???
	if (!syscall) {
		LOG(QMM_LOG_FATAL, "QMM") << "dllEntry(): syscall is NULL!\n";
		return;
	}

	// save syscall pointer
	g_gameinfo.pfnsyscall = syscall;

	main_load_config();

	// update log severity for file output
	std::string cfg_loglevel = cfg_get_string(g_cfg, "loglevel", "");
	if (!cfg_loglevel.empty())
		log_set_severity(log_severity_from_name(cfg_loglevel));

	std::string cfg_game = cfg_get_string(g_cfg, "game", "auto");
	main_detect_game(cfg_game, QMM_DETECT_DLLENTRY);

	// failed to get engine information
	if (!g_gameinfo.game) {
		LOG(QMM_LOG_FATAL, "QMM") << fmt::format("dllEntry(): Unable to determine game engine using \"{}\"\n", cfg_game);
		return;
	}
}

/* Entry point: engine->qmm
   This is the first function called when a GetGameAPI DLL is loaded. This system uses a system closer to HalfLife, where a
   struct of function pointers is given from the engine to the mod, and the mod returns a struct of function pointers
   back to the engine.
   To best integrate this with QMM, game_xyz.cpp/.h create an enum for each import (syscall) and export (vmMain) function/variable.
   A game_export_t is given to the engine which has lambdas for each pointer that calls QMM's vmMain(enum, ...).
   A game_import_t is given to the mod which has lambdas for each pointer that calls QMM's syscall(enum, ...).

   The original import/export tables are stored. When QMM and plugins need to call the mod or engine, g_mod.pfnvmMain or
   g_gameinfo.pfnsyscall point to functions which will take the cmd, and call the proper function pointer out of the struct.

   General flow:
   * engine->GetGameAPI(import):
     - load config file
     - detect game engine
   * GetGameAPI->XYZ_GetGameAPI(import):
     - copy import struct
	 - assign variables from import to qmm_import
	 - assign default values for variables in qmm_export
	 - return &qmm_export
   * GetGameAPI:
     - return &qmm_export to engine

   * engine:
     - stores qmm_export pointer, checks qmm_export->apiversion to match GAME_API_VERSION
	 - calls qmm_export->Init
	 - this eventually hits our vmMain(GAME_INIT) and we load the mod
	 - store variables from mod's real export struct into our qmm_export before returning out of mod
*/
C_DLLEXPORT void* GetGameAPI(void* import) {
	main_detect_env();

	log_init(fmt::format("{}/qmm2.log", g_gameinfo.qmm_dir));

	LOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " SUF_DLL ") (GetGameAPI) loaded!\n";
	LOG(QMM_LOG_INFO, "QMM") << fmt::format("QMM path: \"{}\"\n", g_gameinfo.qmm_path);
	LOG(QMM_LOG_INFO, "QMM") << fmt::format("Engine path: \"{}\"\n", g_gameinfo.exe_path);
	LOG(QMM_LOG_INFO, "QMM") << fmt::format("Mod directory (?): \"{}\"\n", g_gameinfo.moddir);

	// ???
	if (!import) {
		LOG(QMM_LOG_FATAL, "QMM") << "GetGameAPI(): import is NULL!\n";
		return nullptr;
	}

	main_load_config();

	// update log severity for file output
	std::string cfg_loglevel = cfg_get_string(g_cfg, "loglevel", "");
	if (!cfg_loglevel.empty())
		log_set_severity(log_severity_from_name(cfg_loglevel));

	std::string cfg_game = cfg_get_string(g_cfg, "game", "auto");
	main_detect_game(cfg_game, QMM_DETECT_GETGAMEAPI);

	// failed to get engine information
	// by returning nullptr, the engine will error out and so we don't have to worry about it ourselves in GAME_INIT
	if (!g_gameinfo.game) {
		LOG(QMM_LOG_FATAL, "QMM") << fmt::format("GetGameAPI(): Unable to determine game engine using \"{}\"\n", cfg_game);
		return nullptr;
	}

	// call the game-specific GetGameAPI function (e.g. MOHAA_GetGameAPI) which will set up the exports
	// for returning here back to the game engine, as well as save the imports in preparation of loading
	// the mod
	return g_gameinfo.game->apientry(import);
}

// general code to get path/module/binary/etc information
void main_detect_env() {
	// save exe module path
	g_gameinfo.exe_path = path_get_modulepath(nullptr);
	g_gameinfo.exe_dir = path_dirname(g_gameinfo.exe_path);
	g_gameinfo.exe_file = path_basename(g_gameinfo.exe_path);

	// save qmm module path
	g_gameinfo.qmm_path = path_get_modulepath(&g_gameinfo);
	g_gameinfo.qmm_dir = path_dirname(g_gameinfo.qmm_path);
	g_gameinfo.qmm_file = path_basename(g_gameinfo.qmm_path);

	// save qmm module pointer
	g_gameinfo.qmm_module_ptr = path_get_modulehandle(&g_gameinfo);

	// since we don't have the mod directory yet (can only officially get it using engine functions), we can
	// attempt to get the mod directory from the qmm path. if the qmm dir is the same as the exe dir, it's
	// likely that this is a singleplayer game, so just set the temporary moddir to ".".
	// this doesn't have to be exact, since it will only be used only for config loading.
	if (str_striequal(g_gameinfo.qmm_dir, g_gameinfo.exe_dir)) {
		g_gameinfo.moddir = ".";
	}
	else {
		g_gameinfo.moddir = path_basename(g_gameinfo.qmm_dir);
	}
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
			LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Config file found! Path: \"{}\"\n", g_gameinfo.cfg_path);
			break;
		}
	}
	if (g_cfg.empty() || g_cfg.is_discarded()) {
		// a default constructed json object is a blank {}, so in case of load failure, we can still try to read from it and assume defaults
		LOG(QMM_LOG_WARNING, "QMM") << "Unable to load config file, all settings will use default values\n";
	}
}

// general code to auto-detect what game engine loaded us
void main_detect_game(std::string cfg_game, bool is_GetGameAPI_mode) {
	// find what game we are loaded in
	for (int i = 0; g_supportedgames[i].dllname; i++) {
		supportedgame_t& game = g_supportedgames[i];
		// only check games with apientry based on GetGameAPI_mode
		if (is_GetGameAPI_mode != !!game.apientry)
			continue;

		// if short name matches config option, we found it!
		if (str_striequal(cfg_game, game.gamename_short)) {
			LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Found game match for config option \"{}\"\n", cfg_game);
			g_gameinfo.game = &game;
			g_gameinfo.isautodetected = false;
			return;
		}
		// otherwise, if auto, we need to check matching dll names, with optional exe hint
		if (str_striequal(cfg_game, "auto")) {
			// dll name matches
			std::string full_dll_name = fmt::format("{}{}", game.dllname, SUF_DLL "." EXT_DLL);
			if (str_striequal(g_gameinfo.qmm_file, full_dll_name)) {
				LOG(QMM_LOG_INFO, "QMM") << fmt::format("Found game match for dll name \"{}\" - {}\n", full_dll_name, game.gamename_short);
				// if no hint array exists, assume we match
				if (!game.exe_hints.size()) {
					LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("No exe hint for game, assuming match\n");
					g_gameinfo.game = &game;
					g_gameinfo.isautodetected = true;
					return;
				}
				// if a hint array exists, check each for an exe file match
				if (game.exe_hints.size()) {
					for (auto hint : game.exe_hints) {
						if (str_stristr(g_gameinfo.exe_file, hint)) {
							LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Found game match for exe hint name \"{}\"\n", hint);
							g_gameinfo.game = &game;
							g_gameinfo.isautodetected = true;
							return;
						}
					}
				}
			}
		}
	}
}

// general code to find a mod file to load
bool main_load_mod(std::string cfg_mod) {
	LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to find mod using \"{}\"\n", cfg_mod);
	// if config setting is an absolute path, just attempt to load it directly
	if (!path_is_relative(cfg_mod)) {
		if (!mod_load(&g_mod, cfg_mod))
			return false;
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
			fmt::format("{}/qmm_{}{}", g_gameinfo.qmm_dir, g_gameinfo.game->dllname, SUF_DLL "." EXT_DLL),
			fmt::format("{}/{}/qmm_{}{}", g_gameinfo.exe_dir, g_gameinfo.moddir, g_gameinfo.game->dllname, SUF_DLL "." EXT_DLL),
			fmt::format("{}/{}/{}{}", g_gameinfo.exe_dir, g_gameinfo.moddir, g_gameinfo.game->dllname, SUF_DLL "." EXT_DLL),
			fmt::format("./{}/qmm_{}{}", g_gameinfo.moddir, g_gameinfo.game->dllname, SUF_DLL "." EXT_DLL)
		};
		// try paths
		for (auto& try_path : try_paths) {
			// if this is empty or matches qmm's path, skip it
			if (try_path.empty() || str_striequal(try_path, g_gameinfo.qmm_path))
				continue;
			LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to auto-load mod \"{}\"\n", try_path);
			if (mod_load(&g_mod, try_path))
				return true;
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
			// if this is empty or matches qmm's path, skip it
			if (try_path.empty() || str_striequal(try_path, g_gameinfo.qmm_path))
				continue;
			LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to load mod \"{}\"\n", try_path);
			if (mod_load(&g_mod, try_path))
				return true;
		}
	}

	return false;
}

// general code to find a plugin file to load
void main_load_plugin(std::string plugin_path) {
	plugin_t p;
	// absolute path, just attempt to load it directly
	if (!path_is_relative(plugin_path)) {
		if (plugin_load(&p, plugin_path))
			g_plugins.push_back(p);
		return;
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
			return;
		}
	}
}

// get a given argument with G_ARGV, based on game engine type
void main_g_argv(int argn, char* buf, int buflen) {
	// syscall-based games don't return pointers because of QVM interaction, so if this returns anything but
	// null (or true?), we probably are in an api game, and need to get the arg from the return value instead
	intptr_t ret = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], argn, buf, buflen);
	if (ret > 1)
		strncpy(buf, (char*)ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], argn), buflen);
	
	buf[buflen - 1] = '\0';
}

/* Entry point: engine->qmm
   This is the "vmMain" function called by the engine as an entry point into the mod. First thing, we check if the game info is not stored.
   This means that the engine could not be determined, so we fail with G_ERROR and tell the user to set the game in the config file.
   If the engine was determined, it performs some internal tasks on a few events, and then routes the function call according to the
   "overall control flow" comment above.

   The internal events we track:
   GAME_INIT (pre): load mod file, load plugins, and optionally execute a cfg file
   GAME_CONSOLE_COMMAND (pre): handle "qmm" server command
*/
C_DLLEXPORT intptr_t vmMain(int cmd, ...) {
	QMM_GET_VMMAIN_ARGS();

	int loglevel = g_gameinfo.game->is_mod_trace_msg(cmd) ? TRACE : DEBUG;
	LOG(loglevel, "QMM") << fmt::format("vmMain({}) called\n", g_gameinfo.game->mod_msg_names(cmd));

	// couldn't load engine info, so we will just call syscall(G_ERROR) to exit
	if (!g_gameinfo.game) {
		// calling G_ERROR triggers a vmMain(GAME_SHUTDOWN) call, so don't send G_ERROR in GAME_SHUTDOWN or it'll just recurse
		if (cmd != QMM_FAIL_GAME_SHUTDOWN)
			ENG_SYSCALL(QMM_FAIL_G_ERROR, "\n\n=========\nFatal QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n=========\n");
		return 0;
	}

	if (cmd == QMM_MOD_MSG[QMM_GAME_INIT]) {
		// add engine G_PRINT logger (info level)
		log_add_sink([](const AixLog::Metadata& metadata, const std::string& message) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], log_format(metadata, message, false).c_str());
		}, AixLog::Severity::info);

		// get mod dir from engine
		g_gameinfo.moddir = util_get_str_cvar("fs_game");
		// the default mod (including all singleplayer games) return "" for the fs_game, so grab the default mod dir from game info instead
		if (g_gameinfo.moddir.empty())
			g_gameinfo.moddir = g_gameinfo.game->moddir;

		LOG(QMM_LOG_NOTICE, "QMM") << "QMM v" QMM_VERSION " (" QMM_OS " " SUF_DLL ") initializing\n";
		LOG(QMM_LOG_INFO, "QMM") << fmt::format("Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file" );
		LOG(QMM_LOG_INFO, "QMM") << fmt::format("ModDir: {}\n", g_gameinfo.moddir);
		LOG(QMM_LOG_INFO, "QMM") << fmt::format("Config file: \"{}\" {}\n", g_gameinfo.cfg_path, g_cfg.is_discarded() ? "(error)": "");

		LOG(QMM_LOG_INFO, "QMM") << "Built: " QMM_COMPILE " by " QMM_BUILDER "\n";
		LOG(QMM_LOG_INFO, "QMM") << "URL: " QMM_URL "\n";

		LOG(QMM_LOG_INFO, "QMM") << "Registering CVARs\n";

		// make version cvar
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_REGISTER], NULL, "qmm_version", QMM_VERSION, QMM_ENG_MSG[QMM_CVAR_ROM] | QMM_ENG_MSG[QMM_CVAR_SERVERINFO]);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_SET], "qmm_version", QMM_VERSION);

		LOG(QMM_LOG_INFO, "QMM") << "Successfully registered CVARs\n";

		// load mod
		std::string cfg_mod = cfg_get_string(g_cfg, "mod", "auto");
		if (!main_load_mod(cfg_mod)) {
			LOG(QMM_LOG_FATAL, "QMM") << fmt::format("vmMain({}): Unable to load mod using \"{}\"\n", g_gameinfo.game->mod_msg_names(cmd), cfg_mod);
			return 0;
		}
		LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Successfully loaded {} mod \"{}\"\n", g_mod.vmbase ? "VM" : "DLL", g_mod.path);

		// load plugins
		LOG(QMM_LOG_INFO, "QMM") << "Attempting to load plugins\n";
		for (auto plugin_path : cfg_get_array(g_cfg, "plugins")) {
			LOG(QMM_LOG_INFO, "QMM") << fmt::format("Attempting to load plugin \"{}\"\n", plugin_path);
			main_load_plugin(plugin_path);
		}
		LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Successfully loaded {} plugin(s)\n", g_plugins.size());

		// exec the qmmexec cfg
		std::string cfg_execcfg = cfg_get_string(g_cfg, "execcfg", "qmmexec.cfg");
		if (!cfg_execcfg.empty()) {
			LOG(QMM_LOG_NOTICE, "QMM") << fmt::format("Executing config file \"{}\"\n", cfg_execcfg);
			
			// some games don't support a "when" argument
			if (!strcmp(g_gameinfo.game->gamename_short, "STEF2")
				|| !strcmp(g_gameinfo.game->gamename_short, "MOHAA"))
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_SEND_CONSOLE_COMMAND], fmt::format("exec {}\n", cfg_execcfg).c_str());
			else
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_SEND_CONSOLE_COMMAND], QMM_ENG_MSG[QMM_EXEC_APPEND], fmt::format("exec {}\n", cfg_execcfg).c_str());
		}

		// we're done!
		LOG(QMM_LOG_NOTICE, "QMM") << "Startup successful!\n";
	}

	else if (cmd == QMM_MOD_MSG[QMM_GAME_CONSOLE_COMMAND]) {
		char arg0[10], arg1[10], arg2[10];

		main_g_argv(0, arg0, sizeof(arg0));

		if (str_striequal("qmm", arg0)) {
			int argc = (int)ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGC]);
			if (argc > 1)
				main_g_argv(1, arg1, sizeof(arg1));
			if (argc > 2)
				main_g_argv(2, arg2, sizeof(arg2));
			if (argc == 1) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) Usage: qmm <command> [params]\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) Available commands:\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) qmm status - displays information about QMM\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) qmm list - displays information about loaded QMM plugins\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) qmm info <id> - outputs info on plugin with id\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
			}
			else if (str_striequal("status", arg1)) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) QMM v" QMM_VERSION " (" QMM_OS ") loaded\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) ModDir: {}\n", g_gameinfo.moddir).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Config file: \"{}\" {}\n", g_gameinfo.cfg_path, g_cfg.is_discarded() ? " (error)" : "").c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) URL: " QMM_URL "\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Plugin interface: {}:{}\n", QMM_PIFV_MAJOR, QMM_PIFV_MINOR).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Loaded mod file: {}\n", g_mod.path).c_str());
				if (g_mod.vmbase) {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) QVM file size: {}\n", g_mod.qvm.filesize).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) QVM op count: {}\n", g_mod.qvm.header.numops).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) QVM memory offset: {}\n", (void*)g_mod.qvm.memory).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) QVM memory size: {}\n", g_mod.qvm.memorysize).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) QVM codeseg size: {}\n", g_mod.qvm.codeseglen).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) QVM dataseg size: {}\n", g_mod.qvm.dataseglen).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) QVM stack size: {}\n", g_mod.qvm.stackseglen).c_str());
				}
				else {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Mod vmMain() offset: {}\n", (void*)g_mod.pfnvmMain).c_str());
				}
			}
			else if (str_striequal("list", arg1)) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) id - plugin [version]\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) ---------------------\n");
				for (plugin_t& p : g_plugins) {
					int num = 1;
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) {:>2} - {} [{}] ({}) - {}\n", num, p.plugininfo->name, p.plugininfo->version).c_str());
					++num;
				}
			}
			else if (str_striequal("info", arg1)) {
				if (argc == 2) {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) qmm info <id> - outputs info on plugin with id\n");
					return 1;
				}
				unsigned int pid = atoi(arg2);
				if (pid > 0 && pid <= g_plugins.size()) {
					plugin_t& p = g_plugins[pid-1];
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Plugin info for #{}:\n", arg2).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Name: {}\n", p.plugininfo->name).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Version: {}\n", p.plugininfo->version).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) URL: {}\n", p.plugininfo->url).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Author: {}\n", p.plugininfo->author).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Desc: {}\n", p.plugininfo->desc).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Interface version: {}:{}\n", p.plugininfo->pifv_major, p.plugininfo->pifv_minor).c_str());
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Path: {}\n", p.path).c_str());
				}
				else {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Unable to find plugin #{}\n", arg2).c_str());
				}
			}
			else if (str_striequal("loglevel", arg1)) {
				if (argc == 2) {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "(QMM) qmm loglevel <level> - changes QMM log level: TRACE, DEBUG, INFO, NOTICE, WARNING, ERROR, FATAL\n");
					return 1;
				}
				AixLog::Severity severity = log_severity_from_name(arg2);
				log_set_severity(severity);
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("(QMM) Log level set to {}\n", log_name_from_severity(severity)).c_str());
			}

			return 1;
		}
	}

	// store max result
	pluginres_t maxresult = QMM_UNUSED;
	// store return value to pass back to the engine (either real vmMain return value, or value given by a QMM_OVERRIDE/QMM_SUPERCEDE plugin)
	intptr_t final_ret = 0;
	// temp int for return values
	intptr_t ret = 0;
	// begin passing calls to plugins' QMM_vmMain functions
	for (plugin_t& p : g_plugins) {
		g_plugin_result = QMM_UNUSED;
		// call plugin's vmMain and store return value
		ret = p.QMM_vmMain(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]); // update with QMM_MAX_VMMAIN_ARGS
		// set new max result
		maxresult = util_max(g_plugin_result, maxresult);
		if (g_plugin_result == QMM_UNUSED)
			LOG(QMM_LOG_WARNING, "QMM") << fmt::format("vmMain({}): Plugin \"{}\" did not set result flag\n", g_gameinfo.game->eng_msg_names(cmd), p.plugininfo->name);
		if (g_plugin_result == QMM_ERROR)
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("vmMain({}): Plugin \"{}\" resulted in ERROR\n", g_gameinfo.game->eng_msg_names(cmd), p.plugininfo->name);
		// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
		if (g_plugin_result >= QMM_OVERRIDE)
			final_ret = ret;
	}
	// call real vmMain function (unless a plugin resulted in QMM_SUPERCEDE)
	if (maxresult < QMM_SUPERCEDE) {
		ret = g_mod.pfnvmMain(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]); // update with QMM_MAX_VMMAIN_ARGS
		// the return value for GAME_CLIENT_CONNECT is a char* so we have to modify the pointer value for QVMs. the
		// char* is a string to print if the client should not be allowed to connect, so only bother if it's not NULL
		if (cmd == QMM_MOD_MSG[QMM_GAME_CLIENT_CONNECT] && ret && g_mod.vmbase)
			ret += g_mod.vmbase;
	}
	// if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual mod's return value back to the engine
	if (maxresult < QMM_OVERRIDE) 
		final_ret = ret;
	// pass calls to plugins' QMM_vmMain_Post functions (ignore return values and results)
	for (plugin_t& p : g_plugins) {
		p.QMM_vmMain_Post(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]); // update with QMM_MAX_VMMAIN_ARGS
	}

	// handle shut down (this is after the mod and plugins get called with GAME_SHUTDOWN)
	if (cmd == QMM_MOD_MSG[QMM_GAME_SHUTDOWN]) {
		LOG(QMM_LOG_NOTICE, "QMM") << "Shutting down mod\n";
		mod_unload(&g_mod);

		LOG(QMM_LOG_INFO, "QMM") << "Shutting down plugins\n";
		for (plugin_t& p : g_plugins) {
			//unload each plugin (call QMM_Detach, and then dlclose)
			plugin_unload(&p);
		}
		g_plugins.clear();

		LOG(QMM_LOG_INFO, "QMM") << "Finished shutting down\n";
	}

	LOG(loglevel, "QMM") << fmt::format("vmMain({}) returning {}\n", g_gameinfo.game->mod_msg_names(cmd), final_ret);
	
	return final_ret;
}

/* Entry point: mod->qmm
   This is the "syscall" function called by the mod as a way to pass info to or get info from the engine.
   It routes the function call according to the "overall control flow" comment above.
*/
intptr_t syscall(int cmd, ...) {
	QMM_GET_SYSCALL_ARGS();

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("syscall({}) called\n", g_gameinfo.game->eng_msg_names(cmd));

	// store max result
	pluginres_t maxresult = QMM_UNUSED;
	// store return value to pass back to the mod (either real syscall return value, or value given by a QMM_OVERRIDE/QMM_SUPERCEDE plugin)
	intptr_t final_ret = 0;
	// temp int for return values
	intptr_t ret = 0;
	// begin passing calls to plugins' QMM_syscall functions
	for (plugin_t& p : g_plugins) {
		g_plugin_result = QMM_UNUSED;
		// call plugin's syscall and store return value
		ret = p.QMM_syscall(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]); // update with QMM_MAX_SYSCALL_ARGS

		// set new max result
		maxresult = util_max(g_plugin_result, maxresult);
		if (g_plugin_result == QMM_UNUSED)
			LOG(QMM_LOG_WARNING, "QMM") << fmt::format("syscall({}): Plugin \"{}\" did not set result flag\n", g_gameinfo.game->mod_msg_names(cmd), p.plugininfo->name);
		if (g_plugin_result == QMM_ERROR)
			LOG(QMM_LOG_ERROR, "QMM") << fmt::format("syscall({}): Plugin \"{}\" resulted in ERROR\n", g_gameinfo.game->mod_msg_names(cmd), p.plugininfo->name);
		// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
		if (g_plugin_result >= QMM_OVERRIDE)
			final_ret = ret;
	}
	// call real syscall function (unless a plugin resulted in QMM_SUPERCEDE)
	if (maxresult < QMM_SUPERCEDE)
		ret = g_gameinfo.pfnsyscall(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]); // update with QMM_MAX_SYSCALL_ARGS
	// if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual engine's return value back to the mod
	if (maxresult < QMM_OVERRIDE)
		final_ret = ret;
	// pass calls to plugins' QMM_syscall_Post functions (ignore return values and results)
	for (plugin_t& p : g_plugins) {
		p.QMM_syscall_Post(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]); // update with QMM_MAX_SYSCALL_ARGS
	}

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("syscall({}) returning {}\n", g_gameinfo.game->eng_msg_names(cmd), final_ret);

	return final_ret;
}
