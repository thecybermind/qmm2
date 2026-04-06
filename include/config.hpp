/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_CONFIG_H
#define QMM2_CONFIG_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Primary configuration object
extern nlohmann::json g_cfg;

/**
* @brief Load configuration file
*
* @param file Path of configuration file to load
* @return JSON object representing configration file (or empty JSON object if failure)
*/
nlohmann::json cfg_load(std::string file);

/**
* @brief Get string from JSON object
*
* @param j Reference to JSON object to get string from
* @param key Name of string
* @param def Default value to return if key not found
* @return String value of key (or def)
*/
std::string cfg_get_string(nlohmann::json& j, std::string key, std::string def = "");

/**
* @brief Get integer from JSON object
*
* @param j Reference to JSON object to get integer from
* @param key Name of integer
* @param def Default value to return if key not found
* @return Integer value of key (or def)
*/
int cfg_get_int(nlohmann::json& j, std::string key, int def = -1);

/**
* @brief Get boolean from JSON object
*
* @param j Reference to JSON object to get boolean from
* @param key Name of boolean
* @param def Default value to return if key not found
* @return Boolean value of key (or def)
*/
bool cfg_get_bool(nlohmann::json& j, std::string key, bool def = false);

/**
* @brief Get list of strings from JSON object
*
* @param j Reference to JSON object to get string list from
* @param key Name of string list
* @param def Default value to return if key not found
* @return List of string values of key (or def)
*/
std::vector<std::string> cfg_get_array_str(nlohmann::json& j, std::string key, std::vector<std::string> def = {});

/**
* @brief Get list of integers from JSON object
*
* @param j Reference to JSON object to get integer list from
* @param key Name of integer list
* @param def Default value to return if key not found
* @return List of integer values of key (or def)
*/
std::vector<int> cfg_get_array_int(nlohmann::json& j, std::string key, std::vector<int> def = {});

/**
* @brief Get JSON object from JSON object
*
* @param j Reference to JSON object to get object from
* @param key Name of object
* @param def Default value to return if key not found
* @return Object value of key (or def)
*/
nlohmann::json cfg_get_object(nlohmann::json& j, std::string key, nlohmann::json def = nlohmann::json());

#endif // QMM2_CONFIG_H
