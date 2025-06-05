/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include <string>
#include <vector>
#include <fstream>
#include "config.h"

nlohmann::json g_cfg;

nlohmann::json cfg_load(std::string file) {
	std::ifstream f(file);
	if (!f.fail()) {
		// parse(source, callback_handler, allow_exceptions, ignore_comments, ignore_trailing_commas)
		return nlohmann::json::parse(f, nullptr, false, true, true);
	}
	return nlohmann::json();
}


std::string cfg_get_string(nlohmann::json& j, std::string key, std::string def) {
	if (j.contains(key) && j[key].is_string()) {
		return j[key];
	}
	
	return def;
}


int cfg_get_int(nlohmann::json& j, std::string key, int def) {
	if (j.contains(key) && j[key].is_number_integer()) {
		return j[key];
	}

	return def;
}


bool cfg_get_bool(nlohmann::json& j, std::string key, bool def) {
	if (j.contains(key) && j[key].is_boolean()) {
		return j[key];
	}

	return def;
}


std::vector<std::string> cfg_get_array_str(nlohmann::json& j, std::string key, std::vector<std::string> def) {
	if (j.contains(key) && j[key].is_array()) {
		return j[key];
	}

	return def;
}


std::vector<int> cfg_get_array_int(nlohmann::json& j, std::string key, std::vector<int> def) {
	if (j.contains(key) && j[key].is_array()) {
		return j[key];
	}

	return def;
}


nlohmann::json cfg_get_object(nlohmann::json& j, std::string key, nlohmann::json def) {
	if (j.contains(key) && j[key].is_object()) {
		return j[key];
	}

	return def;
}
