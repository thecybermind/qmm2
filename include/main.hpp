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
* @brief Engine->Mod entrypoint for dllEntry/vmMain games.
*
* This is the first function called when a vmMain DLL is loaded. It is only used to pass the mod the engine's "syscall"
* callback pointer. This passes the argument onto QMM::HandleEntry to detect the environment, start logging, load the
* config, detect the engine, and call the game-specific Entry function.
*
* @param syscall Pointer to engine's syscall function
*/
C_DLLEXPORT void dllEntry(eng_syscall syscall);

/**
* @brief Engine->Mod entrypoint for GetGameAPI games.
* 
* This function receives the engine's import callback struct, and returns the mod's export callback struct.
* 
* This passes the arguments onto (and returns the value from) QMM::HandleEntry to detect the environment, start logging,
* load the config, detect the engine, and call the game-specific Entry function.
*
* The SOF2SP engine passes an apiversion as the first arg, and import is the second arg.
*
* @param import Pointer to engine's import function table
* @param extra Optional argument in some engines
* @return Pointer to hooked export function table
*/
C_DLLEXPORT void* GetGameAPI(void* import, void* extra);

/**
* @brief Engine->Mod entrypoint for OpenJK engine.
* 
* This functions the same as the 2-arg GetGameAPI used by SOF2SP but OpenJK renamed it.
*
* @param apiversion Engine's API version
* @param import Pointer to engine's import function table
* @return Pointer to hooked export function table
*/
C_DLLEXPORT void* GetModuleAPI(int apiversion, void* import);

#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
/**
* @brief Engine->Mod entrypoint for Quake 2 Remastered client logic.
*
* If Q2R loads the client, it will always be after the server has been loaded, or it won't load the server at all. So if this
* is called while QMM has not already loaded the server-side, we let the Q2RC game logic handle a very minor hooking of just
* Init() and Shutdown(). All other export calls are passed through directly, and imports are not hooked at all (this import
* pointer is passed directly to the mod's GetCGameAPI function).
*
* @param import Pointer to engine's CGame import function table
* @return Pointer to CGame export function table
*/
C_DLLEXPORT void* GetCGameAPI(void* import);
#endif // QMM_OS_WINDOWS && QMM_ARCH_64

/**
* @brief Engine->Mod callback for dllEntry/vmMain games.
* 
* This just grabs varargs, checks if this is being called for the cgame, and then passes the call onto QMM::vmMain_args
* which subsequently calls plugins and the mod.
* 
* @param cmd Mod function to perform
* @param ... cmd-specific arguments
* @return Return value of mod call
*/
C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...);

/**
* @brief Mod->Engine callback for dllEntry/vmMain games.
*
* This just grabs varargs and then passes the call onto QMM::syscall_args which subsequently calls plugins and the mod.
* 
* Since this isn't an exported function, it is named qmm_syscall to avoid conflict with the POSIX syscall function.
*
* @param cmd Engine function to perform
* @param ... cmd-specific arguments
* @return Return value of engine call
*/
intptr_t qmm_syscall(intptr_t cmd, ...);

#endif // QMM2_MAIN_HPP
