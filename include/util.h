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

namespace util {
    std::string path_normalize(std::string path);
    std::string path_dirname(std::string path);
    std::string path_basename(std::string path);
    std::string path_baseext(std::string path);
    bool path_is_relative(std::string path);
    std::string get_modulepath(void* ptr);
    void* get_modulehandle(void* ptr);

    void path_mkdir(std::string path);

    int stristr(std::string haystack, std::string needle);
    int stricmp(std::string s1, std::string s2);
    int striequal(std::string s1, std::string s2);

    void qmm_argv(intptr_t argn, char* buf, intptr_t buflen);

    template<typename T>
    T util_max(T a, T b) {
        return ((a) > (b) ? (a) : (b));
    }
}

#endif // __QMM2_UTIL_H__
