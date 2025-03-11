/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_FORMAT_H__
#define __QMM2_FORMAT_H__

#ifdef WIN32
// Ill-defined for-loop.  Loop body not executed.
#pragma warning(disable:6294)
// The function 'X' is constexpr, mark variable 'X' constexpr if compile-time evaluation is desired (con.5).
#pragma warning(disable:26498)
// Variable 'X' is uninitialized. Always initialize a member variable (type.6).
#pragma warning(disable:26495)
// Inconsistent annotation for 'X': this instance has no annotations.
#pragma warning(disable:28251)
#endif

#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/format.h"

#endif // __QMM2_FORMAT_H__
