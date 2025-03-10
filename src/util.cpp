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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "qmm.h"
#include "main.h"
#include "util.h"

std::string my_dirname(const std::string path) {
	auto pos = path.find_last_of('/');
	if (pos == std::string::npos || !pos)
		return "./";
	return path.substr(0, pos + 1);
}

std::string my_basename(const std::string path) {
	auto pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return path;
	return path.substr(pos + 1);
}

std::string my_baseext(const std::string path) {
	std::string base = my_basename(path);
	auto pos = base.find_last_of('.');
	if (pos == std::string::npos)
		return base;
	return base.substr(pos + 1);
}

int my_stricmp(const std::string& s1, const std::string& s2) {
	std::string s1c = s1;
	std::string s2c = s2;
	for (auto& c : s1c)
		c = std::tolower(c);
	for (auto& c : s2c)
		c = std::tolower(c);
	
	return s1c.compare(s2c);
}

int my_striequal(const std::string& s1, const std::string& s2) {
	return my_stricmp(s1, s2) == 0;
}

std::string get_qmm_modulepath() {
	static std::string path = "";
	if (path.empty())
		path = osdef_get_qmm_modulepath();	
	return path;
}

int byteswap(int i) {
	byte b1,b2,b3,b4;

	b1 = (byte)(i&255);
	b2 = (byte)((i>>8)&255);
	b3 = (byte)((i>>16)&255);
	b4 = (byte)((i>>24)&255);

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + (int)b4;
}
short byteswap(short s) {
	byte b1,b2;

	b1 = (byte)(s&255);
	b2 = (byte)((s>>8)&255);

	return (short)(((short)b1<<8) + (short)b2);
}

//this uses a cycling array of strings so the return value does not need to be stored locally
char* vaf(const char* format, ...) {
	va_list	argptr;
	static char str[8][1024];
	static int index = 0;
	int i = index;

	va_start(argptr, format);
	my_vsnprintf(str[i], sizeof(str[i]), format, argptr);
	va_end(argptr);

	index = (index + 1) & 7;
	return str[i];
}

int get_int_cvar(const char* cvar) {
	if (!cvar || !*cvar)
		return -1;

	return ENG_SYSCALL(ENG_MSG[QMM_G_CVAR_VARIABLE_INTEGER_VALUE], cvar);
}

//this uses a cycling array of strings so the return value does not need to be stored locally
#define MAX_CVAR_LEN	256
const char* get_str_cvar(const char* cvar) {
	if (!cvar || !*cvar)
		return NULL;

	static char temp[8][MAX_CVAR_LEN];
	static int index = 0;
	int i = index;

	ENG_SYSCALL(ENG_MSG[QMM_G_CVAR_VARIABLE_STRING_BUFFER], cvar, temp[i], sizeof(temp[i]));
	index = (index + 1) & 7;
	return temp[i];
}

static int s_fh = -1;

void log_set(int fh) {
	s_fh = fh;
}

int log_get() {
	return s_fh;
}

int log_write(const char* text, int len) {
	if (s_fh != -1 && text && *text) {
		if (len == -1)
			len = strlen(text);

		return ENG_SYSCALL(ENG_MSG[QMM_G_FS_WRITE], text, len, s_fh);
	}
	
	return -1;
}

int dump_file(const char* file, const char* outfile) {
	outfile = vaf("%s/%s", g_GameInfo.qmm_dir, outfile ? outfile : file);
	
	//check if the real file already exists
	FILE* ffile = fopen(outfile, "r");
	if (ffile) {
		fclose(ffile);
		return 0;
	}

	//open file from inside pk3
	int fpk3, fsize = ENG_SYSCALL(ENG_MSG[QMM_G_FS_FOPEN_FILE], file, &fpk3, ENG_MSG[QMM_FS_READ]);
	if (fsize <= 0) {
		ENG_SYSCALL(ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);
		return 0;
	}

	//open output file
	ffile = fopen(outfile, "wb");
	if (!ffile) {
		ENG_SYSCALL(ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);
		return 0;
	}

	//read file in blocks of 512
	byte buf[512];
	int left = fsize;
	while (left >= sizeof(buf)) {
		ENG_SYSCALL(ENG_MSG[QMM_G_FS_READ], buf, sizeof(buf), fpk3);
		fwrite(buf, sizeof(buf), 1, ffile);
		left -= sizeof(buf);
	}
	if (left) {
		ENG_SYSCALL(ENG_MSG[QMM_G_FS_READ], buf, left, fpk3);
		fwrite(buf, left, 1, ffile);
	}

	//close file handles
	ENG_SYSCALL(ENG_MSG[QMM_G_FS_FCLOSE_FILE], fpk3);
	fclose(ffile);

	return fsize;
}
