/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_UTIL_H
#define QMM2_UTIL_H

#include <vector>
#include <string>
#include <cstddef>		// size_t


// ---------------------------------
// ----- Path parsing/handling -----
// ---------------------------------

/**
* @brief Normalizes path.
* 
* This includes collapsing . & .., using / for path separator, etc.
*
* @param path Path
* @return Normalized path
*/
std::string path_normalize(std::string path);

/**
* @brief Is the path allowed?
*
* This means a relative path to a QVM, or an absolute path within exe_dir or qmm_dir.
*
* @param path Path
* @return true if path is allowed, false otherwise
*/
bool path_is_allowed(std::string path);

/**
* @brief Gets the directory name component of a path.
*
* @param path Path
* @return Directory name component of path
*/
std::string path_dirname(std::string path);

/**
* @brief Gets the filename (or topmost) component of a path.
*
* @param path Path
* @return Filename component of path
*/
std::string path_basename(std::string path);

/**
* @brief Gets the file extension component of a path.
*
* @param path Path
* @return File extension component of path
*/
std::string path_baseext(std::string path);

/**
* @brief Is the path absolute?
*
* @param path Path
* @return true if path is absolute, false otherwise
*/
bool path_is_absolute(std::string path);

/**
* @brief Is the path relative?
*
* @param path Path
* @return true if path is relative, false otherwise
*/
bool path_is_relative(std::string path);

/**
* @brief Creates a directory including all missing parent directories.
*
* @param path Path
*/
void path_mkdir(std::string path);

/**
* @brief Look for the given argument in the command line, and return the next argv.
* 
* E.g. if the command line was:
* 
*   "quake3 --qmm_cfg test.json +set dedicated 1 +map q3dm1"
*
* and "arg" was "--qmm_cfg" then this function returns "test.json".
* 
* If an argv matching "arg" does not exist, it returns "def"
* 
* @param arg Command line argument to look for
* @param def Default return if arg is not found
* @return argv following the found arg, or def
*/
std::string util_get_cmdline_arg(std::string arg, std::string def = "");

/**
* @brief Gets the path of the process' executable.
*
* @return Path of executable
*/
const char* util_get_proc_path();

/**
* @brief Gets the path of the QMM DLL.
*
* @return Path of QMM DLL
*/
const char* util_get_qmm_path();

/**
* @brief Gets the module handle of the QMM DLL.
*
* @return Module handle of QMM
*/
void* util_get_qmm_handle();

/**
* @brief Gets the number of milliseconds since the first call (in GAME_INIT).
* 
* This is used for G_MILLISECONDS polyfills in Q2R, QUAKE2, and SIN.
*
* @return Milliseconds since first call
*/
intptr_t util_get_milliseconds();


// ---------------------------
// ----- DLL interaction -----
// ---------------------------

/**
* @brief Load a DLL into the process.
* 
* This calls LoadLibraryA in Windows, and dlopen in linux.
*
* @param filename Path to DLL
* @return handle for loaded module
*/
void* dll_load(const char* filename);

/**
* @brief Get a named function/variable from a given DLL.
*
* This calls GetProcAddress in Windows, and dlsym in linux.
*
* @param dll Module handle for DLL
* @param symbol Name of function/variable to lookup
* @return pointer to function/variable
*/
void* dll_symbol(void* dll, const char* symbol);

/**
* @brief Unloads a DLL from the process.
*
* This calls FreeLibrary in Windows, and dlclose in linux.
*
* @param dll Module handle for DLL
* @return true if unload successful, false otherwise
*/
bool dll_close(void* dll);

/**
* @brief Gets the most recent DLL error.
*
* This calls GetLastError in Windows, and dlerror in linux.
*
* @return most recent DLL error message
*/
const char* dll_error();

// -----------------------------
// ----- String comparison -----
// -----------------------------

/**
* @brief Convert string to lowercase.
*
* @param str String to convert
* @return converted string
*/
std::string str_tolower(std::string str);

/**
* @brief Convert string to uppercase.
*
* @param str String to convert
* @return converted string
*/
std::string str_toupper(std::string str);

/**
* @brief Case-insensitive search for a string inside a another string.
*
* @param haystack String to search in
* @param needle String to search for
* @return true if needle is in haystack, false otherwise
*/
bool str_stristr(std::string haystack, std::string needle);

/**
* @brief Case-insensitive string comparison.
*
* @param s1 String to compare
* @param s2 String to compare
* @return 0 if s1 and s2 compare equal, less-than-zero if s1 compares less than s2, or greater-than-zero if s1 compares greater than s2
*/
int str_stricmp(std::string s1, std::string s2);

/**
* @brief Case-insensitive string comparison.
*
* @param s1 String to compare
* @param s2 String to compare
* @return true if s1 compares equal to s2, false otherwise
*/
bool str_striequal(std::string s1, std::string s2);

/**
* @brief A "safe" strncpy that always null-terminates.
*
* @param dest Buffer to write to
* @param src Buffer to read from
* @param count Size of dest buffer
* @return same pointer as dest
*/
char* strncpyz(char* dest, const char* src, size_t count);

/**
* @brief Tokenizes an entstring into a vector of strings.
*
* @param entstring Entity string from engine
* @return vector of strings where each element is an entity token from entstring
*/
std::vector<std::string> util_parse_entstring(std::string entstring);

/**
* @brief Returns whichever value is greater - a typical "max" function.
* 
* This was created to avoid any overlap with a "max" function from stdlib or game SDKs.
*
* @param T any type
* @param a an object of type T
* @param b an object of type T
* @return whichever of a or b compares greater
*/
template<typename T>
T util_max(T a, T b) {
    return (a > b ? a : b);
}


template <class OutputClass, class InputClass>
union horrible_union {
    OutputClass out;
    InputClass in;
};
// Same as reinterpret_cast<T>(x) but for anything
template <class OutputClass, class InputClass>
inline OutputClass horrible_cast(const InputClass input) {
    horrible_union<OutputClass, InputClass> u;
    u.in = input;
    return u.out;
}


#endif // QMM2_UTIL_H
