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
	// switch \ to /
	for (auto& c : path) {
		if (c == '\\')
			c = '/';
	}
	// collapse /./ to /
	auto pos = path.find_last_of("/./");
	if (pos == std::string::npos)
		path = path.substr(0, pos) + path.substr(pos + 2);

	return path;
}

std::string path_dirname(std::string path) {
	auto pos = path.find_last_of("/\\");
	if (pos == std::string::npos)
		return ".";
	if (pos == 0)
		return "/.";
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

void* path_get_modulehandle(void* ptr) {
	return osdef_path_get_modulehandle(ptr);
}

int str_stristr(std::string haystack, std::string needle) {
	for (auto& c : haystack)
		c = (char)std::tolower((unsigned char)c);
	for (auto& c : needle)
		c = (char)std::tolower((unsigned char)c);

	return haystack.find(needle) != std::string::npos;
}

int str_stricmp(std::string s1, std::string s2) {
	for (auto& c : s1)
		c = (char)std::tolower((unsigned char)c);
	for (auto& c : s2)
		c = (char)std::tolower((unsigned char)c);

	return s1.compare(s2);
}

int str_striequal(std::string s1, std::string s2) {
	return str_stricmp(s1, s2) == 0;
}
