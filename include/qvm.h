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

#include <stdint.h>
#include <stddef.h>

// magic number is stored in file as 44 14 72 12
#define	QVM_MAGIC       0x12721444

// amount of ints the opstack can hold
#define OPSTACK_SIZE    1024

// function to receive syscalls (engine traps) out of VM
typedef int (*vmsyscall_t)(uint8_t* membase, int cmd, int* args);

// list of VM instructions
typedef enum qvmopcode_e {
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

    OP_NUM_OPS,
} qvmopcode_t;

// array of strings of opcode names
extern const char* opcodename[];

// a single opcode in memory
typedef struct qvmop_s {
    qvmopcode_t op;
    int param;
} qvmop_t;

// QVM file header
typedef struct qvmheader_s {
    uint32_t magic;
    uint32_t numops;
    uint32_t codeoffset;
    uint32_t codelen;
    uint32_t dataoffset;
    uint32_t datalen;
    uint32_t litlen;
    uint32_t bsslen;
} qvmheader_t;

// allocator type for custom allocation
typedef struct qvm_alloc_s {
    void* (*alloc)(ptrdiff_t size, void* ctx);
    void  (*free)(void* ptr, ptrdiff_t size, void* ctx);
    void* ctx;
} qvm_alloc_t;

// default vm allocator (uses malloc/free)
extern qvm_alloc_t qvm_allocator_default;

// all the info for a single QVM object
typedef struct qvm_s {
    qvmheader_t header;             // header information

    // syscall
    vmsyscall_t vmsyscall;          // e.g. Q3A_vmsyscall function from game_q3a.cpp

    // memory
    uint8_t* memory;                // main block of memory
    size_t memorysize;              // size of memory block

    // segments (into memory block)
    qvmop_t* codesegment;           // code segment, each op is 8 bytes (4 op, 4 param)
    uint8_t* datasegment;           // data segment, partially filled on load
    uint8_t* stacksegment;          // stack segment

    // segment sizes
    size_t codeseglen;              // size of code segment
    size_t dataseglen;              // size of data segment
    size_t stackseglen;	            // size of stack segment

    // registers
    int* stackptr;                  // pointer to current location in program stack

    // extra
    size_t filesize;                // .qvm file size
    qvm_alloc_t* allocator;         // allocator
    int verify_data;                // verify data access is inside the memory block
    int exec_depth;                 // current depth of recursive execution
} qvm_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
* Create and initialize a new VM from a QVM file
* 
* @param [qvm_t*] qvm - Pointer to qvm_t object to store VM information
* @param [const uint8_t*] filemem - Buffer with QVM file contents
* @param [size_t] filesize - Size of the filemem buffer
* @param [vmsyscall_t] vmsyscall - Function to be called for engine traps
* @param [size_t] stacksize - Size of QVM program stack in MiB
* @param [int] verify_data - (Boolean) Should data segment reads and writes be validated?
* @param [qvm_alloc_t*] allocator - Pointer to a qvm_alloc_t object which contains custom alloc/free function pointers (pass NULL for default)
* @returns [int] - (Boolean) 1 if success, 0 if failure
*/
int qvm_load(qvm_t* qvm, const uint8_t* filemem, size_t filesize, vmsyscall_t vmsyscall, size_t stacksize, int verify_data, qvm_alloc_t* allocator);

/**
* Begin execution in a VM
*
* @param [qvm_t*] qvm - Pointer to qvm_t object to execute
* @param [int] argc - Number of arguments to pass to VM entry point
* @param [int*] argv - Array of arguments to pass to VM entry point
* @returns [int] - Return value from VM entry point
*/
int qvm_exec(qvm_t* qvm, int argc, int* argv);

/**
* Unload a VM
*
* @param [qvm_t*] qvm - Pointer to qvm_t object to unload
*/
void qvm_unload(qvm_t* qvm);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __QMM2_QVM_H__
