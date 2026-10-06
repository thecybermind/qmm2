/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_QMM_HPP
#define QMM2_QMM_HPP

#include <string>
#include "qmmapi.h"
#include "gameapi.hpp"

// Currently-loaded game & game engine info.
namespace QMM {
    extern std::string exe_path;            // Full path of running server binary
    extern std::string exe_dir;             // Directory of running server binary
    extern std::string exe_file;            // Filename of running server binary
    extern std::string qmm_path;            // Full path of QMM dll
    extern std::string qmm_dir;             // Directory of QMM dll
    extern std::string qmm_file;            // Filename of QMM dll
    extern std::string mod_dir;             // Active mod dir
    extern std::string cfg_path;            // QMM config file path
    extern GameSupport* game;               // loaded engine from supported games table from game_api.cpp
    extern eng_syscall syscall;             // syscall from dllEntry (if applicable) to call G_ERROR if needed
    extern void* qmm_module_ptr;            // QMM module pointer
    extern bool is_auto_detected;           // Was this engine auto-detected?
    extern bool is_shutdown;                // Is the game shutting down due to G_ERROR? Used to avoid calling G_ERROR again from GAME_SHUTDOWN
    extern APIType api;                     // Engine api that QMM was loaded with

    /**
    * @brief Shared code for QMM initialization from all API entry points.
    *
    * @param import First argument to API entry point
    * @param extra Second argument to API entry point
    * @param engine APIType of engine API that was called
    * @return value to return back to the engine
    */
    void* HandleEntry(void* import, void* extra, APIType engine);

    /**
    * @brief Fill "buf" with a given argument.
    *
    * This will use G_ARGV, but supports either type: fill buffer, or return string
    *
    * @param argn Number of argument to receive
    * @param buf String buffer to fill
    * @param buflen Size of buf
    */
    void ArgV(intptr_t argn, char* buf, intptr_t buflen);

    /**
    * @brief Handle vmMain call using intptr_t* args. 
    * 
    * @param cmd Mod function to perform
    * @param args Array of cmd-specific arguments
    * @return Return value of mod call
    */
    intptr_t vmMain_args(intptr_t cmd, intptr_t* args);

    /**
    * @brief Handle syscall call using intptr_t* args
    * 
    * @param cmd Engine function to perform
    * @param args Array of cmd-specific arguments
    * @return Return value of engine call
    */
    intptr_t syscall_args(intptr_t cmd, intptr_t* args);

    /* This is used if we couldn't determine a game engine and we have to fail.
     * G_ERROR appears to be 1 in all supported dllEntry games.
     * They are different in some GetGameAPI games, but for those we just return nullptr from GetGameAPI.
     */
    constexpr int FAIL_G_ERROR = 1;

    // Store cgame passthrough stuff
    namespace CGame {
        // Store syscall pointer to pass through to the mod's dllEntry function
        extern eng_syscall syscall;
        // Store mod's vmMain function to pass vmMain calls
        extern mod_vmMain vmMain;
        // If true, GAME_SHUTDOWN has been called, but the mod DLL was kept loaded so cgame shutdown can run.
        extern bool is_shutdown;
    };

    // RAII class to read a file using engine functions
    struct EngineFileRead {
        EngineFileRead();
        ~EngineFileRead();

        /**
        * @brief Open file.
        *
        * @param path Filename to open
        * @return Pointer to contents of file
        */
        uint8_t* Open(std::string path);

        /**
        * @brief Size of file.
        *
        * @return File size
        */
        int Size();

        /**
        * @brief Close file.
        */
        void Close();
    private:
        int handle;
        std::vector<uint8_t> file;
    };

}   // namespace QMM


// Convert from QMM_G_ message to actual G_ message
#define QMM_ENG_MSG    (QMM::game->QMMEngMsg)
// Convert from QMM_GAME_ message to actual GAME_ message
#define QMM_MOD_MSG    (QMM::game->QMMModMsg)

// Call game-specific syscall handler
#define ENG_SYSCALL    (QMM::game->syscall)

#endif // QMM2_QMM_HPP
