/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAME_SOF2SP_H
#define QMM2_GAME_SOF2SP_H

#include <cstdint>

typedef int fileHandle_t;
typedef enum { FS_READ, FS_WRITE, FS_APPEND, FS_APPEND_SYNC,
               FS_READ_TEXT, FS_WRITE_TEXT, FS_APPEND_TEXT, FS_APPEND_SYNC_TEXT, } fsMode_t;
typedef enum { EXEC_NOW, EXEC_INSERT, EXEC_APPEND, } cbufExec_t;

typedef enum { qfalse, qtrue }	qboolean;
typedef struct gentity_s gentity_t;
typedef struct playerState_s playerState_t;
typedef struct dtiki_s dtiki_t;
typedef float vec3_t[3];

typedef struct cvar_s {
    int unknown1;
    int unknown2;
    int unknown3;
    int unknown4;
    int unknown5;
    int padding[12];
    char* string;
    char* resetString;
    int unknown6;
    int			flags;

    qboolean	modified;			// set each time the cvar is changed
    int			modificationCount;	// incremented each time the cvar is changed
    float		value;				// atof( string )
    int			integer;			// atoi( string )
    struct cvar_s* next;
} cvar_t;

#define	MAX_CVAR_VALUE_STRING	256
typedef int	cvarHandle_t; 
typedef struct {
    cvarHandle_t	handle;
    int			modificationCount;
    float		value;
    int			integer;
    char		string[MAX_CVAR_VALUE_STRING];
    char		latchedString[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

struct game_import_t {
	void(*Printf)(const char* fmt, ...);
	void(*DPrintf)(const char* fmt, ...);
	void(*DPrintf2)(const char* fmt, ...);
    void(*snprintf)(char* destination, int size, const char* fmt, ...);
    void(*ErrorF)(int code, const char* fmt, ...);
    int(*FS_FOpenFile)(const char* qpath, fileHandle_t* f, fsMode_t fsMode, qboolean quiet);
    int(*FS_Read)(char* buffer, int size, fileHandle_t f);
    int(*FS_Write)(char* buffer, int size, fileHandle_t f);
    int(*FS_FCloseFile)(fileHandle_t f);
    void(*FS_ReadFile)(char* qpath, char** buffer, qboolean quiet);
    void(*FS_FreeFile)(fileHandle_t f);
    qboolean(*FS_FileAvailable)(const char* file);
    int (*FS_ListFiles)(const char* filename, const char* extension, fileHandle_t* file, qboolean uniqueFILE);
    void(*FS_FreeFileList)(char** filelist);
    intptr_t(*unknown14)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    int (*Milliseconds)();
    intptr_t(*unknown16)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown17)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    void(*SendConsoleCommand)(const char* exec);
    void(*ExecuteConsoleCommand)(int exec_when, const char* exec);
    int(*Argc)();
    void(*Argv)(int argn, char* buffer, int size);
    const char* (*Args)();
    qboolean(*Cvar_IsModified)(const char* var_name, const char* var_value, int flags);
    void(*Cvar_Register)(vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags);
    void(*Cvar_Update)(vmCvar_t* vmCvar);
    cvar_t* (*Cvar_Set)(const char* var_name, const char* value, int flags);
    cvar_t* (*Cvar_Get)(const char* var_name, const char* var_value, int flags, qboolean setModified);
    void(*Cvar_SetValue)(const char* var_name, float value, int flags);
    int(*Cvar_VariableIntegerValue)(const char* var_name);
    int(*Cvar_VariableFloatValue)(const char* var_name);
    void(*Cvar_VariableStringBuffer)(const char* var_name, char* buffer, int size);
    void* (*Malloc)(int iSize, int eTag, qboolean bZeroIt);
    void(*Free)(void* buf);
    intptr_t(*unknown34)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    void* (*CM_RegisterTerrain)(const char* config, bool server);
    intptr_t(*unknown36)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown37)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown38)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown39)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown40)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown41)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown42)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown43)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown44)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown45)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown46)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown47)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown48)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown49)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown50)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown51)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown52)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown53)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    void(*LocateGameData)(gentity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient);
    intptr_t(*unknown55)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown56)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown57)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown58)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown59)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown60)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown61)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown62)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown63)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown64)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown65)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown66)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t unknown67;
    intptr_t unknown68;
    intptr_t(*unknown69)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown70)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown71)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown72)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown73)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown74)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown75)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown76)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    void(*SetConfigstring)(int index, const char* configstring);
    void(*GetConfigstring)(int index, char* buffer, int size);
    void(*GetServerInfo)(char* buffer, int size);
    void (*AdjustAreaPortalState)(gentity_t* ent, qboolean open);
    intptr_t(*unknown81)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown82)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown83)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown84)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    dtiki_t* (*TIKI_RegisterModel)(const char* path);
    intptr_t(*unknown86)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown87)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown88)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown89)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown90)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown91)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown92)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown93)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown94)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown95)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    qboolean(*GetEntityToken)(char* buffer, int bufferSize);
    intptr_t(*unknown97)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown98)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown99)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown100)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown101)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown102)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown103)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown104)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown105)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown106)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown107)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown108)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown109)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    intptr_t(*unknown110)(intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14, intptr_t arg15, intptr_t arg16);
    void (*CM_TM_Upload)(vec3_t player_origin, vec3_t player_angles);
    void (*SaveTerrainImageToDisk)(const char* terrainName, const char* missionName, const char* seed);
};

// import ("syscall") cmds
enum {
	G_PRINTF,
    G_DPRINTF,
    G_DPRINTF2,
    G_SNPRINTF,
    G_ERRORF,
    G_FS_FOPEN_FILE,
    G_FS_READ,
    G_FS_WRITE,
    G_FS_FCLOSE_FILE,
    G_FS_READFILE,
    G_FS_FREEFILE,
    G_FS_FILEAVAILABLE,
    G_FS_LISTFILES,
    G_FS_FREEFILELIST,
    G_UNKNOWN14,
    G_MILLISECONDS,
    G_UNKNOWN16,
    G_UNKNOWN17,
    G_SEND_CONSOLE_COMMAND,
    G_EXECUTE_CONSOLE_COMMAND,
    G_ARGC,
    G_ARGV,
    G_ARGS,
    G_CVAR_ISMODIFIED,
    G_CVAR_REGISTER,
    G_CVAR_UPDATE,
    G_CVAR_SET,
    G_CVAR_GET,
    G_CVAR_SETVALUE,
    G_CVAR_VARIABLE_INTEGER_VALUE,
    G_CVAR_VARIABLE_FLOAT_VALUE,
    G_CVAR_VARIABLE_STRING_BUFFER,
    G_MALLOC,
    G_FREE,
    G_UNKNOWN34,
    G_CM_REGISTERTERRAIN,
    G_UNKNOWN36,
    G_UNKNOWN37,
    G_UNKNOWN38,
    G_UNKNOWN39,
    G_UNKNOWN40,
    G_UNKNOWN41,
    G_UNKNOWN42,
    G_UNKNOWN43,
    G_UNKNOWN44,
    G_UNKNOWN45,
    G_UNKNOWN46,
    G_UNKNOWN47,
    G_UNKNOWN48,
    G_UNKNOWN49,
    G_UNKNOWN50,
    G_UNKNOWN51,
    G_UNKNOWN52,
    G_UNKNOWN53,
    G_LOCATE_GAME_DATA,
    G_UNKNOWN55,
    G_UNKNOWN56,
    G_UNKNOWN57,
    G_UNKNOWN58,
    G_UNKNOWN59,
    G_UNKNOWN60,
    G_UNKNOWN61,
    G_UNKNOWN62,
    G_UNKNOWN63,
    G_UNKNOWN64,
    G_UNKNOWN65,
    G_UNKNOWN66,
    GV_UNKNOWN67,
    GV_UNKNOWN68,
    G_UNKNOWN69,
    G_UNKNOWN70,
    G_UNKNOWN71,
    G_UNKNOWN72,
    G_UNKNOWN73,
    G_UNKNOWN74,
    G_UNKNOWN75,
    G_UNKNOWN76,
    G_SET_CONFIGSTRING,
    G_GET_CONFIGSTRING,
    G_GET_SERVERINFO,
    G_ADJUSTAREAPORTALSTATE = 80,
    G_UNKNOWN81,
    G_UNKNOWN82,
    G_UNKNOWN83,
    G_UNKNOWN84,
    G_TIKI_REGISTERMODEL = 85,
    G_UNKNOWN86,
    G_UNKNOWN87,
    G_UNKNOWN88,
    G_UNKNOWN89,
    G_UNKNOWN90,
    G_UNKNOWN91,
    G_UNKNOWN92,
    G_UNKNOWN93,
    G_UNKNOWN94,
    G_UNKNOWN95,
    G_GET_ENTITY_TOKEN,
    G_UNKNOWN97,
    G_UNKNOWN98,
    G_UNKNOWN99,
    G_UNKNOWN100,
    G_UNKNOWN101,
    G_UNKNOWN102,
    G_UNKNOWN103,
    G_UNKNOWN104,
    G_UNKNOWN105,
    G_UNKNOWN106,
    G_UNKNOWN107,
    G_UNKNOWN108,
    G_UNKNOWN109,
    G_UNKNOWN110,
    G_CM_TM_UPLOAD,
    G_SAVETERRAINIMAGETODISK,

    G_PRINT = G_PRINTF,
	G_FS_GETFILELIST = G_FS_LISTFILES,
};

struct game_export_t {
	void(*Init)(int levelTime, int randomSeed, int restart);
	void(*Shutdown)(intptr_t arg0);
    qboolean(*ClientConnect)(int clientNum);
	void(*ClientBegin)(int clientNum);
	void(*ClientDisconnect)(int clientNum);
	void(*ClientCommand)(int ClientNum);
	void(*ClientThink)(int clientNum);
	void(*RunFrame)(int levelTime, int frameTime);
	intptr_t(*IsClientActive)();
	intptr_t(*ConsoleCommand)();
	intptr_t unknown10;
	intptr_t(*SpawnRMGEntity)(char* spawnent);
	void(*arioche)(intptr_t arg0);
	void(*EntityList)();
	intptr_t(*WriteLevel)();
	intptr_t unknown15;
	intptr_t(*unknown16)(intptr_t arg0);
	int(*Save)();
    qboolean(*GameAllowedToSaveHere)();
	qboolean(*CanPlayCinematic)();
	intptr_t(*unknown20)(...);
    intptr_t(*unknown21)(...);
    intptr_t(*unknown22)(...);
    intptr_t(*unknown23)(...);
    intptr_t(*unknown24)(...);
    intptr_t(*unknown25)(...);
};

// export ("vmMain") cmds
enum {
	GAME_INIT,
	GAME_SHUTDOWN,
	GAME_CLIENT_CONNECT,
	GAME_CLIENT_BEGIN,
	GAME_CLIENT_DISCONNECT,
	GAME_CLIENT_COMMAND,
	GAME_CLIENT_THINK,
	GAME_RUN_FRAME,
    GAME_IS_CLIENT_ACTIVE,
	GAME_CONSOLE_COMMAND,
	GAMEV_UNKNOWN10,
	GAME_SPAWN_RMG_ENTITY,
	GAME_ARIOCHE,
	GAME_ENTITY_LIST,
	GAME_WRITE_LEVEL,
	GAMEV_UNKNOWN15,
	GAME_UNKNOWN16,
	GAME_SAVE,
	GAME_GAMEALLOWEDTOSAVEHERE,
	GAME_CAN_PLAY_CINEMATIC,
	GAME_UNKNOWN20,
	GAME_UNKNOWN21,
	GAME_UNKNOWN22,
	GAME_UNKNOWN23,
	GAME_UNKNOWN24,
	GAME_UNKNOWN25,
};

// these import messages do not have an exact analogue in SOF2SP (yet?)
enum {
    G_ERROR = -100,                     // void (const char* msg)
};

#define	CVAR_ARCHIVE		0x00000001
#define	CVAR_USERINFO		0x00000002
#define	CVAR_SERVERINFO		0 //0x00000004
#define	CVAR_SYSTEMINFO		0x00000008
#define	CVAR_INIT			0x00000010
#define	CVAR_LATCH			0x00000020
#define	CVAR_ROM			0x00000040
#define	CVAR_USER_CREATED	0x00000080
#define	CVAR_TEMP			0x00000100
#define CVAR_CHEAT			0x00000200
#define CVAR_NORESTART		0x00000400
#define CVAR_INTERNAL		0x00000800
#define	CVAR_PARENTAL		0x00001000
#define CVAR_LOCK_RANGE		0x00002000

#endif // QMM2_GAME_SOF2SP_H
