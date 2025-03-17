/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifdef QMM_GETGAMEAPI_SUPPORT

#ifndef __QMM2_GAME_STEF2_H__
#define __QMM2_GAME_STEF2_H__

// todo: temp
enum {
	G_PRINT, G_ERROR, G_ARGV, G_ARGC, G_SEND_CONSOLE_COMMAND, G_SEND_SERVER_COMMAND,
	G_CVAR_REGISTER, G_CVAR_SET, G_CVAR_VARIABLE_STRING_BUFFER, G_CVAR_VARIABLE_INTEGER_VALUE,
	G_FS_FOPEN_FILE, G_FS_READ, G_FS_WRITE, G_FS_FCLOSE_FILE,
};
enum {
	GAME_INIT, GAME_SHUTDOWN, GAME_CONSOLE_COMMAND, GAME_CLIENT_CONNECT, GAME_CLIENT_COMMAND
};

// import ("syscall") codes
enum {


	NUM_GAME_IMPORTS
};

// export ("vmMain") codes
enum {
	GAMEV_APIVERSION,

	NUM_GAME_EXPORTS,
};

// these import messages do not have an exact analogue in MOHAA
enum {
};

typedef int(*pfn_import_t)(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8);
typedef int(*pfn_export_t)(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);

#endif // __QMM2_GAME_STEF2_H__

#endif // QMM_GETGAMEAPI_SUPPORT
