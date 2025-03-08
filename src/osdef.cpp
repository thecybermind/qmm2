/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#include "osdef.h"
#include "version.h"

#ifdef WIN32
#include <stdlib.h>
#pragma comment(exestr, "\1Q3 MultiMod 2 - QMM v" QMM_VERSION " (" QMM_OS ")")
#pragma comment(exestr, "\1Built: " QMM_COMPILE " by " QMM_BUILDER)
#pragma comment(exestr, "\1URL: https://github.com/thecybermind/qmm2")

char* dlerror() {
    static char buffer[4096 * 2]; // https://stackoverflow.com/a/75644008
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
    //remove newlines at the end
    buffer[strlen(buffer) - 2] = '\0';

    return buffer;
}

#else // linux

volatile const char* binary_comment[] = { "\1Q3 MultiMod 2 - QMM v" QMM_VERSION " (" QMM_OS ")",
                                          "\1Built: " QMM_COMPILE " by " QMM_BUILDER ",
                                          "\1URL: https://github.com/thecybermind/qmm2"
};

#endif

const char* get_qmm_modulename() {
	static char path[PATH_MAX] = "";
	if (path[0])
		return path;

#ifdef WIN32
	MEMORY_BASIC_INFORMATION MBI;

	if (!VirtualQuery((void*)&get_qmm_modulename, &MBI, sizeof(MBI)) || MBI.State != MEM_COMMIT || !MBI.AllocationBase)
		return NULL;

	if (!GetModuleFileName((HMODULE)MBI.AllocationBase, path, sizeof(path)) || !path[0])
		return NULL;
#else
	Dl_info dli;
	memset(&dli, 0, sizeof(dli));

	if (!dladdr((void*)&get_qmm_modulename, &dli))
		return NULL;

	strncpy(path, dli.dli_fname, sizeof(path) - 1);
#endif
	path[sizeof(path) - 1] = '\0';
	return path;
}

void* get_qmm_modulehandle() {
	static void* handle = NULL;
	if (handle)
		return handle;

#ifdef WIN32
	MEMORY_BASIC_INFORMATION MBI;

	if (!VirtualQuery((void*)&get_qmm_modulehandle, &MBI, sizeof(MBI)) || MBI.State != MEM_COMMIT)
		return NULL;

	handle = (void*)MBI.AllocationBase;
#else
	Dl_info dli;
	memset(&dli, 0, sizeof(dli));

	if (!dladdr((void*)&get_qmm_modulehandle, &dli))
		return NULL;

	handle = dli.dli_fbase;
#endif
	return handle;
}
