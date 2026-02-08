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
#include <cstring>
#include "util.h"   // strncpyz in linux only

#ifdef _WIN32
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
#else
// just output a big banner to stdout and stderr
void MessageBoxA(void* handle, const char* message, const char* title, int flags) {
    fprintf(stderr, "**************************************************************************\n");
    fprintf(stderr, "%s\n", title);
    fprintf(stderr, "**************************************************************************\n");
    fprintf(stderr, "%s\n", message);
    fprintf(stderr, "**************************************************************************\n");
    printf("**************************************************************************\n");
    printf("%s\n", title);
    printf("**************************************************************************\n");
    printf("%s\n", message);
    printf("**************************************************************************\n");
}
#endif



#if defined(_WIN32)
static HMODULE s_dll = nullptr;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID) {
    s_dll = hinstDLL;
    return TRUE;
}
#endif


void* osdef_path_get_qmm_handle() {
#if defined(_WIN32)
    return s_dll;
#elif defined(__linux__)
    Dl_info dli;
    memset(&dli, 0, sizeof(dli));

    if (!dladdr(&dli, &dli))
        return nullptr;

    return dli.dli_fbase;
#endif
}


const char* osdef_path_get_qmm_path() {
    static char path[PATH_MAX] = "";
    memset(path, 0, sizeof(path));

#if defined(_WIN32)
    if (!GetModuleFileName(s_dll, path, sizeof(path)))
        return "";
#elif defined(__linux__)
    Dl_info dli;
    memset(&dli, 0, sizeof(dli));

    if (!dladdr(&dli, &dli))
        return "";

    strncpyz(path, dli.dli_fname, sizeof(path));
#endif
    return path;
}


const char* osdef_path_get_proc_path() {
    static char path[PATH_MAX] = "";
    memset(path, 0, sizeof(path));

#if defined(_WIN32)
    if (!GetModuleFileName(nullptr, path, sizeof(path)))
        return "";
#elif defined(__linux__)
    // readlink does NOT null terminate at all
    // we pass sizeof-1 to guarantee the \0 from memset is still present at the end of the string
    // as a null terminator. also we write a \0 at the specific end of the written buffer.
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1)
        path[len] = '\0';
#endif
    return path;
}


#ifdef __linux__
static time_t osdef_get_milliseconds() {
    struct timeval tp;
    struct timezone tzp;

    gettimeofday(&tp, &tzp);

    return (tp.tv_sec - startTime) * 1000 + tp.tv_usec / 1000;
}
#endif
