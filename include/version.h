/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_VERSION_H
#define QMM2_VERSION_H

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define QMM_VERSION_MAJOR	2
#define QMM_VERSION_MINOR	4
#define QMM_VERSION_REV		7

#define QMM_VERSION		STRINGIFY(QMM_VERSION_MAJOR) "." STRINGIFY(QMM_VERSION_MINOR) "." STRINGIFY(QMM_VERSION_REV)

#define QMM_COMPILE		__TIME__ " " __DATE__
#define QMM_BUILDER		"Kevin Masterson"
#define QMM_URL         "https://github.com/thecybermind/qmm2/"

#if defined(_WIN32)
 #define QMM_OS			"Windows"
 #ifdef _WIN64
  #define QMM_ARCH      "x86_64"
 #else
  #define QMM_ARCH      "x86"
 #endif
#elif defined(__linux__)
 #define QMM_OS			"Linux"
 #ifdef __LP64__
  #define QMM_ARCH      "x86_64"
 #else
  #define QMM_ARCH      "x86"
 #endif
#endif

#define QMM_VERSION_DWORD	QMM_VERSION_MAJOR , QMM_VERSION_MINOR , QMM_VERSION_REV , 0

#endif // QMM2_VERSION_H
