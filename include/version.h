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

#include "osdef.h"

// Evaluate and stringify a macro
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

// Major semver component
#define QMM_VERSION_MAJOR	2
// Minor semver component
#define QMM_VERSION_MINOR	6
// Revision semver component
#define QMM_VERSION_REV		1

// String of dotted version number
#define QMM_VERSION		STRINGIFY(QMM_VERSION_MAJOR) "." STRINGIFY(QMM_VERSION_MINOR) "." STRINGIFY(QMM_VERSION_REV)

// When was the DLL built? Uses __TIME__ and __DATE__
#define QMM_COMPILE		__TIME__ " " __DATE__
// Name of the builder
#define QMM_BUILDER		"Kevin Masterson"
// QMM URL
#define QMM_URL         "https://github.com/thecybermind/qmm2/"

#if defined(QMM_OS_WINDOWS)
 #define QMM_OS			"Windows"
#elif defined(QMM_OS_LINUX)
 #define QMM_OS			"Linux"
#else
 #error Unknown OS
#endif

#if defined(QMM_ARCH_64)
 #define QMM_ARCH "x86_64"
#elif defined(QMM_ARCH_32)
 #define QMM_ARCH "x86"
#else
 #error Unknown Arch
#endif

// Comma-separated DWORD form of version for qmm2.rc
#define QMM_VERSION_DWORD	QMM_VERSION_MAJOR , QMM_VERSION_MINOR , QMM_VERSION_REV , 0

#endif // QMM2_VERSION_H
