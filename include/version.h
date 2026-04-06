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

// Evaluate and stringify a macro
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

// Major semver component
#define QMM_VERSION_MAJOR	2
// Minor semver component
#define QMM_VERSION_MINOR	6
// Revision semver component
#define QMM_VERSION_REV		0

// String of dotted version number
#define QMM_VERSION		STRINGIFY(QMM_VERSION_MAJOR) "." STRINGIFY(QMM_VERSION_MINOR) "." STRINGIFY(QMM_VERSION_REV)

// When was the DLL built? Uses __TIME__ and __DATE__
#define QMM_COMPILE		__TIME__ " " __DATE__
// Name of the builder
#define QMM_BUILDER		"Kevin Masterson"
// QMM URL
#define QMM_URL         "https://github.com/thecybermind/qmm2/"

#if defined(_WIN32)
 #define QMM_OS			"Windows"
 #define QMM_OS_WINDOWS // Windows
 #ifdef _WIN64
  #define QMM_ARCH      "x86_64"
  #define QMM_ARCH_64   // x86-64
 #else
  #define QMM_ARCH      "x86"
  #define QMM_ARCH_32   // x86
 #endif
#elif defined(__linux__)
 #define QMM_OS			"Linux"
 #define QMM_OS_LINUX   // Linux
 #ifdef __LP64__
  #define QMM_ARCH      "x86_64"
  #define QMM_ARCH_64   // x86-64
 #else
  #define QMM_ARCH      "x86"
  #define QMM_ARCH_32   // x86
 #endif
#else
 #error Unknown OS
#endif

// Comma-separated DWORD form of version for qmm2.rc
#define QMM_VERSION_DWORD	QMM_VERSION_MAJOR , QMM_VERSION_MINOR , QMM_VERSION_REV , 0

#endif // QMM2_VERSION_H
