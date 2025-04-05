/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_QVM_H__
#define __QMM2_QVM_H__

// magic number is stored in file as 44 14 72 12
#define	QVM_MAGIC       0x12721444

#define QMM_MAX_SYSCALL_ARGS_QVM	13	// change whenever a QVM mod has a bigger syscall list

typedef unsigned char byte;
typedef int (*vmsyscall_t)(unsigned char* membase, int cmd, int* args);

typedef enum {
	OP_UNDEF,
	OP_NOP,
	OP_BREAK,
	OP_ENTER,
	OP_LEAVE,
	OP_CALL,
	OP_PUSH,
	OP_POP,
	OP_CONST,
	OP_LOCAL,
	OP_JUMP,
	OP_EQ,
	OP_NE,
	OP_LTI,
	OP_LEI,
	OP_GTI,
	OP_GEI,
	OP_LTU,
	OP_LEU,
	OP_GTU,
	OP_GEU,
	OP_EQF,
	OP_NEF,
	OP_LTF,
	OP_LEF,
	OP_GTF,
	OP_GEF,
	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,
	OP_ARG,
	OP_BLOCK_COPY,
	OP_SEX8,
	OP_SEX16,
	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,
	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,
	OP_LSH,
	OP_RSHI,
	OP_RSHU,
	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,
	OP_CVIF,
	OP_CVFI
} qvmopcode_t;

extern const char* opcodename[];

// a single opcode in memory
typedef struct {
	qvmopcode_t op;
	int param;
} qvmop_t;

typedef struct {
	int magic;
	unsigned int numops;
	unsigned int codeoffset;
	unsigned int codelen;
	unsigned int dataoffset;
	unsigned int datalen;
	unsigned int litlen;
	unsigned int bsslen;
} qvmheader_t;

// all the info for a single QVM object
typedef struct {
	qvmheader_t header;			// header information

	// extra
	unsigned int filesize;		// .qvm file size

	// memory
	byte* memory;				// main block of memory
	unsigned int memorysize;	// size of memory block

	// segments
	qvmop_t* codesegment;		// code segment, each op is 8 bytes (4 op, 4 param)
	byte* datasegment;			// data segment, partially filled on load
	byte* stacksegment;			// stack segment

	// segment sizes
	unsigned int codeseglen;	// size of code segment
	unsigned int dataseglen;	// size of data segment
	unsigned int stackseglen;	// size of stack segment

	// "registers"
	qvmop_t* opptr;				// current op in code segment
	int* stackptr;				// pointer to current location in stack
	int argbase;				// lower end of arg heap

	// syscall
	vmsyscall_t vmsyscall;		// e.g. Q3A_vmsyscall function from game_q3a.cpp
} qvm_t;

// entry point for qvms (given to plugins to call for qvm mods)
bool qvm_load(qvm_t* qvm, byte* filemem, unsigned int filelen, vmsyscall_t vmsyscall, unsigned int stacksize);
void qvm_unload(qvm_t* qvm);
int qvm_exec(qvm_t* qvm, int argc, int* argv);

// easier to change allocation later
void* qvm_malloc(size_t len);
void qvm_free(void*);

#endif // __QMM2_QVM_H__
