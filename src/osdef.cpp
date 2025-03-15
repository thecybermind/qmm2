/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#include "osdef.h"

#ifdef _WIN32

char* dlerror() {
    static char buffer[4096 * 2]; // https://stackoverflow.com/a/75644008
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
    // remove newlines at the end
    buffer[strlen(buffer) - 2] = '\0';

    return buffer;
}

#endif

const char* osdef_path_get_modulepath(void* ptr) {
	static char path[PATH_MAX] = "";
	memset(path, 0, sizeof(path));

#ifdef _WIN32
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

	strncpy(path, dli.dli_fname, sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';
#else

#endif
	return path;
}

const char* osdef_path_get_procpath() {
	static char path[PATH_MAX] = "";
	memset(path, 0, sizeof(path));

#ifdef _WIN32
	if (!GetModuleFileName(NULL, path, sizeof(path)))
		return "";
#elif defined(__linux__)
	ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (len != -1) {
		path[len] = '\0';
	}
	path[sizeof(path) - 1] = '\0';
#else

#endif
	return path;
}