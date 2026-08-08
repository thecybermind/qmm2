/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_OSDEF_H
#define QMM2_OSDEF_H

#if defined(_WIN32)

 #define QMM_OS_WINDOWS

 #if defined(_WIN64)
  #define QMM_ARCH_64   // x86-64
 #else
  #define QMM_ARCH_32   // x86
 #endif

#elif defined(__linux__)

 #define QMM_OS_LINUX

 #if defined(__LP64__)
  #define QMM_ARCH_64   // x86-64
 #else
  #define QMM_ARCH_32   // x86
 #endif

#endif

#endif // QMM2_OSDEF_H
