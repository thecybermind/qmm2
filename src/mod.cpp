/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include "mod.h"

mod_t g_mod;

/* todo:
   * use extension and/or load with engine file functions to determine mod type
   * if qvm, create a qvm object, set g_mod.pfnvmMain to the qvm entry point function
   * if dll, load normal crap, set g_mod.pfnvmMain to the actual dll vmMain function
   * when mod should be unloaded, set to nullptr
   
   also need to handle code for loading from different directories:
   if cfg "mod" exists but is not relative path, load it directly with no fallback
   if cfg "mod" exists but is relative path:
    * check qmm dir + "<mod>"
	* check exe dir + "<moddir>/<mod>"
	* check "./<moddir>/<mod>"
   if cfg "mod" doesn't exist:
	* check qmm dir + "qmm_<dllname>"
	* check exe dir + "<moddir>/qmm_<dllname>"
	* check exe dir + "<moddir>/<dllname>" (as long as exe dir is not same as qmm dir)
	* check "./<moddir>/qmm_<dllname>"


*/