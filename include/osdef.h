/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_OSDEF_H__
#define __QMM2_OSDEF_H__

#ifdef _WIN32
 #ifndef WIN32
  #define WIN32
 #endif
#endif

#ifdef WIN32
 #ifdef linux
  #undef linux
 #endif
#else
 #ifndef linux
  #define linux
 #endif
#endif

#ifdef WIN32
 #define WIN32_LEAN_AND_MEAN
 #define _CRT_SECURE_NO_WARNINGS 1
 #include <windows.h>
 constexpr const char* DLL_SUF = "x86";
 constexpr const char* DLL_EXT = "dll";
 constexpr const char* QVM_EXT = "qvm";
 #define PATH_MAX			4096
 #define my_vsnprintf		_vsnprintf
 #define strcasecmp			_stricmp
 #define dlopen(file, x)	((void*)LoadLibrary(file))
 #define dlsym(dll, func)	((void*)GetProcAddress((HMODULE)(dll), (func)))
 #define dlclose(dll)		FreeLibrary((HMODULE)(dll))
 char* dlerror();			// this returns the last error from any win32 function, not just library functions

 // 'typedef ': ignored on left of '<unnamed-enum>' when no variable is declared
 // found in a lot of the game sdks
 #pragma warning(disable:4091)	
#else // linux
 #include <dlfcn.h>
 #include <limits.h>
 #include <ctype.h>
 constexpr const char* DLL_SUF = "i386";
 constexpr const char* DLL_EXT = "so";
 constexpr const char* QVM_EXT = "qvm";
 #define my_vsnprintf	vsnprintf
#endif

const char* osdef_get_qmm_modulepath();

#endif //__QMM2_OSDEF_H__
