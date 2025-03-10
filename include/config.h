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
#include "nlohmann/json.hpp"

extern nlohmann::json g_cfg;

nlohmann::json cfg_load(std::string);

std::string cfg_get_string(nlohmann::json&, std::string, std::string = "");
int cfg_get_int(nlohmann::json&, std::string, int = -1);
bool cfg_get_bool(nlohmann::json&, std::string, bool = false);
std::vector<std::string> cfg_get_array(nlohmann::json&, std::string, std::vector<std::string> = {});
nlohmann::json cfg_get_object(nlohmann::json&, std::string, nlohmann::json = nlohmann::json());

#endif //__QMM2_CONFIG_H__
