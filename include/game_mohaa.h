/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_GAME_MOHAA_H__
#define __QMM2_GAME_MOHAA_H__

// import ("syscall") codes
enum {
	G_PRINTF,
	G_DPRINTF,
	G_DPRINTF2,
	G_DEBUGPRINTF,
	G_ERROR,
	G_MILLISECONDS,
	G_LV_CONVERTSTRING,
	G_CL_LV_CONVERTSTRING,
	G_MALLOC,
	G_FREE,
	G_CVAR_GET,
	G_CVAR_SET,
	G_CVAR_SET2,
	G_NEXTCVAR,
	G_CVAR_CHECKRANGE,
	G_ARGC,
	G_ARGV,
	G_ARGS,
	G_ADDCOMMAND,
	G_FS_READFILE,
	G_FS_FREEFILE,
	G_FS_WRITEFILE,
	G_FS_FOPEN_FILE_WRITE,
	G_FS_FOPEN_FILE_APPEND,
	G_FS_FOPEN_FILE,
	G_FS_PREPFILEWRITE,
	G_FS_WRITE,
	G_FS_READ,
	G_FS_FCLOSE_FILE,
	G_FS_TELL,
	G_FS_SEEK,
	G_FS_FLUSH,
	G_FS_FILENEWER,
	G_FS_CANONICALFILENAME,
	G_FS_LISTFILES,
	G_FS_FREEFILELIST,
	G_GETARCHIVEFILENAME,
	G_SEND_CONSOLE_COMMAND,
	G_EXECUTECONSOLECOMMAND,
	G_DEBUGGRAPH,
	G_SEND_SERVER_COMMAND,
	G_DROPCLIENT,
	G_MSG_WRITEBITS,
	G_MSG_WRITECHAR,
	G_MSG_WRITEBYTE,
	G_MSG_WRITESVC,
	G_MSG_WRITESHORT,
	G_MSG_WRITELONG,
	G_MSG_WRITEFLOAT,
	G_MSG_WRITESTRING,
	G_MSG_WRITEANGLE8,
	G_MSG_WRITEANGLE16,
	G_MSG_WRITECOORD,
	G_MSG_WRITEDIR,
	G_MSG_STARTCGM,
	G_MSG_ENDCGM,
	G_MSG_SETCLIENT,
	G_SETBROADCASTVISIBLE,
	G_SETBROADCASTHEARABLE,
	G_SETBROADCASTALL,
	G_SET_CONFIGSTRING,
	G_GET_CONFIGSTRING,
	G_SET_USERINFO,
	G_GET_USERINFO,
	G_SETBRUSHMODEL,
	G_MODELBOUNDSFROMNAME,
	G_SIGHTTRACEENTITY,
	G_SIGHTTRACE,
	G_TRACE,
	G_CM_VISUALOBFUSCATION,
	G_GETSHADER,
	G_POINTCONTENTS,
	G_POINTBRUSHNUM,
	G_ADJUSTAREAPORTALSTATE,
	G_AREAFORPOINT,
	G_AREASCONNECTED,
	G_INPVS,
	G_LINKENTITY,
	G_UNLINKENTITY,
	G_AREAENTITIES,
	G_CLIPTOENTITY,
	G_HITENTITY,
	G_IMAGEINDEX,
	G_ITEMINDEX,
	G_SOUNDINDEX,
	G_TIKI_REGISTERMODEL,
	G_MODELTIKI,
	G_MODELTIKIANIM,
	G_SETLIGHTSTYLE,
	G_GAMEDIR,
	G_SETMODEL,
	G_CLEARMODEL,
	G_TIKI_NUMANIMS,
	G_TIKI_NUMSURFACES,
	G_TIKI_NUMTAGS,
	G_TIKI_CALCULATEBOUNDS,
	G_TIKI_GETSKELETOR,
	G_ANIM_NAMEFORNUM,
	G_ANIM_NUMFORNAME,
	G_ANIM_RANDOM,
	G_ANIM_NUMFRAMES,
	G_ANIM_TIME,
	G_ANIM_FRAMETIME,
	G_ANIM_CROSSTIME,
	G_ANIM_DELTA,
	G_ANIM_ANGULARDELTA,
	G_ANIM_HASDELTA,
	G_ANIM_DELTAOVERTIME,
	G_ANIM_ANGULARDELTAOVERTIME,
	G_ANIM_FLAGS,
	G_ANIM_FLAGSSKEL,
	G_ANIM_HASCOMMANDS,
	G_ANIM_HASCOMMANDS_CLIENT,
	G_NUMHEADMODELS,
	G_GETHEADMODEL,
	G_NUMHEADSKINS,
	G_GETHEADSKIN,
	G_FRAME_COMMANDS,
	G_FRAME_COMMANDS_CLIENT,
	G_SURFACE_NAMETONUM,
	G_SURFACE_NUMTONAME,
	G_TAG_NUMFORNAME,
	G_TAG_NAMEFORNUM,
	G_TIKI_ORIENTATIONINTERNAL,
	G_TIKI_TRANSFORMINTERNAL,
	G_TIKI_ISONGROUNDINTERNAL,
	G_TIKI_SETPOSEINTERNAL,
	G_CM_GETHITLOCATIONINFO,
	G_CM_GETHITLOCATIONINFOSECONDARY,
	G_ALIAS_ADD,
	G_ALIAS_FINDRANDOM,
	G_ALIAS_DUMP,
	G_ALIAS_CLEAR,
	G_ALIAS_UPDATEDIALOG,
	G_TIKI_NAMEFORNUM,
	G_GLOBALALIAS_ADD,
	G_GLOBALALIAS_FINDRANDOM,
	G_GLOBALALIAS_DUMP,
	G_GLOBALALIAS_CLEAR,
	G_CENTERPRINTF,
	G_LOCATIONPRINTF,
	G_SOUND,
	G_STOPSOUND,
	G_SOUNDLENGTH,
	G_SOUNDAMPLITUDES,
	G_S_ISSOUNDPLAYING,
	G_CALCCRC,
	GVP_DEBUGLINES,
	GVP_NUMDEBUGLINES,
	GVP_DEBUGSTRINGS,
	GVP_NUMDEBUGSTRINGS,
	G_LOCATE_GAME_DATA,
	G_SETFARPLANE,
	G_SETSKYPORTAL,
	G_POPMENU,
	G_SHOWMENU,
	G_HIDEMENU,
	G_PUSHMENU,
	G_HIDEMOUSECURSOR,
	G_SHOWMOUSECURSOR,
	G_MAPTIME,
	G_LOADRESOURCE,
	G_CLEARRESOURCE,
	G_KEY_STRINGTOKEYNUM,
	G_KEY_KEYNUMTOBINDSTRING,
	G_KEY_GETKEYSFORCOMMAND,
	G_ARCHIVELEVEL,
	G_ADDSVSTIMEFIXUP,
	G_HUDDRAWSHADER,
	G_HUDDRAWALIGN,
	G_HUDDRAWRECT,
	G_HUDDRAWVIRTUALSIZE,
	G_HUDDRAWCOLOR,
	G_HUDDRAWALPHA,
	G_HUDDRAWSTRING,
	G_HUDDRAWFONT,
	G_SANITIZENAME,
	G_PVSSOUNDINDEX,
	GVP_FSDEBUG,

	G_PRINT = G_PRINTF
};

// export ("vmMain") codes
enum {
	GAMEV_APIVERSION,
	GAME_INIT,
	GAME_SHUTDOWN,
	GAME_CLEANUP,
	GAME_PRECACHE,
	GAME_SETMAP,
	GAME_RESTART,
	GAME_SETTIME,
	GAME_SPAWN_ENTITIES,
	GAME_CLIENT_CONNECT,
	GAME_CLIENT_BEGIN,
	GAME_CLIENT_USERINFOCHANGED,
	GAME_CLIENT_DISCONNECT,
	GAME_CLIENT_COMMAND,
	GAME_CLIENT_THINK,
	GAME_BOTBEGIN,
	GAME_BOTTHINK,
	GAME_PREP_FRAME,
	GAME_RUN_FRAME,
	GAME_SERVER_SPAWNED,
	GAME_REGISTER_SOUNDS,
	GAME_ALLOW_PAUSED,
	GAME_CONSOLE_COMMAND,
	GAME_ARCHIVE_PERSISTANT,
	GAME_WRITE_LEVEL,
	GAME_READ_LEVEL,
	GAME_LEVEL_ARCHIVE_VALID,
	GAME_ARCHIVE_INTEGER,
	GAME_ARCHIVE_FLOAT,
	GAME_ARCHIVE_STRING,
	GAME_ARCHIVE_SVSTIME,
	GAME_TIKI_ORIENTATION,
	GAME_DEBUG_CIRCLE,
	GAME_SET_FRAME_NUMBER,
	GAME_SOUND_CALLBACK,
	GAMEVP_PROFSTRUCT,
	GAMEVP_GENTITIES,
	GAMEV_GENTITYSIZE,
	GAMEV_NUM_ENTITIES,
	GAMEV_MAX_ENTITIES,
	GAMEVP_ERRORMESSAGE
};

// these import messages do not have an exact analogue in MOHAA
enum {
	G_CVAR_REGISTER = -100,
	G_CVAR_VARIABLE_STRING_BUFFER,
	G_CVAR_VARIABLE_INTEGER_VALUE,
};

#endif // __QMM2_GAME_MOHAA_H__
