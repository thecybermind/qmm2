/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#include "osdef.h"
#include "version.h"

#ifdef WIN32
#include <stdlib.h>
#pragma comment(exestr, "\1Q3 MultiMod 2 - QMM v" QMM_VERSION " (" QMM_OS ")")
#pragma comment(exestr, "\1Built: " QMM_COMPILE " by " QMM_BUILDER)
#pragma comment(exestr, "\1URL: https://github.com/thecybermind/qmm2")

char* dlerror() {
    static char buffer[4096 * 2]; // https://stackoverflow.com/a/75644008
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
    //remove newlines at the end
    buffer[strlen(buffer) - 2] = '\0';

    return buffer;
}

#else // linux

volatile const char* binary_comment[] = { "\1Q3 MultiMod 2 - QMM v" QMM_VERSION " (" QMM_OS ")",
                                          "\1Built: " QMM_COMPILE " by " QMM_BUILDER ",
                                          "\1URL: https://github.com/thecybermind/qmm2"
};

#endif
