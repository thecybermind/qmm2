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
#include "format.h"
#include "game_api.h"
#include "main.h"
#include "util.h"

std::string path_normalize(std::string path) {
	for (auto& c : path) {
		if (c == '\\')
			c = '/';
	}
	return path;
}

std::string path_dirname(std::string path) {
	auto pos = path.find_last_of("/\\");
	if (pos == std::string::npos || !pos)
		return ".";
	return path.substr(0, pos);
}

std::string path_basename(std::string path) {
	auto pos = path.find_last_of("/\\");
	if (pos == std::string::npos)
		return path;
	return path.substr(pos + 1);
}

std::string path_baseext(std::string path) {
	std::string base = path_basename(path);
	auto pos = base.find_last_of('.');
	if (pos == std::string::npos)
		return base;
	return base.substr(pos + 1);
}

bool path_is_relative(std::string path) {
	// \\dir\file
	// \dir\file
	// /dir/file
	if (path[0] == '/' || path[0] == '\\')
		return false;
	// ./file
	// ..\file
	// .ssh/known_hosts
	if (path[0] == '.')
		return true;
	// C:\dir\file
	if (path[1] == ':' && std::isalpha((unsigned char)(path[0])))
		return false;
	// colon ANYWHERE is probably absolute too?
	if (path.find_first_of(':') != std::string::npos)
		return false;
	return true;
}

std::string path_get_modulepath(void* ptr) {
	return ptr ? osdef_path_get_modulepath(ptr) : osdef_path_get_procpath();
}

int str_stristr(std::string haystack, std::string needle) {
	for (auto& c : haystack)
		c = std::tolower(c);
	for (auto& c : needle)
		c = std::tolower(c);

	return haystack.find(needle) != std::string::npos;
}

int str_stricmp(std::string s1, std::string s2) {
	for (auto& c : s1)
		c = std::tolower(c);
	for (auto& c : s2)
		c = std::tolower(c);

	return s1.compare(s2);
}

int str_striequal(std::string s1, std::string s2) {
	return str_stricmp(s1, s2) == 0;
}

// this uses a cycling array of strings so the return value does not need to be stored locally
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

int util_get_int_cvar(const char* cvar) {
	if (!cvar || !*cvar)
		return -1;

	return ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_INTEGER_VALUE], cvar);
}

// this uses a cycling array of strings so the return value does not need to be stored locally
#define MAX_CVAR_LEN	256
const char* util_get_str_cvar(const char* cvar) {
	if (!cvar || !*cvar)
		return NULL;

	static char temp[8][MAX_CVAR_LEN];
	static int index = 0;
	int i = index;

	ENG_SYSCALL(QMM_ENG_MSG[QMM_G_CVAR_VARIABLE_STRING_BUFFER], cvar, temp[i], sizeof(temp[i]));
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

		return ENG_SYSCALL(QMM_ENG_MSG[QMM_G_FS_WRITE], text, len, s_fh);
	}
	
	return -1;
}
