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

#include "version.h"

#if defined(QMM_OS_WINDOWS)

 #define WIN32_LEAN_AND_MEAN
 #ifndef _CRT_SECURE_NO_WARNINGS
  #define _CRT_SECURE_NO_WARNINGS
 #endif
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
 #include <direct.h>
 #include <shellapi.h>

 #if defined(QMM_ARCH_64)
  #define SUF_DLL "x86_64"
 #elif defined(QMM_ARCH_32)
  #define SUF_DLL "x86"
 #else
  #error Unknown architecture?
 #endif

 #define EXT_DLL "dll"
 #define EXT_QVM "qvm"

 #define PATH_MAX			4096
 #define strcasecmp			_stricmp
 #define dlopen(file, x)		((void*)LoadLibrary(file))
 #define dlsym(dll, func)	((void*)GetProcAddress((HMODULE)(dll), (func)))
 #define dlclose(dll)		FreeLibrary((HMODULE)(dll))
 #define mkdir(path, x)		_mkdir(path)
 #define osdef_get_milliseconds GetTickCount64
 const char* dlerror();		// this will return the last error from any win32 function, not just library functions

#elif defined(QMM_OS_LINUX)

 #include <dlfcn.h>
 #include <unistd.h> 
 #include <limits.h>
 #include <ctype.h>
 #include <stdint.h>
 #include <sys/time.h>
 #include <sys/stat.h>
 #include <sys/types.h>

 #if defined(QMM_ARCH_64)
  #define SUF_DLL "x86_64"
 #elif defined(QMM_ARCH_32)
  #define SUF_DLL "i386"
 #else
  #error Unknown architecture?
 #endif

 #define EXT_DLL "so"
 #define EXT_QVM "qvm"
 uint64_t osdef_get_milliseconds();
 void MessageBoxA(void* handle, const char* message, const char* title, int flags);

#else // !QMM_OS_WINDOWS && !QMM_OS_LINUX

 #error Unknown OS?

#endif

#define MOD_DLL SUF_DLL "." EXT_DLL

void* osdef_path_get_qmm_handle();
const char* osdef_path_get_qmm_path();
const char* osdef_path_get_proc_path();

#endif // QMM2_OSDEF_H
