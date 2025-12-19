/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1
#include "osdef.h"
#include <string>
#include <string.h>
#include "util.h"

std::string path_normalize(std::string path) {
	// switch \ to /
	for (auto& c : path) {
		if (c == '\\')
			c = '/';
	}
	// collapse /./ to /
	auto pos = path.find_last_of("/./");
	while (pos != std::string::npos) {
		path = path.substr(0, pos) + path.substr(pos + 2);
		pos = path.find_last_of("/./");
	}

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
	// \\computer\dir\file
	// \dir\file
	// /dir/file
	if (path[0] == '/' || path[0] == '\\')
		return false;
	// ./file
	// ..\file
	// .ssh/known_hosts
	if (path[0] == '.')
		return true;
#ifdef _WIN32
	// windows: C:\dir\file
	if (path[1] == ':' && std::isalpha((unsigned char)(path[0])))
		return false;
	// windows: colon ANYWHERE is probably absolute too?
	if (path.find_first_of(':') != std::string::npos)
		return false;
#endif
	return true;
}


std::string util_get_modulepath(void* ptr) {
	return ptr ? osdef_path_get_modulepath(ptr) : osdef_path_get_procpath();
}


void* util_get_modulehandle(void* ptr) {
	return osdef_path_get_modulehandle(ptr);
}


void path_mkdir(std::string path) {
	unsigned int i = 1; // start after a possible /
#ifdef _WIN32
	// if this is an absolute path, start at the 3rd index (after "X:\")
	if (path[1] == ':')
		i = 3;
#endif
	path = path_normalize(path);

	// if the path ends with a /, remove it
	if (path[path.size() - 1] == '/')
		path[path.size() - 1] = '\0';

	// loop through each character
	for (; i < path.size(); i++)
		// if we found a directory separator
		if (path[i] == '/') {
			// replace it with a null terminator
			path[i] = '\0';
			// call mkdir on the whole path up to now to make the directory
			(void)mkdir(path.c_str(), S_IRWXU);
			// put the directory separator back
			path[i] = '/';
		}
	// call mkdir on the entire path to make the final directory
	(void)mkdir(path.c_str(), S_IRWXU);
}


std::string str_tolower(std::string str) {
	for (auto& c : str)
		c = (char)std::tolower((unsigned char)c);

	return str;
}


std::string str_toupper(std::string str) {
	for (auto& c : str)
		c = (char)std::toupper((unsigned char)c);

	return str;
}


int str_stristr(std::string haystack, std::string needle) {
	return str_tolower(haystack).find(str_tolower(needle)) != std::string::npos;
}


int str_stricmp(std::string s1, std::string s2) {
	return str_tolower(s1).compare(str_tolower(s2));
}


int str_striequal(std::string s1, std::string s2) {
	return str_stricmp(s1, s2) == 0;
}


// "safe" strncpy that always null-terminates
char* strncpyz(char* dest, const char* src, std::size_t count) {
	char* ret = strncpy(dest, src, count);
	dest[count - 1] = '\0';
	return ret;
}


// tokenize an entstring into a vector of strings 
std::vector<std::string> util_parse_entstring(std::string entstring) {
	std::vector<std::string> ret;

	std::string build;
	bool buildstr = false;

	for (auto& c : entstring) {
		// end if null (shouldn't happen)
		if (!c)
			break;
		// skip whitespace outside strings
		else if (std::isspace(c) && !buildstr)
			continue;
		// handle opening braces
		else if (c == '{')
			ret.push_back("{");
		// handle closing braces
		else if (c == '}')
			ret.push_back("}");
		// handle quote, start of a key or value
		else if (c == '"' && !buildstr) {
			build.clear();
			buildstr = true;
		}
		// handle quote, end of a key or value
		else if (c == '"' && buildstr) {
			ret.push_back(build);
			build.clear();
			buildstr = false;
		}
		// all other chars, add to build string
		else
			build.push_back(c);
	}

	return ret;
}
