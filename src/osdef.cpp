/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include "osdef.h"
#include <string>
#include <cstring>  // memset in linux only
#include "util.h"   // strncpyz in linux only

#if defined(QMM_OS_WINDOWS)
// store module handle
static HMODULE s_dll = nullptr;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID) {
    s_dll = hinstDLL;
    return TRUE;
}


// return error string for GetLastError()
const char* dlerror() {
    static std::string str;
    char* buf = nullptr;
    str = "";

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, nullptr);

    str = buf;

    LocalFree(buf);

    return str.c_str();
}
#elif defined(QMM_OS_LINUX)
uint64_t osdef_get_milliseconds() {
    struct timeval tp;
    struct timezone tzp;

    gettimeofday(&tp, &tzp);

    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}
#endif


void* osdef_path_get_qmm_handle() {
#if defined(QMM_OS_WINDOWS)
    return s_dll;
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


const char* osdef_path_get_qmm_path() {
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


const char* osdef_path_get_proc_path() {
    static char path[PATH_MAX];
    if (path[0])
        return path;

#if defined(QMM_OS_WINDOWS)
    if (!GetModuleFileName(nullptr, path, sizeof(path)))
        return "";
#elif defined(QMM_OS_LINUX)
    // readlink does NOT null terminate at all
    // we pass sizeof-1 to guarantee the \0 from memset is still present at the end of the string
    // as a null terminator. also we write a \0 at the specific end of the written buffer.
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1)
        path[len] = '\0';
#endif
    return path;
}
