/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_UTIL_H
#define QMM2_UTIL_H

#include <vector>
#include <string>
#include <cstddef>		// size_t


// path parsing/handling

std::string path_normalize(std::string path);	// normalize path (remove ".", "..", use all "/", etc)
bool path_is_allowed(std::string path);			// verify path is within exe_dir or qmm_dir
std::string path_dirname(std::string path);		// return directory component of path
std::string path_basename(std::string path);	// return filename component of path
std::string path_baseext(std::string path);		// return filename extension
bool path_is_absolute(std::string path);		// is path absolute? (!path_is_relative)
bool path_is_relative(std::string path);		// is path relative? (!path_is_absolute)
void path_mkdir(std::string path);              // create directory if it does not exist

// look for the given argument in the command line, and return the next argv
// e.g. if the command line was: "quake3 --qmm_cfg test.json +set dedicated 1 +map q3dm1"
// then util_get_cmdline_arg("--qmm_cfg") returns "test.json"
std::string util_get_cmdline_arg(std::string arg, std::string def = "");
const char* util_get_proc_path();	// return path of process executable
const char* util_get_qmm_path();	// return path of QMM DLL
void* util_get_qmm_handle();		// return QMM DLL handle

// get milliseconds since GAME_INIT
intptr_t util_get_milliseconds();

// dll handling
void* dll_load(const char* filename);				// load DLL (LoadLibraryA/dlopen)
void* dll_symbol(void* dll, const char* symbol);	// find symbol in DLL (GetProcAddress, dlsym)
int dll_close(void* dll);							// close DLL (FreeLibrary/dlclose)
const char* dll_error();							// DLL error (GetLastError/dlerror)

// string comparison

std::string str_tolower(std::string str);					// convert str to lowercase
std::string str_toupper(std::string str);					// convert str to uppercase
bool str_stristr(std::string haystack, std::string needle);	// is needle in haystack (case-insensitive)
int str_stricmp(std::string s1, std::string s2);			// compare s1 and s2 (-1,0,1 for <,=,>) (case-insensitive)
bool str_striequal(std::string s1, std::string s2);			// are s1 and s2 equal? (case-insensitive)

// "safe" strncpy that always null-terminates
char* strncpyz(char* dest, const char* src, size_t count);

// tokenize an entstring into a vector of strings 
std::vector<std::string> util_parse_entstring(std::string entstring);

// max template to avoid any overlap with "max" in stdlib or game SDKs
template<typename T>
T util_max(T a, T b) {
    return (a > b ? a : b);
}

#endif // QMM2_UTIL_H
