/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1
#include "osdef.h"
#include <cctype>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream> // util_get_proc_cmdline in linux
#include "main.h"
#include "util.h"


std::string path_normalize(std::string path) {
    std::filesystem::path fspath = path;
    fspath = fspath.lexically_normal();
    // check for empty or starting with ".." just in case
    if (fspath.empty() || *fspath.begin() == "..")
        return "";
    return fspath.generic_u8string();
}


bool path_is_allowed(std::string path) {
    path = path_normalize(path);
    auto rel_qmm = std::filesystem::relative(path, g_gameinfo.qmm_dir);
    auto rel_exe = std::filesystem::relative(path, g_gameinfo.exe_dir);
    // if there is no relative path, the return is ""
    // if the relative path requires going back up, it starts with ".."
    // otherwise it should be the relative path from qmm_dir or exe_dir 
    if ((!rel_qmm.empty() && rel_qmm.string()[0] != '.') || (!rel_exe.empty() && rel_exe.string()[0] != '.'))
        return true;
    return false;
}


std::string path_dirname(std::string path) {
    std::filesystem::path fspath = path;
    return fspath.parent_path().lexically_normal().generic_u8string();
}


std::string path_basename(std::string path) {
    std::filesystem::path fspath = path;
    return fspath.filename().u8string();

}


std::string path_baseext(std::string path) {
    std::filesystem::path fspath = path;
    return fspath.extension().u8string();
}


bool path_is_absolute(std::string path) {
    if (path.empty())
        return false;
    std::filesystem::path fspath = path;
    return fspath.is_absolute();
}


bool path_is_relative(std::string path) {
    if (path.empty())
        return false;
    return !path_is_absolute(path);
}


static std::vector<std::string> util_get_proc_cmdline() {
    std::vector<std::string> ret;
#if defined(QMM_OS_WINDOWS)
    // CommandLineToArgvA doesn't exist, so we have to do this with wide strings and convert them to utf8 std::strings
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char buf[MAX_PATH];
    for (int i = 0; i < argc; i++) {
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, buf, sizeof(buf), NULL, NULL);
        buf[sizeof(buf) - 1] = '\0';
        ret.push_back(buf);
    }
#elif defined(QMM_OS_LINUX)
    // read null-terminated argv strings from /proc/self/cmdline
    std::ifstream in("/proc/self/cmdline");
    std::string argv;
    while (std::getline(in, argv, '\0')) {
        ret.push_back(argv);
    }
#endif
    return ret;
}


std::string util_get_cmdline_arg(std::string arg, std::string def) {
    std::vector<std::string> argv = util_get_proc_cmdline();
    // 1 to skip binary name
    for (size_t i = 1; i < argv.size() - 1; i++) { // don't read last arg because it can't have a "next" arg
        if (str_striequal(argv[i], arg)) {
            return argv[i + 1];
        }
    }
    return def;
}


std::string util_get_proc_path() {
    return osdef_path_get_proc_path();
}


std::string util_get_qmm_path() {
    return osdef_path_get_qmm_path();
}


void* util_get_qmm_handle() {
    return osdef_path_get_qmm_handle();
}


intptr_t util_get_milliseconds() {
    static bool initialized = false;
    static uint64_t startTime = 0;
    if (!initialized) {
        initialized = true;
        startTime = osdef_get_milliseconds();
    }
    return (intptr_t)(osdef_get_milliseconds() - startTime);
}


void path_mkdir(std::string path) {
    if (path.empty())
        return;

    std::filesystem::path fspath = path;

    std::filesystem::path build;
    // loop through each segment of path and call mkdir on it
    for (auto& seg : fspath.lexically_normal().parent_path()) {
        build /= seg;
        (void)mkdir(build.u8string().c_str(), S_IRWXU);
    }
}


std::string str_tolower(std::string str) {
    for (char& c : str)
        c = (char)std::tolower((unsigned char)c);

    return str;
}


std::string str_toupper(std::string str) {
    for (char& c : str)
        c = (char)std::toupper((unsigned char)c);

    return str;
}


bool str_stristr(std::string haystack, std::string needle) {
    return str_tolower(haystack).find(str_tolower(needle)) != std::string::npos;
}


int str_stricmp(std::string s1, std::string s2) {
    return str_tolower(s1).compare(str_tolower(s2));
}


int str_striequal(std::string s1, std::string s2) {
    return str_stricmp(s1, s2) == 0;
}


// "safe" strncpy that always null-terminates
char* strncpyz(char* dest, const char* src, size_t count) {
    if (!dest || !src || !count)
        return dest;
    char* ret = strncpy(dest, src, count);
    dest[count - 1] = '\0';
    return ret;
}


// tokenize an entstring into a vector of strings 
std::vector<std::string> util_parse_entstring(std::string entstring) {
    std::vector<std::string> ret;

    std::string build;
    bool buildstr = false;

    for (const char& c : entstring) {
        // end if null (shouldn't happen)
        if (!c)
            break;
        // skip whitespace outside strings
        else if (std::isspace(c) && !buildstr)
            continue;
        // handle opening braces
        else if (c == '{' && !buildstr)
            ret.push_back("{");
        // handle closing braces
        else if (c == '}' && !buildstr)
            ret.push_back("}");
        // handle quote, start of a key or value
        else if (c == '"' && !buildstr) {
            build.clear();
            buildstr = true;
        }
        // handle quote, end of a key or value
        else if (c == '"' && buildstr) {
            ret.push_back(build);
            build.clear();
            buildstr = false;
        }
        // all other chars, add to build string
        else
            build.push_back(c);
    }

    return ret;
}
