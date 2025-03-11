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

std::string my_dirname(std::string);
std::string my_basename(std::string);
std::string my_baseext(std::string);

int my_stricmp(std::string, std::string);
int my_striequal(std::string, std::string);

bool is_relative_path(std::string);

std::string get_modulepath(void*);

int byteswap(int);
short byteswap(short);
char* vaf(const char*, ...);    // this uses a cycling array of strings so the return value does not need to be stored locally
int get_int_cvar(const char*);
const char* get_str_cvar(const char*);
void log_set(int);
int log_get();
int log_write(const char*, int = -1);
int dump_file(const char*, const char* = NULL);

#endif // __QMM2_UTIL_H__
