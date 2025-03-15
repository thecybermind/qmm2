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
#define _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>
#include <limits.h>
constexpr const char* SUF_DLL = "x86";
constexpr const char* SUF_SO  = "x86";
constexpr const char* EXT_DLL = "dll";
constexpr const char* EXT_SO  = "dll";
constexpr const char* EXT_QVM = "qvm";

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
constexpr const char* SUF_DLL = "i386";
constexpr const char* SUF_SO  = "i386";
constexpr const char* EXT_DLL = "so";
constexpr const char* EXT_SO  = "so";
constexpr const char* EXT_QVM = "qvm";

constexpr const unsigned char MAGIC_DLL[] = { 0x7F,  'E', 'L',  'F' };
constexpr const unsigned char MAGIC_SO[]  = { 0x7F,  'E', 'L',  'F' };
constexpr const unsigned char MAGIC_QVM[] = {  'D', 0x14, 'r', 0x12 };

#define my_vsnprintf	vsnprintf

#else // !_WIN32 && !__linux__

constexpr const char* SUF_DLL = "";
constexpr const char* SUF_SO  = "";
constexpr const char* EXT_DLL = "";
constexpr const char* EXT_SO  = "";
constexpr const char* EXT_QVM = "";

constexpr const unsigned char MAGIC_DLL[] = { 0x00, 0x00, 0x00, 0x00 };
constexpr const unsigned char MAGIC_SO[]  = { 0x00, 0x00, 0x00, 0x00 };
constexpr const unsigned char MAGIC_QVM[] = { 0x00, 0x00, 0x00, 0x00 };

#endif

const char* osdef_path_get_modulepath(void* ptr);
const char* osdef_path_get_procpath();

#endif // __QMM2_OSDEF_H__
