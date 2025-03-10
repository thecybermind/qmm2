/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/format.h"
#include "config.h"
#include "CModMgr.h"
#include "main.h"
#include "game_api.h"
#include "qmmapi.h"
#include "qmm.h"
#include "util.h"
#include "version.h"

CModMgr* g_ModMgr = NULL;
CPluginMgr* g_PluginMgr = NULL;
game_info_t g_GameInfo;

// syscall flow for QVM mods:
//   mod calls <GAME>_vmsyscall (thinks it is the engine's syscall)
//   pointers are converted
//   continue with next section

// syscall flow for all mods:
//   call passed to QMM_SysCall
//   call passed to plugins
//   call passed to engine

// vmMain flow for all mods:
//   engine calls vmMain (thinks it is the mod's syscall)
//   call passed to plugins
//   call passed to mod

// engine->qmm
// load config file, get various load-time settings, auto-detect the game as neccesary, store engine/qvm syscalls
// note: we can't call syscall from here
C_DLLEXPORT void dllEntry(eng_syscall_t syscall) {
	// save syscall pointer
	g_GameInfo.pfnsyscall = syscall;
	// save qmm module path
	g_GameInfo.qmm_path = get_qmm_modulepath();
	g_GameInfo.qmm_dir = my_dirname(g_GameInfo.qmm_path);
	g_GameInfo.qmm_file = my_basename(g_GameInfo.qmm_path);
	fmt::print("[QMM] Detected QMM load from directory \"{}\"\n", g_GameInfo.qmm_dir);

	// load config file
	g_cfg = cfg_load(fmt::format("{}/qmm2.json", g_GameInfo.qmm_dir));
	if (g_cfg.empty()) {
		// a default constructed json object is a blank {}, so in case of load failure, we can still try to read from it and assume defaults
		fmt::print("[QMM] WARNING: ::dllEntry(): Unable to load config file, all settings will use default values\n");
	}

	std::string cfg_game = cfg_get_string(g_cfg, "game", "auto");
	
	// find what game we are loaded in
	for (int i = 0; g_SupportedGameList[i].dllname; i++) {
		supported_game_t& game = g_SupportedGameList[i];
		// if short name matches config option, we found it!
		if (cfg_game == game.gamename_short) {
			g_GameInfo.game = &game;
			g_GameInfo.isautodetected = false;
			g_ModMgr = CModMgr::GetInstance(QMM_syscall);
			break;
		}
		// otherwise, if auto, we need to check matching dll names
		if (cfg_game == "auto") {
			if (g_GameInfo.qmm_file == game.dllname) {
				g_GameInfo.game = &game;
				g_GameInfo.isautodetected = true;
				g_ModMgr = CModMgr::GetInstance(QMM_syscall);
				break;
			}
		}
	}
	
	// failed to get engine information
	if (!g_GameInfo.game) {
		return;
	}

	g_PluginMgr = CPluginMgr::GetInstance();
}

// engine->qmm
C_DLLEXPORT int vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	// couldn't load engine info, so we will just call syscall(G_ERROR) to exit
	if (!g_GameInfo.game) {
		// G_ERROR and GAME_SHUTDOWN are 1 in every known q3 engine game, so we just use some temp defines.
		// also, calling G_ERROR triggers a vmMain(GAME_SHUTDOWN) call, so don't send G_ERROR in GAME_SHUTDOWN or it'll just recurse		
		if (cmd != QMM_FAIL_GAME_SHUTDOWN)
			g_GameInfo.pfnsyscall(QMM_FAIL_G_ERROR, "\n\n=========\nCritical QMM Error:\nQMM was unable to determine the game.\nPlease set the \"game\" option in qmm.json.\nRefer to the documentation for more information.\n=========\n");
		return 0;
	}

	if (cmd == MOD_MSG[QMM_GAME_INIT]) {
		// get mod dir from engine
		g_GameInfo.moddir = get_str_cvar("fs_game");
		// RTCWSP returns "" for the mod, and others may too. grab the default mod dir from game info
		if (g_GameInfo.moddir.empty()) {
			g_GameInfo.moddir = g_GameInfo.game->moddir;
		}

		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] QMM v" QMM_VERSION " (" QMM_OS ") loaded\n");
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Game: {}/\"{}\" (Source: {})\n", g_GameInfo.game->gamename_short, g_GameInfo.game->gamename_long, g_GameInfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Mod: {}\n", g_GameInfo.moddir).c_str());
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] URL: " QMM_URL "\n");

		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Registering CVARs\n");

		//make version cvar
		ENG_SYSCALL(ENG_MSG[QMM_G_CVAR_REGISTER], NULL, "qmm_version", QMM_VERSION, ENG_MSG[QMM_CVAR_ROM] | ENG_MSG[QMM_CVAR_SERVERINFO]);
		ENG_SYSCALL(ENG_MSG[QMM_G_CVAR_SET], "qmm_version", QMM_VERSION);

		//make game cvar
		ENG_SYSCALL(ENG_MSG[QMM_G_CVAR_REGISTER], NULL, "qmm_game", g_GameInfo.game->gamename_short, ENG_MSG[QMM_CVAR_ROM]);
		ENG_SYSCALL(ENG_MSG[QMM_G_CVAR_SET], "qmm_game", g_GameInfo.game->gamename_short);

		//make nocrash cvar
		ENG_SYSCALL(ENG_MSG[QMM_G_CVAR_REGISTER], NULL, "qmm_nocrash", "1", ENG_MSG[QMM_CVAR_ARCHIVE]);

		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Attempting to load mod\n");
		if (!g_ModMgr->LoadMod()) {
			ENG_SYSCALL(ENG_MSG[QMM_G_ERROR], "[QMM] FATAL ERROR: Unable to load mod\n");
			return 0;
		}
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Successfully loaded {} mod \"{}\"\n", g_ModMgr->Mod()->IsVM() ? "VM" : "DLL", g_ModMgr->Mod()->File()).c_str());

		//load plugins
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Attempting to load plugins\n");
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Successfully loaded {} plugin(s)\n", g_PluginMgr->LoadPlugins()).c_str());

		// exec the qmmexec cfg
		std::string cfg_execcfg = cfg_get_string(g_cfg, "execcfg", "qmmexec.cfg");
		if (!cfg_execcfg.empty()) {
			ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Executing config file \"{}\"\n", cfg_execcfg).c_str());
			ENG_SYSCALL(ENG_MSG[QMM_G_SEND_CONSOLE_COMMAND], ENG_MSG[QMM_EXEC_APPEND], fmt::format("exec {}\n", cfg_execcfg).c_str());
		}

		// we're done!
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Startup successful, proceeding to mod startup\n");
	}

	else if (cmd == MOD_MSG[QMM_GAME_CONSOLE_COMMAND]) {
		char arg0[5], arg1[8], arg2[5];
		ENG_SYSCALL(ENG_MSG[QMM_G_ARGV], 0, arg0, sizeof(arg0));
		arg0[sizeof(arg0) - 1] = '\0';
		int argc = ENG_SYSCALL(ENG_MSG[QMM_G_ARGC]);

		if (!strcasecmp("qmm", arg0)) {
			if (argc > 1)
				ENG_SYSCALL(ENG_MSG[QMM_G_ARGV], 1, arg1, sizeof(arg1));
				arg1[sizeof(arg1) - 1] = '\0';
			if (argc > 2)
				ENG_SYSCALL(ENG_MSG[QMM_G_ARGV], 2, arg2, sizeof(arg2));
				arg2[sizeof(arg2) - 1] = '\0';
			if (argc == 1) {
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Usage: qmm <command> [params]\n");
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Available commands:\n");
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] qmm status - displays information about QMM\n");
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] qmm list - displays information about loaded QMM plugins\n");
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] qmm info <id> - outputs info on plugin with id\n");
				return 1;
			} else if (!strcasecmp("status", arg1)) {
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] QMM v" QMM_VERSION " (" QMM_OS ") loaded\n");
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Game: {}/\"{}\" (Source: {})\n", g_GameInfo.game->gamename_short, g_GameInfo.game->gamename_long, g_GameInfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Mod: {}\n", g_GameInfo.moddir).c_str());
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] URL: " QMM_URL "\n");
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Loaded mod file: {}\n", g_ModMgr->Mod()->File()).c_str());
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] NoCrash: {}\n", get_int_cvar("qmm_nocrash") ? "on" : "off").c_str());
				g_ModMgr->Mod()->Status();
			} else if (!strcasecmp("list", arg1)) {
				g_PluginMgr->ListPlugins();
			} else if (!strcasecmp("info", arg1)) {
				if (argc == 2) {
					ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] qmm info <id> - outputs info on plugin with id\n");
					return 1;
				}
				const plugininfo_t* plugininfo = g_PluginMgr->PluginInfo(atoi(arg2));
				if (!plugininfo) {
					ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Unable to find plugin # {}\n", arg2).c_str());
					return 1;
				}
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Plugin info for # {}:\n", arg2).c_str());
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Name: \"{}\"\n", plugininfo->name).c_str());
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Version: \"{}\"\n", plugininfo->version).c_str());
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] URL: \"{}\"\n", plugininfo->url).c_str());
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Author: \"{}\"\n", plugininfo->author).c_str());
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], fmt::format("[QMM] Desc: \"{}\"\n", plugininfo->desc).c_str());
			}

			return 1;
		}
	}

	else if (cmd == MOD_MSG[QMM_GAME_CLIENT_COMMAND]) {
		if (get_int_cvar("qmm_nocrash")) {
			int argc = ENG_SYSCALL(ENG_MSG[QMM_G_ARGC]);
			int len = 0;
			static char bigbuf[1024];
			for (int i = 0; i < argc; ++i) {
				ENG_SYSCALL(ENG_MSG[QMM_G_ARGV], i, bigbuf, sizeof(bigbuf));
				bigbuf[sizeof(bigbuf) - 1] = '\0';
				len += (strlen(bigbuf) + 1);	//1 for space
				if (len > 900)
					break;
			}
			--len;	//get rid of the last space added
			if (len >= 900) {
				char* y = vaf("[QMM] NoCrash: Userid %d has attempted to execute a command longer than 900 chars\n", arg0);
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], y);
				log_write(y);
				return 1;
			}
		}
	}

	//pass vmMain call to plugins, allow them to halt
	int ret = g_PluginMgr->CallvmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);

	//if user is connecting for the first time, user is not a bot, and "nogreeting" option is not set
	if (cmd == MOD_MSG[QMM_GAME_CLIENT_CONNECT] && arg1 && !arg2) {
		bool cfg_nogreeting = cfg_get_bool(g_cfg, "nogreeting", false);

		if (!cfg_nogreeting) {
			ENG_SYSCALL(ENG_MSG[QMM_G_SEND_SERVER_COMMAND], arg0, "print \"^7This server is running ^4QMM^7 v^4" QMM_VERSION "^7\n\"");
			ENG_SYSCALL(ENG_MSG[QMM_G_SEND_SERVER_COMMAND], arg0, "print \"^7URL: ^4" QMM_URL "^7\n\"");
		}
	}
	else if (cmd == MOD_MSG[QMM_GAME_SHUTDOWN]) {
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Shutting down plugins\n");
		delete g_PluginMgr;

		//this is after plugin unload, so plugins can call mod's vmMain while shutting down
		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Shutting down mod\n");
		delete g_ModMgr;

		ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Finished shutting down, prepared for unload.\n");
	}

	return ret;
}

// mod->qmm
int QMM_syscall(int cmd, ...) {
	va_list arglist;
	int args[13];	//JK2 decided to mess it all up and have a single cmd with 13 args
	va_start(arglist, cmd);
	for (int i = 0; i < (sizeof(args)/sizeof(args[0])); ++i)
		args[i] = va_arg(arglist, int);
	va_end(arglist);

	//if this is a call to close a file, check the handle to see if it matches our existing log handle
	if (cmd == ENG_MSG[QMM_G_FS_FCLOSE_FILE]) {
		if (args[0] == log_get()) {
			//we have it, output final line and clear log file handle
			ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Detected close operation on g_log file handle, unhooking...\n");
			log_write("[QMM] Detected close operation on g_log file handle, unhooking...\n\n");
			log_set(-1);
		}
	}

	//integrated nocrash protection
	//vsay fix
	else if (cmd == ENG_MSG[QMM_G_SEND_SERVER_COMMAND]) {
		if (get_int_cvar("qmm_nocrash") && args[1] && strlen((char*)args[1]) >= 1022) {
			ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] NoCrash: A user has attempted to use the vsay exploit");
			log_write("[QMM] NoCrash: A user has attempted to use the vsay exploit", -1);
			return 1;
		}
	}

	//pass syscall to plugins, allow them to halt
	int ret = g_PluginMgr->Callsyscall(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12]);

	//if this is a call to open a file for APPEND or APPEND_SYNC
	if (cmd == ENG_MSG[QMM_G_FS_FOPEN_FILE] && args[1]) {
		if (args[2] == ENG_MSG[QMM_FS_APPEND] || args[2] == ENG_MSG[QMM_FS_APPEND_SYNC]) {
			//compare filename against g_log cvar
			if (!strcasecmp(get_str_cvar("g_log"), (char*)(args[0]))) {
				//we have it, save log file handle
				log_set(*(int*)(args[1]));
				ENG_SYSCALL(ENG_MSG[QMM_G_PRINT], "[QMM] Successfully hooked g_log file\n");
				log_write("[QMM] Successfully hooked g_log file\n");
				log_write("[QMM] QMM v" QMM_VERSION " (" QMM_OS ") loaded\n");
				log_write(fmt::format("[QMM] Game: {}/\"{}\" (Source: {})\n", g_GameInfo.game->gamename_short, g_GameInfo.game->gamename_long, g_GameInfo.isautodetected ? "Auto-detected" : "Config file" ).c_str());
				log_write(fmt::format("[QMM] Mod: {}\n", g_GameInfo.moddir).c_str());
				log_write("[QMM] Built: " QMM_COMPILE " by " QMM_BUILDER "\n");
				log_write("[QMM] URL: " QMM_URL "\n");
			}
		}
	}

	return ret;
}
