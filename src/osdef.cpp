/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#include "osdef.h"
#include <string>
#include "version.h"

#ifdef WIN32
#include <stdlib.h>

char* dlerror() {
    static char buffer[4096 * 2]; // https://stackoverflow.com/a/75644008
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
    //remove newlines at the end
    buffer[strlen(buffer) - 2] = '\0';

    return buffer;
}

#endif

std::string get_qmm_modulepath() {
	static std::string path = "";
	if (!path.empty())
		return path;

#ifdef WIN32
	MEMORY_BASIC_INFORMATION MBI;
	static char buffer[PATH_MAX] = "";

	if (!VirtualQuery((void*)&get_qmm_modulepath, &MBI, sizeof(MBI)) || MBI.State != MEM_COMMIT || !MBI.AllocationBase)
		return "";

	if (!GetModuleFileName((HMODULE)MBI.AllocationBase, buffer, sizeof(buffer)))
		return "";
	
	path = buffer;
#else
	Dl_info dli;
	memset(&dli, 0, sizeof(dli));

	if (!dladdr((void*)&get_qmm_modulepath, &dli))
		return "";

	path = dli.dli_fname;
#endif
	return path;
}
