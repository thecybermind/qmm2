/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_CVMMOD_H__
#define __QMM2_CVMMOD_H__

#include <string>
#include "CMod.h"
#include "qmm.h"
#include "qvm.h"

class CVMMod : public CMod {
	public:
		CVMMod();
		~CVMMod();

		int LoadMod(std::string file);

		int vmMain(int, int, int, int, int, int, int, int, int, int, int, int, int);

		int IsVM();
		std::string& File();

		int GetBase();

		void Status();

	private:
		std::string file;

		//segments
		qvmop_t* codesegment;	//code segment, each op is 8 bytes (4 op, 4 param)
		byte* datasegment;	//data segment, partially filled on load
		byte* stacksegment;	//stack segment

		//segment sizes
		int codeseglen;		//size of code segment
		int dataseglen;		//size of data segment
		int stackseglen;	//size of stack segment

		//"registers"
		qvmop_t* opptr;		//current op in code segment
		int* stackptr;		//pointer to current location in stack
		int argbase;		//lower end of arg heap

		//memory
		byte* memory;		//main block of memory
		int memorysize;		//size of memory block

		//extra
		int swapped;		//was this file byteswapped? (is the server big-endian)
		int filesize;		//.qvm file size

		qvmheader_t header;	//header information
};

#endif //__QMM2_CVMMOD_H__
