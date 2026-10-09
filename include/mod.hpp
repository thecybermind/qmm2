/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_MOD_HPP
#define QMM2_MOD_HPP

#include <string>
#include "gameapi.hpp"
#include "qvm.h"

// Currently loaded mod info.
namespace Mod {
    extern qvm vm;              // QVM object
    extern void* dll;           // OS DLL handle
    extern std::string path;    // Mod file path
    extern APIType api;         // API the mod DLL was loaded with

    /**
    * @brief Load the given mod file
    *
    * @param file Path to mod file
    * @return true if mod load was successful, false otherwise
    */
    bool Load(std::string file, APIType mod_api = QMM_API_UNKNOWN);

    /**
    * @brief Unload mod file
    */
    void Unload();
}   // namespace Mod

#endif // QMM2_MOD_HPP

