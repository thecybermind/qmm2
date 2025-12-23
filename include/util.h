/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_UTIL_H__
#define __QMM2_UTIL_H__

#include <vector>
#include <string>

std::string path_normalize(std::string path);
std::string path_dirname(std::string path);
std::string path_basename(std::string path);
std::string path_baseext(std::string path);
bool path_is_relative(std::string path);

std::string util_get_modulepath(void* ptr);
void* util_get_modulehandle(void* ptr);

void path_mkdir(std::string path);

std::string str_tolower(std::string str);
std::string str_toupper(std::string str);
int str_stristr(std::string haystack, std::string needle);
int str_stricmp(std::string s1, std::string s2);
int str_striequal(std::string s1, std::string s2);

// "safe" strncpy that always null-terminates
char* strncpyz(char* dest, const char* src, std::size_t count);

// tokenize an entstring into a vector of strings 
std::vector<std::string> util_parse_entstring(std::string entstring);

template<typename T>
T util_max(T a, T b) {
    return (a > b ? a : b);
}

#endif // __QMM2_UTIL_H__
