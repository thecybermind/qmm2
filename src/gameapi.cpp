/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "version.h"
#include <vector>
#include "gameapi.hpp"

// externs for each game's support objects
GEN_GAME_EXTS(COD11MP);
GEN_GAME_EXTS(CODMP);
GEN_GAME_EXTS(CODUOMP);
GEN_GAME_EXTS(JAMP);
GEN_GAME_EXTS(JK2MP);
GEN_GAME_EXTS(Q3A);
GEN_GAME_EXTS(RTCWMP);
GEN_GAME_EXTS(RTCWSP);
GEN_GAME_EXTS(SOF2MP);
GEN_GAME_EXTS(STVOYHM);
GEN_GAME_EXTS(WET);

GEN_GAME_EXTS(JASP);
GEN_GAME_EXTS(JK2SP);
GEN_GAME_EXTS(MOHAA);
GEN_GAME_EXTS(MOHSH);
GEN_GAME_EXTS(MOHBT);
GEN_GAME_EXTS(Q2R);
GEN_GAME_EXTS(QUAKE2);
GEN_GAME_EXTS(SIN);
GEN_GAME_EXTS(SOF2SP);
GEN_GAME_EXTS(STEF2);
GEN_GAME_EXTS(STVOYSP);

// Table of pointers to GameSupport objects
std::vector<GameSupport*> api_supportedgames = {
	// vmMain games
	GET_GAME_OBJ(Q3A),
	GET_GAME_OBJ(RTCWSP),
	GET_GAME_OBJ(JK2MP),
	GET_GAME_OBJ(JAMP),
	GET_GAME_OBJ(WET),
	GET_GAME_OBJ(RTCWMP),

// these games don't appear to have an official 64-bit version or source port
#if defined(QMM_ARCH_32)
	GET_GAME_OBJ(STVOYHM),
	GET_GAME_OBJ(SOF2MP),
	GET_GAME_OBJ(CODMP),
	GET_GAME_OBJ(CODUOMP),
	// allow a user to choose "COD11MP" manually if they are playing an old version of CoD (no auto-detection)
	GET_GAME_OBJ(COD11MP),
#endif

	// GetGameAPI games
	GET_GAME_OBJ(JK2SP),
	GET_GAME_OBJ(JASP),
	GET_GAME_OBJ(QUAKE2),

// OpenMOHAA adds 64-bit MoH support but the API is very different, so disable it for now
// the rest of the games don't appear to have an official 64-bit version or source port
#if defined(QMM_ARCH_32)
	GET_GAME_OBJ(MOHAA),
	GET_GAME_OBJ(MOHSH),
	GET_GAME_OBJ(MOHBT),
	GET_GAME_OBJ(STEF2),
	GET_GAME_OBJ(SOF2SP),
	GET_GAME_OBJ(STVOYSP),
	GET_GAME_OBJ(SIN),
#endif

// Q2R only exists for 64-bit Windows
#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
	GET_GAME_OBJ(Q2R),
#endif
};


const char* APIType_Name(APIType api) {
	switch (api) {
		GEN_CASE(QMM_API_ERROR);

		GEN_CASE(QMM_API_QVM);

		GEN_CASE(QMM_API_DLLENTRY);
		GEN_CASE(QMM_API_GETGAMEAPI);
		GEN_CASE(QMM_API_GETMODULEAPI);
	default:
		return "unknown";
	};
}


const char* APIType_Function(APIType api) {
	switch (api) {
	case QMM_API_ERROR:
		return "(error)";

	case QMM_API_QVM:
		return "QVM";
	case QMM_API_DLLENTRY:
		return "dllEntry";
	case QMM_API_GETGAMEAPI:
		return "GetGameAPI";
	case QMM_API_GETMODULEAPI:
		return "GetModuleAPI";
	default:
		return "unknown";
	};
}
