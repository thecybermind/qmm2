/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_VERSION_H__
#define __QMM2_VERSION_H__

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define QMM_VERSION_MAJOR	2
#define QMM_VERSION_MINOR	0
#define QMM_VERSION_REV		0

#define QMM_VERSION		STRINGIFY(QMM_VERSION_MAJOR) "." STRINGIFY(QMM_VERSION_MINOR) "." STRINGIFY(QMM_VERSION_REV)

#define QMM_COMPILE		__TIME__ " " __DATE__
#define QMM_BUILDER		"cybermind"
#define QMM_URL         "https://github.com/thecybermind/qmm2/"

#ifdef _WIN32
 #define QMM_OS			"Win32"
#elif defined(__linux__)
 #define QMM_OS			"Linux"
#else
 #define QMM_OS          "?"
#endif

#define QMM_VERSION_DWORD	QMM_VERSION_MAJOR , QMM_VERSION_MINOR , QMM_VERSION_REV , 0

#endif // __QMM2_VERSION_H__
