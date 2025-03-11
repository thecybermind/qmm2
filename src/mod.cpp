/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include "osdef.h"
#include "main.h"
#include "mod.h"
#include "util.h"

mod_t g_mod;

/* todo:
   * use extension and/or load with engine file functions to determine mod type
   * if qvm, create a qvm object, set g_mod.pfnvmMain to the qvm entry point function (like qvm_vmMain)
   * if dll, load normal crap, set g_mod.pfnvmMain to the actual dll vmMain function
   * when mod should be unloaded, set to nullptr
   
   also need to handle code for loading from different directories:
   if cfg "mod" exists but is not relative path, load it directly with no fallback
   if cfg "mod" exists but is relative path:
    * check qmm dir + "<mod>"
	* check exe dir + "<moddir>/<mod>"
	* check "./<moddir>/<mod>"
   if cfg "mod" doesn't exist:
	* check qmm dir + "qmm_<dllname>"
	* check exe dir + "<moddir>/qmm_<dllname>"
	* check exe dir + "<moddir>/<dllname>" (as long as exe dir is not same as qmm dir)
	* check "./<moddir>/qmm_<dllname>"


*/

// load file using engine functions, and check the magic numbers at the beginning of the file
headertype_t check_header(std::string file) {
	int fid;
	unsigned char hdr[4];

	// attempt to open the file
	int filesize = ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FOPEN_FILE], file.c_str(), &fid, QMM_ENG_MSG[QMM_FS_READ]);
	// if it exists and loaded
	if (filesize > 0) {
		// read magic numbers and close file handle
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_READ], &hdr, sizeof(hdr), fid);
		ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_FCLOSE_FILE], fid);

		if (memcmp(hdr, MAGIC_DLL, sizeof(hdr)) == 0)
			return HEADER_DLL;
		// only allow it to return a qvm mod if this game supports it
		if (memcmp(hdr, MAGIC_QVM, sizeof(hdr) && g_gameinfo.game->vmsyscall) == 0)
			return HEADER_QVM;
	}

	// if unable to determine file type via header, just check the extension
	std::string ext = my_baseext(file);
	if (my_striequal(ext, EXT_DLL))
		return HEADER_DLL;
	// only allow it to return a qvm mod if this game supports it
	if (my_striequal(ext, EXT_QVM) && g_gameinfo.game->vmsyscall)
		return HEADER_QVM;

	return HEADER_UNKNOWN;
}