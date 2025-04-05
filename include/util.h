/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_UTIL_H__
#define __QMM2_UTIL_H__

std::string path_normalize(std::string path);
std::string path_dirname(std::string path);
std::string path_basename(std::string path);
std::string path_baseext(std::string path);
bool path_is_relative(std::string path);
std::string path_get_modulepath(void* ptr);
void* path_get_modulehandle(void* ptr);

int str_stristr(std::string haystack, std::string needle);
int str_stricmp(std::string s1, std::string s2);
int str_striequal(std::string s1, std::string s2);

#define util_max(a, b)  ((a) > (b) ? (a) : (b))

char* vaf(const char* format, ...);    // this uses a cycling array of strings so the return value does not need to be stored locally
int util_get_int_cvar(const char* cvar);
const char* util_get_str_cvar(const char* cvar);

#endif // __QMM2_UTIL_H__
