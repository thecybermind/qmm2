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

#define WIN32_LEAN_AND_MEAN
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <windows.h>
#include <limits.h>

#ifdef _WIN64
#define SUF_DLL "x86_64"
#define SUF_SO  "x86_64"
#else
#define SUF_DLL "x86"
#define SUF_SO  "x86"
#endif
#define EXT_DLL "dll"
#define EXT_SO  "dll"
#define EXT_QVM "qvm"

constexpr const unsigned char MAGIC_DLL[] = { 'M',  'Z', 0x90, 0x00 };
constexpr const unsigned char MAGIC_SO[]  = { 'M',  'Z', 0x90, 0x00 };
constexpr const unsigned char MAGIC_QVM[] = { 'D', 0x14,  'r', 0x12 };

#define PATH_MAX			4096
#define my_vsnprintf		_vsnprintf
#define strcasecmp			_stricmp
#define dlopen(file, x)		((void*)LoadLibrary(file))
#define dlsym(dll, func)	((void*)GetProcAddress((HMODULE)(dll), (func)))
#define dlclose(dll)		FreeLibrary((HMODULE)(dll))
char* dlerror();			// this will return the last error from any win32 function, not just library functions

#elif defined(__linux__)

#include <dlfcn.h>
#include <unistd.h> 
#include <limits.h>
#include <ctype.h>

#ifdef __LP64__
#define SUF_DLL "x86_64"
#define SUF_SO  "x86_64"
#else
#define SUF_DLL "i386"
#define SUF_SO  "i386"
#endif
#define EXT_DLL "so"
#define EXT_SO  "so"
#define EXT_QVM "qvm"

constexpr const unsigned char MAGIC_DLL[] = { 0x7F,  'E', 'L',  'F' };
constexpr const unsigned char MAGIC_SO[]  = { 0x7F,  'E', 'L',  'F' };
constexpr const unsigned char MAGIC_QVM[] = {  'D', 0x14, 'r', 0x12 };

#define my_vsnprintf	vsnprintf

#else // !_WIN32 && !__linux__

#define SUF_DLL ""
#define SUF_SO  ""
#define EXT_DLL ""
#define EXT_SO  ""
#define EXT_QVM ""

constexpr const unsigned char MAGIC_DLL[] = { 0x00, 0x00, 0x00, 0x00 };
constexpr const unsigned char MAGIC_SO[]  = { 0x00, 0x00, 0x00, 0x00 };
constexpr const unsigned char MAGIC_QVM[] = { 0x00, 0x00, 0x00, 0x00 };

#endif

void* osdef_path_get_modulehandle(void* ptr);
const char* osdef_path_get_modulepath(void* ptr);
const char* osdef_path_get_procpath();

#endif // __QMM2_OSDEF_H__
