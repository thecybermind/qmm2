/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <codmp/game/g_public.h>

#include "game_api.h"
#include "log.h"
// QMM-specific CODMP header
#include "game_codmp.h"
#include "main.h"
#include "mod.h"

// GAME_GET_APIVERSION gets called first, which is when QMM has to perform mod/plugin loading, but we
// don't want to make plugins have to use separate code to handle the actual GAME_INIT message, so just
// do a dirty redefine here for the QMM_GAME_INIT message definition
#define GAME_INIT GAME_GET_APIVERSION
GEN_QMM_MSGS(CODMP);
#undef GAME_INIT
GEN_EXTS(CODMP);

// a copy of the original syscall pointer that comes from the game engine
static eng_syscall_t orig_syscall = nullptr;

// mod vmMain is access via g_mod.pfnvmMain


// wrapper syscall function that calls actual engine func from orig_import
// this is how QMM and plugins will call into the engine
intptr_t CODMP_syscall(intptr_t cmd, ...) {
	QMM_GET_SYSCALL_ARGS();

#ifdef _DEBUG
	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("CODMP_syscall({} {}) called\n", CODMP_eng_msg_names(cmd), cmd);
#endif

	intptr_t ret = 0;

	switch (cmd) {
		// handle special cmds which QMM uses but CODMP doesn't have an analogue for
		case G_ARGS: {
			// quake2: char* (*args)(void);
			static std::string s;
			static char buf[MAX_STRING_CHARS];
			s = "";
			int i = 1;
			while (i < orig_syscall(G_ARGC)) {
				orig_syscall(G_ARGV, buf, sizeof(buf));
				buf[sizeof(buf) - 1] = '\0';
				if (i != 1)
					s += " ";
				s += buf;
			}
			ret = (intptr_t)s.c_str();
			break;
		}
		// all normal engine functions go to engine
		default:
			ret = orig_syscall(cmd, QMM_PUT_SYSCALL_ARGS());
	}

	// do anything that needs to be done after function call here

#ifdef _DEBUG
	if (cmd != G_PRINT)
		LOG(QMM_LOG_TRACE, "QMM") << fmt::format("CODMP_syscall({} {}) returning {}\n", CODMP_eng_msg_names(cmd), cmd, ret);
#endif

	return ret;
}


// wrapper vmMain function that calls actual mod func from orig_export
// this is how QMM and plugins will call into the mod
intptr_t CODMP_vmMain(intptr_t cmd, ...) {
	QMM_GET_VMMAIN_ARGS();

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_vmMain({} {}) called\n", CODMP_mod_msg_names(cmd), cmd);

	if (!g_mod.pfnvmMain)
		return 0;

	// store return value since we do some stuff after the function call is over
	intptr_t ret = 0;

	// all normal mod functions go to mod
	ret = g_mod.pfnvmMain(cmd, QMM_PUT_VMMAIN_ARGS());

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_vmMain({} {}) returning {}\n", CODMP_mod_msg_names(cmd), cmd, ret);

	return ret;
}


void CODMP_dllEntry(eng_syscall_t syscall) {
	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_dllEntry({}) called\n", (void*)syscall);

	// store original syscall from engine
	orig_syscall = syscall;

	// pointer to wrapper vmMain function that calls actual mod vmMain func g_mod.pfnvmMain
	g_gameinfo.pfnvmMain = CODMP_vmMain;

	// pointer to wrapper syscall function that calls actual engine syscall func
	g_gameinfo.pfnsyscall = CODMP_syscall;

	LOG(QMM_LOG_DEBUG, "QMM") << fmt::format("CODMP_dllEntry({}) returning\n", (void*)syscall);
}


const char* CODMP_eng_msg_names(intptr_t cmd) {
	switch (cmd) {
		GEN_CASE(G_PRINTF);
		GEN_CASE(G_ERROR);
		GEN_CASE(G_ERROR_LOCALIZED);
		GEN_CASE(G_MILLISECONDS);
		GEN_CASE(G_CVAR_REGISTER);
		GEN_CASE(G_CVAR_UPDATE);
		GEN_CASE(G_CVAR_SET);
		GEN_CASE(G_CVAR_VARIABLE_INTEGER_VALUE);
		GEN_CASE(G_CVAR_VARIABLE_VALUE);
		GEN_CASE(G_CVAR_VARIABLE_STRING_BUFFER);
		GEN_CASE(G_ARGC);
		GEN_CASE(G_ARGV);
		GEN_CASE(G_HUNK_ALLOCINTERNAL);
		GEN_CASE(G_HUNK_ALLOCLOWINTERNAL);
		GEN_CASE(G_HUNK_ALLOCALIGNINTERNAL);
		GEN_CASE(G_HUNK_ALLOCLOWALIGNINTERNAL);
		GEN_CASE(G_HUNK_ALLOCATETEMPMEMORYINTERNAL);
		GEN_CASE(G_HUNK_FREETEMPMEMORYINTERNAL);
		GEN_CASE(G_FS_FOPEN_FILE);
		GEN_CASE(G_FS_READ);
		GEN_CASE(G_FS_WRITE);
		GEN_CASE(G_FS_RENAME);
		GEN_CASE(G_FS_FCLOSE_FILE);
		GEN_CASE(G_SEND_CONSOLE_COMMAND);
		GEN_CASE(G_LOCATE_GAME_DATA);
		GEN_CASE(G_GET_GUID);
		GEN_CASE(G_DROP_CLIENT);
		GEN_CASE(G_SEND_SERVER_COMMAND);
		GEN_CASE(G_SET_CONFIGSTRING);
		GEN_CASE(G_GET_CONFIGSTRING);
		GEN_CASE(G_GET_CONFIGSTRING_CONST);
		GEN_CASE(G_ISLOCALCLIENT);
		GEN_CASE(G_GET_CLIENT_PING);
		GEN_CASE(G_GET_USERINFO);
		GEN_CASE(G_SET_USERINFO);
		GEN_CASE(G_GET_SERVERINFO);
		GEN_CASE(G_SET_BRUSH_MODEL);
		GEN_CASE(G_TRACE);
		GEN_CASE(G_TRACECAPSULE);
		GEN_CASE(G_SIGHTTRACE);
		GEN_CASE(G_SIGHTTRACE_CAPSULE);
		GEN_CASE(G_SIGHTTRACEENTITY);
		GEN_CASE(G_CM_BOXTRACE);
		GEN_CASE(G_CM_CAPSULETRACE);
		GEN_CASE(G_CM_BOXSIGHTTRACE);
		GEN_CASE(G_CM_CAPSULESIGHTTRACE);
		GEN_CASE(G_LOCATIONAL_TRACE);
		GEN_CASE(G_POINT_CONTENTS);
		GEN_CASE(G_IN_PVS);
		GEN_CASE(G_IN_PVS_IGNORE_PORTALS);
		GEN_CASE(G_IN_SNAPSHOT);
		GEN_CASE(G_ADJUST_AREA_PORTAL_STATE);
		GEN_CASE(G_AREAS_CONNECTED);
		GEN_CASE(G_LINKENTITY);
		GEN_CASE(G_UNLINKENTITY);
		GEN_CASE(G_ENTITIES_IN_BOX);
		GEN_CASE(G_ENTITY_CONTACT);
		GEN_CASE(G_GET_USERCMD);
		GEN_CASE(G_GET_ENTITY_TOKEN);
		GEN_CASE(G_FS_GETFILELIST);
		GEN_CASE(G_MAPEXISTS);
		GEN_CASE(G_REAL_TIME);
		GEN_CASE(G_SNAPVECTOR);
		GEN_CASE(G_ENTITY_CONTACTCAPSULE);
		GEN_CASE(G_SOUNDALIASSTRING);
		GEN_CASE(G_PICKSOUNDALIAS);
		GEN_CASE(G_SOUNDALIASINDEX);
		GEN_CASE(G_SURFACETYPEFROMNAME);
		GEN_CASE(G_SURFACETYPETONAME);
		GEN_CASE(G_ADDTESTCLIENT);
		GEN_CASE(G_ARCHIVED_CLIENTINFO);
		GEN_CASE(G_ADDDEBUGSTRING);
		GEN_CASE(G_ADDDEBUGLINE);
		GEN_CASE(G_SET_ARCHIVE);
		GEN_CASE(G_ZMALLOC_INTERNAL);
		GEN_CASE(G_ZFREE_INTERNAL);
		GEN_CASE(G_XANIM_CREATETREE);
		GEN_CASE(G_XANIM_CREATESMALLTREE);
		GEN_CASE(G_XANIM_FREESMALLTREE);
		GEN_CASE(G_XMODELEXISTS);
		GEN_CASE(G_XMODELGET);
		GEN_CASE(G_DOBJ_CREATE);
		GEN_CASE(G_DOBJ_EXISTS);
		GEN_CASE(G_DOBJ_SAFEFREE);
		GEN_CASE(G_XANIM_GETANIMS);
		GEN_CASE(G_XANIM_CLEARTREEGOALWEIGHTS);
		GEN_CASE(G_XANIM_CLEARGOALWEIGHT);
		GEN_CASE(G_XANIM_CLEARTREEGOALWEIGHTSSTRICT);
		GEN_CASE(G_XANIM_SETCOMPLETEGOALWEIGHTKNOB);
		GEN_CASE(G_XANIM_SETCOMPLETEGOALWEIGHTKNOBALL);
		GEN_CASE(G_XANIM_SETANIMRATE);
		GEN_CASE(G_XANIM_SETTIME);
		GEN_CASE(G_XANIM_SETGOALWEIGHTKNOB);
		GEN_CASE(G_XANIM_CLEARTREE);
		GEN_CASE(G_XANIM_HASTIME);
		GEN_CASE(G_XNAIM_ISPRIMITIVE);
		GEN_CASE(G_XANIM_GETLENGTH);
		GEN_CASE(G_XANIM_GETLENGTHSECONDS);
		GEN_CASE(G_XANIM_SETCOMPLETEGOALWEIGHT);
		GEN_CASE(G_XANIM_SETGOALWEIGHT);
		GEN_CASE(G_XANIM_CALCABSDELTA);
		GEN_CASE(G_XANIM_CALCDELTA);
		GEN_CASE(G_XANIM_GETRELDELTA);
		GEN_CASE(G_XANIM_GETABSDELTA);
		GEN_CASE(G_XANIM_ISLOOPED);
		GEN_CASE(G_XANIM_NOTETRACKEXISTS);
		GEN_CASE(G_XANIM_GETTIME);
		GEN_CASE(G_XANIM_GETWEIGHT);
		GEN_CASE(G_DOBJ_DUMPINFO);
		GEN_CASE(G_DOBJ_CREATESKELFORBONE);
		GEN_CASE(G_DOBJ_CREATESKELFORBONES);
		GEN_CASE(G_DOBJ_UPDATESERVERTIME);
		GEN_CASE(G_DOBJ_INITSERVERTIME);
		GEN_CASE(G_DOBJ_GETHIERARCHYBITS);
		GEN_CASE(G_DOBJ_CALCANIM);
		GEN_CASE(G_DOBJ_CALCSKEL);
		GEN_CASE(G_XANIM_LOADANIMTREE);
		GEN_CASE(G_XANIM_SAVEANIMTREE);
		GEN_CASE(G_XANIM_CLONEANIMTREE);
		GEN_CASE(G_DOBJ_NUMBONES);
		GEN_CASE(G_DOBJ_GETBONEINDEX);
		GEN_CASE(G_DOBJ_GETMATRIXARRAY);
		GEN_CASE(G_DOBJ_DISPLAYANIM);
		GEN_CASE(G_XANIM_HASFINISHED);
		GEN_CASE(G_XANIM_GETNUMCHILDREN);
		GEN_CASE(G_XANIM_GETCHILDAT);
		GEN_CASE(G_XMODEL_NUMBONES);
		GEN_CASE(G_XMODEL_GETBONENAMES);
		GEN_CASE(G_DOBJ_GETROTTRANSARRAY);
		GEN_CASE(G_DOBJ_SETROTTRANSARRAY);
		GEN_CASE(G_DOBJ_SETCONTROLROTTRANSINDEX);
		GEN_CASE(G_DOBJ_GETBOUNDS);
		GEN_CASE(G_XANIM_GETANIMNAME);
		GEN_CASE(G_DOBJ_GETTREE);
		GEN_CASE(G_XANIM_GETANIMTREESIZE);
		GEN_CASE(G_XMODEL_DEBUGBOXES);
		GEN_CASE(G_GETWEAPONINFOMEMORY);
		GEN_CASE(G_FREEWEAPONINFOMEMORY);
		GEN_CASE(G_FREECLIENTSCRIPTPERS);
		GEN_CASE(G_RESETENTITYPARSEPOINT);

		// polyfills
		GEN_CASE(G_ARGS);

		default:
			return "unknown";
	}
}


const char* CODMP_mod_msg_names(intptr_t cmd) {
	switch (cmd) {
		GEN_CASE(GAME_DEFAULT_0);
		GEN_CASE(GAME_GET_APIVERSION);
		GEN_CASE(GAME_INIT);
		GEN_CASE(GAME_SHUTDOWN);
		GEN_CASE(GAME_CLIENT_CONNECT);
		GEN_CASE(GAME_CLIENT_BEGIN);
		GEN_CASE(GAME_CLIENT_USERINFO_CHANGED);
		GEN_CASE(GAME_CLIENT_DISCONNECT);
		GEN_CASE(GAME_CLIENT_COMMAND);
		GEN_CASE(GAME_CLIENT_THINK);
		GEN_CASE(GAME_GET_FOLLOWPLAYERSTATE);
		GEN_CASE(GAME_UPDATE_CVARS);
		GEN_CASE(GAME_RUN_FRAME);
		GEN_CASE(GAME_CONSOLE_COMMAND);
		GEN_CASE(GAME_SCR_FARHOOK);
		GEN_CASE(GAME_DOBJ_CALCPOSE);
		GEN_CASE(GAME_GET_NUMWEAPONS);
		GEN_CASE(GAME_SET_SAVEPERSIST);
		GEN_CASE(GAME_GET_SAVEPERSIST);
		GEN_CASE(GAME_GET_CLIENTSTATE);
		GEN_CASE(GAME_GET_CLIENTARCHIVETIME);
		GEN_CASE(GAME_SET_CLIENTARCHIVETIME);
		GEN_CASE(GAME_GET_CLIENTSCORE);
		GEN_CASE(GAME_GET_FOG_DISTANCE);

		default:
			return "unknown";
	}
}
