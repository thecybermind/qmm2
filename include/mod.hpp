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
    qvm vm = {};					// QVM object
    void* dll = nullptr;			// OS DLL handle
    std::string path;				// Mod file path
    APIType api = QMM_API_ERROR;	// API the mod DLL was loaded with

    Mod();
    ~Mod();
    // delete copy constructor/assignment since a Mod object owns the DLL pointer and QVM object
    Mod(const Mod& other) = delete;
    Mod& operator=(const Mod& other) = delete;
    Mod(Mod&& other) noexcept;
    Mod& operator=(Mod&& other) noexcept;

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
    * @brief Attempt to load a QVM mod
    *
    * @param file Path to QVM mod file
    * @return true if mod load was successful, false otherwise
    */
    bool LoadQVM(std::string file);

    /**
    * @brief Attempt to initialize Mod::dll as a DLL mod with the given API type
    *
    * @param file Path to DLL mod file
    * @param handle Pointer to loaded DLL
    * @param dll_api API type to load the mod
    * @return true if mod load was successful, false otherwise
    */
    bool InitDLL(std::string file, void* handle, APIType dll_api);
};

// The game mod
extern Mod g_mod;

#endif // QMM2_MOD_H

