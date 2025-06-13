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

int str_stristr(std::string haystack, std::string needle);
int str_stricmp(std::string s1, std::string s2);
int str_striequal(std::string s1, std::string s2);

// get a given argument with G_ARGV, based on game engine type
void qmm_argv(intptr_t argn, char* buf, intptr_t buflen);

// tokenize an entstring into a vector of strings 
std::vector<std::string> util_parse_tokens(std::string entstring);

template<typename T>
T util_max(T a, T b) {
    return (a > b ? a : b);
}

#endif // __QMM2_UTIL_H__
