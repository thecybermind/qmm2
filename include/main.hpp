/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_MAIN_HPP
#define QMM2_MAIN_HPP

#include <cstdint>  // intptr_t
#include "qmmapi.h" // C_DLLEXPORT

/**
* @brief Entry point: engine->qmm
*
* This is the first function called when a vmMain DLL is loaded. The address of the engine's syscall callback is given,
* but it is not guaranteed to be initialized and ready for calling until vmMain() is called later. For now, all we can
* do is store the syscall, load the config file, and attempt to figure out what game engine we are in. This is either
* determined by the config file, or by getting the filename of the QMM DLL itself.
*
* @param syscall Pointer to engine's syscall function
*/
C_DLLEXPORT void dllEntry(eng_syscall syscall);

/**
* @brief Entry point: engine->qmm
*
* This is the first function called when a GetGameAPI DLL is loaded. This system is based on the API model used by
* Quake 2, where a struct of function pointers is given from the engine to the mod, and the mod returns a struct of
* function pointers back to the engine.
* To best integrate this with QMM, game_xyz.cpp/.h create an enum for each import (syscall) and export (vmMain)
* function/variable.
* A game_export_t is given to the engine which has lambdas for each function pointer that calls QMM's
* vmMain_args(enum, args).
* A game_import_t is given to the mod which has lambdas for each function pointer that calls QMM's
* syscall_args(enum, args).
*
* The original import/export tables are stored. When QMM and plugins need to call the mod or engine,
* QMM::game->vmMain_args or QMM::game->syscall_args point to game-specific functions which will take the cmd, and
* route to the proper function pointer in the struct.
*
* The SOF2SP engine passes an apiversion as the first arg, and import is the second arg.
*
* @param import Pointer to engine's import function table
* @param extra Optional argument in some engines
* @return Pointer to hooked export function table
*/
C_DLLEXPORT void* GetGameAPI(void* import, void* extra);

/**
* @brief Entry point: engine->qmm
* 
* This is the same as the 2-arg GetGameAPI used by SOF2SP but OpenJK renamed it.
*
* @param apiversion Engine's API version
* @param import Pointer to engine's import function table
* @return Pointer to hooked export function table
*/
C_DLLEXPORT void* GetModuleAPI(int apiversion, void* import);

#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
/**
* @brief Entry point: engine->qmm
*
* Quake 2 Remastered has a heavily modified engine + API, and includes Game and CGame in the same DLL, with this GetCGameAPI as
* the entry point. This is the first function called when a Q2R mod DLL is loaded. The only thing the engine does at first,
* however, is get the export struct returned from this function, and then calls export->Init() and export->Shutdown(). After
* this, the engine unloads the mod (QMM). Then, it loads it again as a server DLL through GetGameAPI, and then when a game is
* started, it calls GetCGameAPI to load the DLL as a client. Since at this point, QMM has everything loaded already, it just
* grabs the function from g_mod.dll and gets the GetCGameAPI function easily.
*
* Since QMM does not care about CGame, we check if QMM has already loaded things. If it has, QMM has already loaded
* the mod DLL so we can easily route to the mod's GetCGameAPI function. If not, we load the mod DLL and find GetCGameAPI,
* pass the import pointer to it, and return the mod's export pointer. If QMM has not loaded things, then we have 2 situations:
* this is the initial load at startup where just Init() and Shutdown() are called, or the player is joining a remote server to
* play. Either way, QMM won't be loaded at all during this load so just do a quick DLL load, with no config, no logfile, etc.
*
* @param import Pointer to engine's CGame import function table
* @return Pointer to CGame export function table
*/
C_DLLEXPORT void* GetCGameAPI(void* import);
#endif // QMM_OS_WINDOWS && QMM_ARCH_64

/**
* @brief Entry point: engine->qmm
*
* This is the "vmMain" function called by the engine as an entry point into the mod. First thing, we check if the
* cgame system has stored a syscall pointer. If it has, it means that this is a call for the cgame system: the
* game_import_t struct returned to the engine is filled with lambdas that call QMM::vmMain_args. Next, we check if
* the game engine could not be determined. If so, we fail with G_ERROR and tell the user to set the game in the
* config file. If the engine was determined, routes the function call to QMM::vmMain_args which subsequently calls
* plugins and the mod.
* 
* For GetGameAPI games, the functions in the game_import_t struct passed to the engine call QMM::vmMain_args.
*
* The events that QMM listens for to perform some tasks:
* 
* GAME_INIT (pre): load mod file, load plugins, and optionally execute a cfg file
* 
* GAME_CONSOLE_COMMAND (pre): handle "qmm" server command
* 
* GAME_SHUTDOWN (post): unload plugins and mod file
*
* @param cmd Mod function to perform
* @param ... cmd-specific arguments
* @return Return value of mod call
*/
C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...);

/**
* @brief Entry point: mod->qmm
*
* This is the "syscall" function called by the mod as a way to pass info to or get info from the engine.
* It routes the function call to QMM::syscall_args which subsequently calls plugins and the engine.
* 
* For GetGameAPI games, the functions in the game_export_t struct returned to the mod call QMM::syscall_args.
* 
* Named qmm_syscall to avoid conflict with POSIX syscall function.
*
* @param cmd Engine function to perform
* @param ... cmd-specific arguments
* @return Return value of engine call
*/
intptr_t qmm_syscall(intptr_t cmd, ...);

#endif // QMM2_MAIN_HPP
