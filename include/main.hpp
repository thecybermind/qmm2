/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_MAIN_H
#define QMM2_MAIN_H

#include <cstdint>  // intptr_t
#include "qmmapi.h" // C_DLLEXPORT

C_DLLEXPORT void dllEntry(eng_syscall syscall);
C_DLLEXPORT void* GetGameAPI(void* import, void* extra);
C_DLLEXPORT void* GetModuleAPI(void* import, void* extra);
#if defined(QMM_OS_WINDOWS) && defined(QMM_ARCH_64)
C_DLLEXPORT void* GetCGameAPI(void* import);
#endif // QMM_OS_WINDOWS && QMM_ARCH_64

C_DLLEXPORT intptr_t vmMain(intptr_t cmd, ...);
intptr_t qmm_syscall(intptr_t cmd, ...);

// get a given argument with G_ARGV, based on game engine type
void ArgV(intptr_t argn, char* buf, intptr_t buflen);

#endif // QMM2_MAIN_H
