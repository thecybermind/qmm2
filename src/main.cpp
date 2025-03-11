/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#include "format.h"
#include "config.h"
#include "CModMgr.h"
#include "main.h"
#include "game_api.h"
#include "qmmapi.h"
#include "plugin.h"
#include "util.h"
#include "version.h"

CModMgr* g_ModMgr = NULL;
CPluginMgr* g_PluginMgr = NULL;
game_info_t g_gameinfo;

/* About overall control flow:
   syscall (mod->engine) call flow for QVM mods only:
   1. mod calls <GAME>_vmsyscall function
   2. pointers are converted (if not NULL, the QVM data segment base address is added)
   3. continue with next section as if it were a DLL mod
   
   syscall (mod->engine) call flow for DLL mods:
   1. call is handled by QMM_syscall
   2. call is passed to plugins' QMM_syscall functions
   3. call is passed to engine syscall (unless at least 1 plugin uses the result QMM_SUPERCEDE)
   4. call is passed to plugins' QMM_syscall_Post functions
   5. engine syscall return value (or a value given by last plugin which uses result QMM_SUPERCEDE or QMM_OVERRIDE)
      is returned to mod
   
   vmMain (engine->mod) call flow:
   1. call is handled by vmMain
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
	// save syscall pointer
	g_gameinfo.pfnsyscall = syscall;
	// save qmm module path
	g_gameinfo.qmm_path = get_qmm_modulepath();
	g_gameinfo.qmm_dir = my_dirname(g_gameinfo.qmm_path);
	g_gameinfo.qmm_file = my_basename(g_gameinfo.qmm_path);
	fmt::print("[QMM] ::dllEntry(): QMM loaded! Path: \"{}\"\n", g_gameinfo.qmm_path);

	// load config file
	g_cfg = cfg_load(fmt::format("{}/qmm2.json", g_gameinfo.qmm_dir));
	if (g_cfg.empty()) {
		// a default constructed json object is a blank {}, so in case of load failure, we can still try to read from it and assume defaults
		fmt::print("[QMM] WARNING: ::dllEntry(): Unable to load config file, all settings will use default values\n");
	}

	std::string cfg_game = cfg_get_string(g_cfg, "game", "auto");
	
	// find what game we are loaded in
	for (int i = 0; g_supportedgames[i].dllname; i++) {
		supportedgame_t& game = g_supportedgames[i];
		// if short name matches config option, we found it!
		if (cfg_game == game.gamename_short) {
			g_gameinfo.game = &game;
			g_gameinfo.isautodetected = false;
			g_ModMgr = CModMgr::GetInstance(syscall);
			break;
		}
		// otherwise, if auto, we need to check matching dll names
		if (cfg_game == "auto") {
			if (g_gameinfo.qmm_file == game.dllname) {
				g_gameinfo.game = &game;
				g_gameinfo.isautodetected = true;
				g_ModMgr = CModMgr::GetInstance(syscall);
				break;
			}
		}
	}
	
	// failed to get engine information
	if (!g_gameinfo.game) {
		fmt::print("[QMM] WARNING: ::dllEntry(): Unable to determine game engine, using game=\"{}\"\n", cfg_game);
		return;
	}

	g_PluginMgr = CPluginMgr::GetInstance();
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
			g_gameinfo.pfnsyscall(QMM_FAIL_G_ERROR, "\n\n=========\nCritical QMM Error:\nQMM was unable to determine the game engine.\nPlease set the \"game\" option in qmm2.json.\nRefer to the documentation for more information.\n=========\n");
		return 0;
	}

	if (cmd == QMM_MOD_MSG[QMM_GAME_INIT]) {
		// get mod dir from engine
		g_gameinfo.moddir = get_str_cvar("fs_game");
		// RTCWSP returns "" for the mod, and others may too? grab the default mod dir from game info
		if (g_gameinfo.moddir.empty()) {
			g_gameinfo.moddir = g_gameinfo.game->moddir;
		}

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] QMM v" QMM_VERSION " (" QMM_OS ") loaded\n");
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Mod: {}\n", g_gameinfo.moddir).c_str());
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

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Attempting to load mod\n");
		if (!g_ModMgr->LoadMod()) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ERROR], "[QMM] FATAL ERROR: Unable to load mod\n");
			return 0;
		}
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Successfully loaded {} mod \"{}\"\n", g_ModMgr->Mod()->IsVM() ? "VM" : "DLL", g_ModMgr->Mod()->File()).c_str());

		// load plugins
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Attempting to load plugins\n");
		std::vector<std::string> plugin_files = cfg_get_array(g_cfg, "plugins");
		for (auto plugin_file : plugin_files) {
			plugin_t p;
			if (plugin_load(&p, plugin_file)) {
				g_plugins.push_back(p);
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
		char arg0[5], arg1[8], arg2[5];
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], 0, arg0, sizeof(arg0));
		arg0[sizeof(arg0) - 1] = '\0';
		int argc = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGC]);

		if (!strcasecmp("qmm", arg0)) {
			if (argc > 1)
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], 1, arg1, sizeof(arg1));
				arg1[sizeof(arg1) - 1] = '\0';
			if (argc > 2)
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_ARGV], 2, arg2, sizeof(arg2));
				arg2[sizeof(arg2) - 1] = '\0';
			if (argc == 1) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Usage: qmm <command> [params]\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Available commands:\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] qmm status - displays information about QMM\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] qmm list - displays information about loaded QMM plugins\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] qmm info <id> - outputs info on plugin with id\n");
				return 1;
			} else if (!strcasecmp("status", arg1)) {
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] QMM v" QMM_VERSION " (" QMM_OS ") loaded\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Game: {}/\"{}\" (Source: {})\n", g_gameinfo.game->gamename_short, g_gameinfo.game->gamename_long, g_gameinfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Mod: {}\n", g_gameinfo.moddir).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] URL: " QMM_URL "\n");
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Loaded mod file: {}\n", g_ModMgr->Mod()->File()).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] NoCrash: {}\n", get_int_cvar("qmm_nocrash") ? "on" : "off").c_str());
				g_ModMgr->Mod()->Status();
			} else if (!strcasecmp("list", arg1)) {
				g_PluginMgr->ListPlugins();
			} else if (!strcasecmp("info", arg1)) {
				if (argc == 2) {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] qmm info <id> - outputs info on plugin with id\n");
					return 1;
				}
				const plugininfo_t* plugininfo = g_PluginMgr->PluginInfo(atoi(arg2));
				if (!plugininfo) {
					ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Unable to find plugin # {}\n", arg2).c_str());
					return 1;
				}
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Plugin info for # {}:\n", arg2).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Name: \"{}\"\n", plugininfo->name).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Version: \"{}\"\n", plugininfo->version).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] URL: \"{}\"\n", plugininfo->url).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Author: \"{}\"\n", plugininfo->author).c_str());
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Desc: \"{}\"\n", plugininfo->desc).c_str());
			}

			return 1;
		}
	}

	else if (cmd == QMM_MOD_MSG[QMM_GAME_CLIENT_COMMAND]) {
		if (get_int_cvar("qmm_nocrash")) {
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
				char* y = vaf("[QMM] NoCrash: Userid %d has attempted to execute a command longer than 900 chars\n", arg0);
				ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], y);
				log_write(y);
				return 1;
			}
		}
	}

	// pass vmMain call to plugins, allow them to halt
	int ret = g_PluginMgr->CallvmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);

	// if user is connecting for the first time, user is not a bot, and "nogreeting" option is not set
	if (cmd == QMM_MOD_MSG[QMM_GAME_CLIENT_CONNECT] && arg1 && !arg2) {
		bool cfg_nogreeting = cfg_get_bool(g_cfg, "nogreeting", false);

		if (!cfg_nogreeting) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_SEND_SERVER_COMMAND], arg0, "print \"^7This server is running ^4QMM^7 v^4" QMM_VERSION "^7\n\"");
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_SEND_SERVER_COMMAND], arg0, "print \"^7URL: ^4" QMM_URL "^7\n\"");
		}
	}
	else if (cmd == QMM_MOD_MSG[QMM_GAME_SHUTDOWN]) {
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Shutting down plugins\n");
		delete g_PluginMgr;

		// this is after plugin unload, so plugins can call mod's vmMain while shutting down
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Shutting down mod\n");
		delete g_ModMgr;

		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] Finished shutting down, prepared for unload.\n");
	}

	return ret;
}

/* Entry point: mod->qmm
   This is the "syscall" function called by the mod as a way to pass info to or get info from the engine.
   It performs some internal tasks on a few events, and then routes the function call according to the "overall control flow" comment above.

   The internal events we track:
   QMM_G_FS_FCLOSE_FILE (pre): watch for the mod closing the log file handle
   QMM_G_SEND_SERVER_COMMAND (pre): if "qmm_nocrash" cvar is set to 1, block client commands that are too long (msgboom bug)
   QMM_G_FS_FOPEN_FILE (post): watch for the mod opening a handle for the log file (based on g_log cvar), and save it for logging
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
		if (get_int_cvar("qmm_nocrash") && args[1] && strlen((char*)args[1]) >= 1022) {
			ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], "[QMM] NoCrash: A user has attempted to use the vsay exploit");
			log_write("[QMM] NoCrash: A user has attempted to use the vsay exploit", -1);
			return 1;
		}
	}

	// pass syscall to plugins, allow them to halt
	int ret = g_PluginMgr->Callsyscall(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12]);

	// if this is a call to open a file for APPEND or APPEND_SYNC
	if (cmd == QMM_ENG_MSG[QMM_G_FS_FOPEN_FILE] && args[1]) {
		if (args[2] == QMM_ENG_MSG[QMM_FS_APPEND] || args[2] == QMM_ENG_MSG[QMM_FS_APPEND_SYNC]) {
			// compare filename against g_log cvar
			if (!strcasecmp(get_str_cvar("g_log"), (char*)(args[0]))) {
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
