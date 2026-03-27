/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1
#include "version.h"
#include <cctype>
#include <cstring>
#include <cstdint>
#include <cstddef>      // size_t
#include <vector>
#include <string>
#include <filesystem>
#include "gameinfo.hpp"
#include "util.hpp"

#if defined(QMM_OS_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>				// _mkdir
#include <shellapi.h>			// CommandLineToArgvW

#define PATH_MAX				MAX_PATH
#define mkdir(path, x)			_mkdir(path)
#define util_get_ticks			GetTickCount64


// store module handle for util_get_qmm_path and util_get_qmm_handle
static HMODULE s_dll = nullptr;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID) {
    s_dll = hinstDLL;
    return TRUE;
}

#elif defined(QMM_OS_LINUX)

#include <cstdlib>
#include <fstream>			// getline for util_get_proc_cmdline
#include <dlfcn.h>			// dlopen, dlclose, dlsym
#include <unistd.h>			// readlink
#include <limits.h>			// PATH_MAX
#include <sys/stat.h>		// mkdir
#include <sys/time.h>


static uint64_t util_get_ticks() {
    struct timeval tp;
    struct timezone tzp;

    gettimeofday(&tp, &tzp);

    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

#endif


std::string path_normalize(std::string path) {
    std::filesystem::path fspath = path;
    fspath = fspath.lexically_normal();
    // check for empty or starting with ".." just in case
    if (fspath.empty() || *fspath.begin() == "..")
        return "";
    return fspath.generic_u8string();
}


bool path_is_allowed(std::string path) {
    if (path_baseext(path) == EXT_QVM)
        return true;

    path = path_normalize(path);
    auto rel_qmm = std::filesystem::relative(path, gameinfo.qmm_dir);
    auto rel_exe = std::filesystem::relative(path, gameinfo.exe_dir);
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


static std::vector<std::string> util_get_proc_cmdline() {
    static std::vector<std::string> ret;
    if (ret.size())
        return ret;
#if defined(QMM_OS_WINDOWS)
    // CommandLineToArgvA doesn't exist, so we have to do this with wide strings and convert them to utf8 std::strings
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char buf[1024];
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
    // don't read last arg because it can't have a "next" arg
    for (size_t i = 1; i < argv.size() - 1; i++) {
        if (str_striequal(argv[i], arg)) {
            return argv[i + 1];
        }
    }
    return def;
}


const char* util_get_proc_path() {
    static char path[PATH_MAX];
    if (path[0])
        return path;

#if defined(QMM_OS_WINDOWS)
    if (!GetModuleFileName(nullptr, path, sizeof(path)))
        return "";
#elif defined(QMM_OS_LINUX)
    // readlink does NOT null terminate at all
    // we pass sizeof-1 to guarantee the \0 from init is still present at the end of the string
    // as a null terminator. also we write a \0 at the specific end of the written buffer.
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1)
        path[len] = '\0';
#endif
    return path;
}


const char* util_get_qmm_path() {
    static char path[PATH_MAX];
    if (path[0])
        return path;

#if defined(QMM_OS_WINDOWS)
    if (!GetModuleFileName(s_dll, path, sizeof(path)))
        return "";
#elif defined(QMM_OS_LINUX)
    Dl_info dli;
    memset(&dli, 0, sizeof(dli));

    if (!dladdr(&dli, &dli))
        return "";

    strncpyz(path, dli.dli_fname, sizeof(path));
#endif
    return path;
}


void* util_get_qmm_handle() {
#if defined(QMM_OS_WINDOWS)
    return (void*)s_dll;
#elif defined(QMM_OS_LINUX)
    static void* module;
    if (module)
        return module;

    Dl_info dli;
    memset(&dli, 0, sizeof(dli));

    if (!dladdr(&dli, &dli))
        return nullptr;

    module = dli.dli_fbase;
    return module;
#endif
}


intptr_t util_get_milliseconds() {
    static bool initialized = false;
    static uint64_t startTime = 0;
    if (!initialized) {
        initialized = true;
        startTime = util_get_ticks();
    }
    return (intptr_t)(util_get_ticks() - startTime);
}


void* dll_load(const char* filename) {
#if defined(QMM_OS_WINDOWS)
    return (void*)LoadLibraryA(filename);
#elif defined(QMM_OS_LINUX)
    return dlopen(filename, RTLD_NOW);
#endif
}


void* dll_symbol(void* dll, const char* symbol) {
#if defined(QMM_OS_WINDOWS)
    return (void*)GetProcAddress((HMODULE)dll, symbol);
#elif defined(QMM_OS_LINUX)
    return dlsym(dll, symbol);
#endif
}


int dll_close(void* dll) {
#if defined(QMM_OS_WINDOWS)
    return FreeLibrary((HMODULE)dll);
#elif defined(QMM_OS_LINUX)
    return dlclose(dll);
#endif
}


const char* dll_error() {
#if defined(QMM_OS_WINDOWS)
    // this will return the last error from any win32 function, not just library functions
    static std::string str;
    char* buf = nullptr;
    str = "";

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, nullptr);

    str = buf;

    LocalFree(buf);

    return str.c_str();
#elif defined(QMM_OS_LINUX)
    return dlerror();
#endif
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


bool str_striequal(std::string s1, std::string s2) {
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
