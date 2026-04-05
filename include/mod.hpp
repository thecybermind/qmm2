/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_MOD_H
#define QMM2_MOD_H

#include <string>
#include "gameapi.hpp"
#include "qvm.h"

// A game mod
struct Mod {
    std::string path;				// Mod file path
    qvm vm = {};					// QVM object
    void* dll = nullptr;			// OS DLL handle
    intptr_t vmbase = 0;			// Base data segment address for QVMs (0 if DLL mod)
    APIType api = QMM_API_ERROR;	// API the mod DLL was loaded with

    /**
    * @brief Load the given mod file
    *
    * @param file Path to mod file
    * @return true if mod load was successful, false otherwise
    */
    bool Load(std::string file);

    /**
    * @brief Unload mod file
    */
    void Unload();

private:
    // Entry point into QVM mods. Passed to GameSupport::Entry
    static intptr_t QVM_vmMain(intptr_t cmd, ...);

    // Exit point from QVM mods. Handle syscalls from the QVM.
    static int QVM_syscall(uint8_t* membase, int cmd, int* args);

    /**
    * @brief Attempt to load Mod::file a QVM mod
    *
    * @return true if mod load was successful, false otherwise
    */
    bool LoadQVM();

    /**
    * @brief Attempt to load Mod::file as a DLL mod with the given API type
    *
    * @param dll_api API type to load the mod
    * @return true if mod load was successful, false otherwise
    */
    bool LoadDLL(APIType dll_api);
};

// The game mod
extern Mod g_mod;

#endif // QMM2_MOD_H

