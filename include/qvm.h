/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_QVM_H
#define QMM2_QVM_H

#include <stdint.h>
#include <stddef.h>

// macros used throughout qvm_load or qvm_exec

// magic number is stored in file as 44 14 72 12
#define QVM_MAGIC                       0x12721444
// amount of operands the opstack can hold (same amount used by Q3 engine)
#define QVM_OPSTACK_SIZE                1024
// max size of program stack (this is set by q3asm for ALL QVM-compatible games)
#define QVM_PROGRAMSTACK_SIZE           0x10000     // 64KiB
// allow extra space to be allocated to the data segment for additional stack space
#define QVM_EXTRA_PROGRAMSTACK_SIZE     0

// round number up to next power of 2: https://stackoverflow.com/a/1322548/809900
#define QVM_NEXT_POW_2(var) var--; var |= var >> 1; var |= var >> 2; var |= var >> 4; var |= var >> 8; var |= var >> 16; var++;

// macro to manage stack frame in bytes
#define QVM_STACKFRAME(size) programstack = (int*)((uint8_t*)programstack - (size))

// macros to manage opstack
#define QVM_POPN(n) opstack += (n)
#define QVM_POP()   QVM_POPN(1)
#define QVM_PUSH(v) --opstack; opstack[0] = (v)

// move instruction pointer to a given index, masked to code segment
#define QVM_JUMP(x) opptr = qvm->codesegment + ((x) & codemask)

// branch comparisons
// signed integer comparison
#define QVM_JUMP_SIF(o) if (opstack[1] o opstack[0]) { QVM_JUMP(param); } QVM_POPN(2)
// unsigned integer comparison
#define QVM_JUMP_UIF(o) if (*(unsigned int*)&opstack[1] o *(unsigned int*)&opstack[0]) { QVM_JUMP(param); } QVM_POPN(2)
// floating point comparison
#define QVM_JUMP_FIF(o) if (*(float*)&opstack[1] o *(float*)&opstack[0]) { QVM_JUMP(param); } QVM_POPN(2)

// math operations
// signed integer (opstack[0] done to opstack[1], stored in opstack[1])
#define QVM_SOP(o) opstack[1] o opstack[0]; QVM_POP()
// unsigned integer (opstack[0] done to opstack[1], stored in opstack[1])
#define QVM_UOP(o) *(unsigned int*)&opstack[1] o *(unsigned int*)&opstack[0]; QVM_POP()
// floating point (opstack[0] done to opstack[1], stored in opstack[1])
#define QVM_FOP(o) *(float*)&opstack[1] o *(float*)&opstack[0]; QVM_POP()
// signed integer (done to self)
#define QVM_SSOP(o) opstack[0] = o opstack[0]
// floating point (done to self)
#define QVM_SFOP(o) *(float*)&opstack[0] = o *(float*)&opstack[0]



// function to receive syscalls (engine traps) out of VM
typedef int (*vmsyscall_t)(uint8_t* membase, int cmd, int* args);

// list of VM instructions
typedef enum qvmopcode_e {
    QVM_OP_UNDEF,
    QVM_OP_NOP,
    QVM_OP_BREAK,
    QVM_OP_ENTER,
    QVM_OP_LEAVE,
    QVM_OP_CALL,
    QVM_OP_PUSH,
    QVM_OP_POP,
    QVM_OP_CONST,
    QVM_OP_LOCAL,
    QVM_OP_JUMP,
    QVM_OP_EQ,
    QVM_OP_NE,
    QVM_OP_LTI,
    QVM_OP_LEI,
    QVM_OP_GTI,
    QVM_OP_GEI,
    QVM_OP_LTU,
    QVM_OP_LEU,
    QVM_OP_GTU,
    QVM_OP_GEU,
    QVM_OP_EQF,
    QVM_OP_NEF,
    QVM_OP_LTF,
    QVM_OP_LEF,
    QVM_OP_GTF,
    QVM_OP_GEF,
    QVM_OP_LOAD1,
    QVM_OP_LOAD2,
    QVM_OP_LOAD4,
    QVM_OP_STORE1,
    QVM_OP_STORE2,
    QVM_OP_STORE4,
    QVM_OP_ARG,
    QVM_OP_BLOCK_COPY,
    QVM_OP_SEX8,
    QVM_OP_SEX16,
    QVM_OP_NEGI,
    QVM_OP_ADD,
    QVM_OP_SUB,
    QVM_OP_DIVI,
    QVM_OP_DIVU,
    QVM_OP_MODI,
    QVM_OP_MODU,
    QVM_OP_MULI,
    QVM_OP_MULU,
    QVM_OP_BAND,
    QVM_OP_BOR,
    QVM_OP_BXOR,
    QVM_OP_BCOM,
    QVM_OP_LSH,
    QVM_OP_RSHI,
    QVM_OP_RSHU,
    QVM_OP_NEGF,
    QVM_OP_ADDF,
    QVM_OP_SUBF,
    QVM_OP_DIVF,
    QVM_OP_MULF,
    QVM_OP_CVIF,
    QVM_OP_CVFI,

    QVM_OP_NUM_OPS,
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
    uint32_t instructioncount;
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
    // syscall
    vmsyscall_t vmsyscall;          // e.g. Q3A_vmsyscall function from game_q3a.cpp

    // memory
    uint8_t* memory;                // main block of memory
    size_t memorysize;              // size of memory block

    // segments (into memory block)
    qvmop_t* codesegment;           // code segment, each op is 8 bytes (4 op, 4 param)
    uint8_t* datasegment;           // data segment, partially filled on load

    // segment sizes
    size_t instructioncount;        // number of instructions, from qvm header
    size_t codeseglen;              // size of code segment
    size_t dataseglen;              // size of data segment
    size_t stacksize;               // size of program stack in bss segment
    int* stacklow;                  // pointer to lowest address of program stack
    int* stackhigh;                 // pointer to highest address of program stack

    // registers
    int* stackptr;                  // pointer to current location in program stack

    // extra
    size_t filesize;                // .qvm file size
    qvm_alloc_t* allocator;         // allocator
    int verify_data;                // verify data access is inside the memory block
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
int qvm_load(qvm_t* qvm, const uint8_t* filemem, size_t filesize, vmsyscall_t vmsyscall, int verify_data, qvm_alloc_t* allocator);

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

#endif // QMM2_QVM_H
