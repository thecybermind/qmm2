/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_QVM_H__
#define __QMM2_QVM_H__

#include <cstdint>
#include <vector>

// magic number is stored in file as 44 14 72 12
#define	QVM_MAGIC       0x12721444

#define QMM_MAX_SYSCALL_ARGS_QVM	13	// change whenever a QVM mod has a bigger syscall list

typedef int (*vmsyscall_t)(uint8_t* membase, int cmd, int* args);

enum qvmopcode_t {
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
    OP_CVFI,
};

extern const char* opcodename[];

// a single opcode in memory
struct qvmop_t {
    qvmopcode_t op;
    int param;
};

struct qvmheader_t {
    int magic;
    unsigned int numops;
    unsigned int codeoffset;
    unsigned int codelen;
    unsigned int dataoffset;
    unsigned int datalen;
    unsigned int litlen;
    unsigned int bsslen;
};

// all the info for a single QVM object
struct qvm_t {
    qvmheader_t header = {};		// header information

    // extra
    size_t filesize = 0;			// .qvm file size

    // memory
    std::vector<uint8_t> memory;	// main block of memory

    // segments (into memory vector)
    qvmop_t* codesegment = nullptr;	// code segment, each op is 8 bytes (4 op, 4 param)
    uint8_t* datasegment = nullptr;	// data segment, partially filled on load
    uint8_t* stacksegment = nullptr;// stack segment

    // segment sizes
    unsigned int codeseglen = 0;	// size of code segment
    unsigned int dataseglen = 0;	// size of data segment
    unsigned int stackseglen = 0;	// size of stack segment

    // "registers"
    qvmop_t* opptr = nullptr;		// current op in code segment
    int* stackptr = nullptr;		// pointer to current location in stack
    int argbase = 0;				// lower end of arg heap

    // syscall
    vmsyscall_t vmsyscall = nullptr;// e.g. Q3A_vmsyscall function from game_q3a.cpp

    bool verify_data = true;		// verify data access is inside the memory block
};

// entry point for qvms (given to plugins to call for qvm mods)
bool qvm_load(qvm_t& qvm, const uint8_t* filemem, unsigned int filesize, vmsyscall_t vmsyscall, unsigned int stacksize, bool verify_data);
void qvm_unload(qvm_t& qvm);
int qvm_exec(qvm_t& qvm, int argc, int* argv);

#endif // __QMM2_QVM_H__
