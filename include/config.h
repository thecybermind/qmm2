/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_CONFIG_H__
#define __QMM2_CONFIG_H__

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

extern nlohmann::json g_cfg;

nlohmann::json cfg_load(std::string file);

std::string cfg_get_string(nlohmann::json& j, std::string key, std::string def = "");
int cfg_get_int(nlohmann::json& j, std::string key, int def = -1);
bool cfg_get_bool(nlohmann::json& j, std::string key, bool def = false);
std::vector<std::string> cfg_get_array_str(nlohmann::json& j, std::string key, std::vector<std::string> def = {});
std::vector<int> cfg_get_array_int(nlohmann::json& j, std::string key, std::vector<int> def = {});
nlohmann::json cfg_get_object(nlohmann::json& j, std::string key, nlohmann::json def = nlohmann::json());

#endif // __QMM2_CONFIG_H__
