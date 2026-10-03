/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_GAME_Q2RC_H
#define QMM2_GAME_Q2RC_H

#include <q2r/rerelease/game.h>

// export ("vmMain") cmds
enum {
    CGAME_INIT,
    CGAME_SHUTDOWN,

    CGAME_CONSOLE_COMMAND = -100,    // doesn't exist, but QMM needs a value to compare
};

// import ("syscall") cmds that QMM needs values for
enum {
    CG_PRINT,
    CG_ERROR,
    CG_ARGV,
    CG_ARGC,
    CG_SEND_CONSOLE_COMMAND,
    CG_GET_CONFIGSTRING,
    CG_CVAR_REGISTER,
    CG_CVAR_VARIABLE_STRING_BUFFER,
    CG_CVAR_VARIABLE_INTEGER_VALUE,
    CG_FS_FOPEN_FILE,
    CG_FS_READ,
    CG_FS_WRITE,
    CG_FS_FCLOSE_FILE,
};

// other values
enum {
    // not used with Q2RC
    EXEC_APPEND,
    // file flags
    FS_READ,
    FS_WRITE,
    FS_APPEND,
    FS_APPEND_SYNC = FS_APPEND,
    // used by qmm_version cvar
    CVAR_ROM = CVAR_NOSET // 8
};

#endif // QMM2_GAME_Q2RC_H
