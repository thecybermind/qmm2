/*
QMM2 - Q3 MultiMod 2
Copyright 2025
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



void* osdef_path_get_modulehandle(void* ptr) {
    void* handle = nullptr;

#if defined(_WIN32)
    MEMORY_BASIC_INFORMATION MBI;

    if (!VirtualQuery(ptr, &MBI, sizeof(MBI)) || MBI.State != MEM_COMMIT || !MBI.AllocationBase)
        return nullptr;

    handle = (void*)MBI.AllocationBase;
#elif defined(__linux__)
    Dl_info dli;
    memset(&dli, 0, sizeof(dli));

    if (!dladdr(ptr, &dli))
        return nullptr;

    handle = dli.dli_fbase;
#endif
    return handle;
}


const char* osdef_path_get_modulepath(void* ptr) {
    static char path[PATH_MAX] = "";
    memset(path, 0, sizeof(path));

#if defined(_WIN32)
    MEMORY_BASIC_INFORMATION MBI;

    if (!VirtualQuery(ptr, &MBI, sizeof(MBI)) || MBI.State != MEM_COMMIT || !MBI.AllocationBase)
        return "";

    if (!GetModuleFileName((HMODULE)MBI.AllocationBase, path, sizeof(path)))
        return "";
#elif defined(__linux__)
    Dl_info dli;
    memset(&dli, 0, sizeof(dli));

    if (!dladdr(ptr, &dli))
        return "";

    strncpyz(path, dli.dli_fname, sizeof(path));
#endif
    return path;
}


const char* osdef_path_get_procpath() {
    static char path[PATH_MAX] = "";
    memset(path, 0, sizeof(path));

#if defined(_WIN32)
    if (!GetModuleFileName(nullptr, path, sizeof(path)))
        return "";
#elif defined(__linux__)
    // readlink does NOT null terminate at all
    // we pass sizeof-1 to guarantee the \0 from memset is still present at the end of the string
    // as  a null terminator. also we write a \0 at the specific end of the written buffer.
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1)
        path[len] = '\0';
#endif
    return path;
}
