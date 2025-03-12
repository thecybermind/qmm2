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

std::string path_normalize(std::string);
std::string path_dirname(std::string);
std::string path_basename(std::string);
std::string path_baseext(std::string);
bool path_is_relative(std::string);
std::string path_get_modulepath(void*);

int str_stricmp(std::string, std::string);
int str_striequal(std::string, std::string);

char* vaf(const char*, ...);    // this uses a cycling array of strings so the return value does not need to be stored locally
int util_get_int_cvar(const char*);
const char* util_get_str_cvar(const char*);
void log_set(int);
int log_get();
int log_write(const char*, int = -1);
int util_dump_file(std::string, std::string);

#endif // __QMM2_UTIL_H__
