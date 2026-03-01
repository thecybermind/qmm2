/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_MAIN_AUX_H
#define QMM2_MAIN_AUX_H

#include <string>
#include <cstdint>

void main_detect_env();
void main_load_config();
void main_detect_game(std::string cfg_game, bool is_GetGameAPI_mode);
bool main_load_mod(std::string cfg_mod);
bool main_load_plugin(std::string plugin_path);
void main_handle_command_qmm(intptr_t arg_start);
intptr_t main_route_vmmain(intptr_t cmd, intptr_t* args);
intptr_t main_route_syscall(intptr_t cmd, intptr_t* args);

#endif // QMM2_MAIN_AUX_H
