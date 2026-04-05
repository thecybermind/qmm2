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

#include <stdint.h>     // intptr_t and uint8_t
#include <stddef.h>     // ptrdiff_t and size_t

// Magic number is stored in file as 44 14 72 12
#define QVM_MAGIC                       0x12721444

// Magic number is stored in file as 45 14 72 12
// QVM version 2: ioQuake3 added a new version of QVM with jump targets for JIT optimization.
// There is no difference if interpreted, and extra header field can be ignored.
#define QVM_MAGIC_VER2                  0x12721445

// Amount of operands the opstack can hold (same amount used by Q3 engine)
#define QVM_OPSTACK_SIZE                1024
// Max size of program stack (this is set by q3asm for ALL QVM-compatible games) (64KiB)
#define QVM_PROGRAMSTACK_SIZE           0x10000
// Default size of temporary hunk segment (ioRTCW defaults to 2 MiB) (1MiB)
#define QVM_HUNK_SIZE                   0x100000

// Round "var" up to next power of 2: https://stackoverflow.com/a/1322548/809900
#define QVM_NEXT_POW_2(var) var--; var |= var >> 1; var |= var >> 2; var |= var >> 4; var |= var >> 8; var |= var >> 16; var++

// Add "size" bytes to program stack frame
#define QVM_STACKFRAME(size) programstack = (int*)((uint8_t*)programstack - (size))

// Pop "n" values from opstack
#define QVM_POPN(n) opstack += (n)
// Pop 1 value from opstack
#define QVM_POP()   QVM_POPN(1)
// Push "v" to opstack
#define QVM_PUSH(v) --opstack; opstack[0] = (v)

// Move instruction pointer to a given index, masked to code segment
#define QVM_JUMP(x) opptr = codesegment + ((x) & codemask)

// Conditional branches

// Conditional branch to param; compare top 2 opstack operands as signed integers
#define QVM_JUMP_SIF(o) if (opstack[1] o opstack[0]) { QVM_JUMP(param); } QVM_POPN(2)
// Conditional branch to param; compare top 2 opstack operands as unsigned integers
#define QVM_JUMP_UIF(o) if (*(unsigned int*)&opstack[1] o *(unsigned int*)&opstack[0]) { QVM_JUMP(param); } QVM_POPN(2)
// Conditional branch to param; compare top 2 opstack operands as floats
#define QVM_JUMP_FIF(o) if (*(float*)&opstack[1] o *(float*)&opstack[0]) { QVM_JUMP(param); } QVM_POPN(2)

// Math operations

// Signed integer operation; opstack[0] done to opstack[1], stored in opstack[1]
#define QVM_SOP(o) opstack[1] o opstack[0]; QVM_POP()
// Unsigned integer operation; opstack[0] done to opstack[1], stored in opstack[1]
#define QVM_UOP(o) *(unsigned int*)&opstack[1] o *(unsigned int*)&opstack[0]; QVM_POP()
// Floating point operation; opstack[0] done to opstack[1], stored in opstack[1]
#define QVM_FOP(o) *(float*)&opstack[1] o *(float*)&opstack[0]; QVM_POP()
// Signed integer operation; opstack[0] done to self
#define QVM_SSOP(o) opstack[0] = o opstack[0]
// Floating point operation; opstack[0] done to self
#define QVM_SFOP(o) *(float*)&opstack[0] = o *(float*)&opstack[0]

// Type of pointer to function that receives syscalls (engine traps) out of VM
typedef int (*qvm_syscall)(uint8_t* membase, int cmd, int* args);

// QVM instructions
typedef enum {
    QVM_OP_UNDEF,       // Undefined (error)
    QVM_OP_NOP,         // No-op
    QVM_OP_BREAK,       // Break to debugger (unused, treated as no-op)
    QVM_OP_ENTER,       // Enter a function, increase program stack by param bytes
    QVM_OP_LEAVE,       // Leave a function, decrease program stack by param bytes
    QVM_OP_CALL,        // Store IP in program stack, jump to o1
    QVM_OP_PUSH,        // Push 0 onto opstack
    QVM_OP_POP,         // Pop o1 from opstack
    QVM_OP_CONST,       // Push param onto opstack
    QVM_OP_LOCAL,       // Push address of paramth value of program stack onto opstack
    QVM_OP_JUMP,        // Jump to o1
    QVM_OP_EQ,          // Conditional jump to param, if o1 == o2
    QVM_OP_NE,          // Conditional jump to param, if o1 != o2
    QVM_OP_LTI,         // Conditional jump to param, if o1 < o2
    QVM_OP_LEI,         // Conditional jump to param, if o1 <= o2
    QVM_OP_GTI,         // Conditional jump to param, if o1 > o2
    QVM_OP_GEI,         // Conditional jump to param, if o1 >= o2
    QVM_OP_LTU,         // Conditional jump to param, if o1 < o2 (unsigned)
    QVM_OP_LEU,         // Conditional jump to param, if o1 <= o2 (unsigned)
    QVM_OP_GTU,         // Conditional jump to param, if o1 > o2 (unsigned)
    QVM_OP_GEU,         // Conditional jump to param, if o1 >= o2 (unsigned)
    QVM_OP_EQF,         // Conditional jump to param, if o1 == o2 (float)
    QVM_OP_NEF,         // Conditional jump to param, if o1 != o2 (float)
    QVM_OP_LTF,         // Conditional jump to param, if o1 < o2 (float)
    QVM_OP_LEF,         // Conditional jump to param, if o1 <= o2 (float)
    QVM_OP_GTF,         // Conditional jump to param, if o1 > o2 (float)
    QVM_OP_GEF,         // Conditional jump to param, if o1 >= o2 (float)
    QVM_OP_LOAD1,       // Load 1-byte value at address o1, store in o1
    QVM_OP_LOAD2,       // Load 2-byte value at address o1, store in o1
    QVM_OP_LOAD4,       // Load 4-byte value at address o1, store in o1
    QVM_OP_STORE1,      // Store 1-byte value in o1 into address o2 
    QVM_OP_STORE2,      // Store 2-byte value in o1 into address o2
    QVM_OP_STORE4,      // Store 4-byte value in o1 into address o2
    QVM_OP_ARG,         // Store o1 in paramth value of program stack
    QVM_OP_BLOCK_COPY,  // Copy param bytes from address o1 to address o2
    QVM_OP_SEX8,        // Sign extension of 8-bit value in o1
    QVM_OP_SEX16,       // Sign extension of 16-bit value in o2
    QVM_OP_NEGI,        // Math operation, o1 = -o1
    QVM_OP_ADD,         // Math operation, o2 += o1
    QVM_OP_SUB,         // Math operation, o2 -= o1
    QVM_OP_DIVI,        // Math operation, o2 /= o1
    QVM_OP_DIVU,        // Math operation, o2 /= o1 (unsigned)
    QVM_OP_MODI,        // Math operation, o2 %= o1
    QVM_OP_MODU,        // Math operation, o2 %= o1 (unsigned)
    QVM_OP_MULI,        // Math operation, o2 *= o1
    QVM_OP_MULU,        // Math operation, o2 *= o1 (unsigned)
    QVM_OP_BAND,        // Math operation, o2 &= o1
    QVM_OP_BOR,         // Math operation, o2 |= o1
    QVM_OP_BXOR,        // Math operation, o2 ^= o1
    QVM_OP_BCOM,        // Math operation, o1 = ~o1
    QVM_OP_LSH,         // Math operation, o2 <<= o1 (unsigned)
    QVM_OP_RSHI,        // Math operation, o2 >>= o1
    QVM_OP_RSHU,        // Math operation, o2 >>= o1 (unsigned)
    QVM_OP_NEGF,        // Math operation, o1 = -o1 (float)
    QVM_OP_ADDF,        // Math operation, o2 += o1 (float)
    QVM_OP_SUBF,        // Math operation, o2 -= o1 (float)
    QVM_OP_DIVF,        // Math operation, o2 /= o1 (float)
    QVM_OP_MULF,        // Math operation, o2 *= o1 (float)
    QVM_OP_CVIF,        // Math operation, convert int o1 to float
    QVM_OP_CVFI,        // Math operation, convert float o1 to int

    QVM_OP_NUM_OPS,     // Number of QVM opcodes
} qvm_opcode;

// Array of strings of opcode names
extern const char* qvm_opcodename[];

// A single instruction in memory
typedef struct {
    qvm_opcode op;              // Opcode
    int param;                  // Immediate
} qvm_op;

// QVM file header
typedef struct {
    uint32_t magic;             // Magic number
    uint32_t instructioncount;  // Number of instructions in code segment
    uint32_t codeoffset;        // Offset of code segment
    uint32_t codelen;           // Length of code segment
    uint32_t dataoffset;        // Offset of data segment
    uint32_t datalen;           // Length of data segment
    uint32_t litlen;            // Length of lit segment
    uint32_t bsslen;            // Length of bss segment
    // uint32_t jtrglen;        // Length of jump target table in version 2, only used by ioQuake3 for JIT optimization
} qvm_header;

// Allocator type for custom allocation
typedef struct {
    void* (*alloc)(ptrdiff_t size, void* ctx);              // Function to allocate 'size' bytes, given 'ctx' context pointer
    void  (*free)(void* ptr, ptrdiff_t size, void* ctx);    // Function to free 'size' bytes existing at 'ptr', given 'ctx' context pointer
    void* ctx;                                              // Context pointer
} qvm_alloc;

// Default VM allocator (uses malloc/free)
extern qvm_alloc qvm_allocator_default;

// All the info for a single QVM object
typedef struct {
    uint32_t magic;                 // Magic number from QVM

    qvm_syscall syscall;            // Function that will handle syscalls and adjust pointer arguments

    uint8_t* memory;                // Main block of memory
    size_t memorysize;              // Size of memory block

    qvm_op* codesegment;            // Start of code segment, each op is 8 bytes (4 op, 4 param)
    uint8_t* datasegment;           // Start of data segment, partially filled on load

    size_t instructioncount;        // Number of instructions, from QVM header
    size_t codeseglen;              // Size of code segment in memory
    size_t dataseglen;              // Size of entire data segment in memory

    size_t stacksize;               // Size of program stack in bss segment
    int* stacklow;                  // Pointer to lowest address of program stack
    int* stackhigh;                 // Pointer to highest address of program stack

    size_t hunksize;                // Size of the hunk
    int hunklow;                    // Offset of lowest address of hunk
    int hunkhigh;                   // Offset of highest address of hunk

    int* stackptr;                  // Pointer to current location in program stack
    int hunkptr;                    // Offset of current location in hunk

    size_t filesize;                // .qvm file size
    qvm_alloc* allocator;           // Allocator
    int verify_data;                // Verify data access is inside the memory block
} qvm;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
* @brief Create and initialize a new VM from a QVM file.
* 
* @param vm Pointer to QVM object to store VM information
* @param filemem Buffer with QVM file contents
* @param filesize Size of the filemem buffer
* @param qvmsyscall Function to be called for engine traps
* @param verify_data Should data segment reads and writes be validated?
* @param hunk_size Size of hunk in VM data segment
* @param allocator Pointer to a qvm_alloc object which contains custom alloc/free function pointers (pass NULL for default)
* @return 1 if success, 0 if failure
*/
int qvm_load(qvm* vm, const uint8_t* filemem, size_t filesize, qvm_syscall qvmsyscall, int verify_data, size_t hunk_size, qvm_alloc* allocator);

/**
* @brief Begin execution in a VM at the start of the code segment.
*
* @param vm Pointer to QVM object to execute
* @param argc Number of arguments to pass to VM entry point
* @param argv Array of arguments to pass to VM entry point
* @return Return value from VM entry point
*/
int qvm_exec(qvm* vm, int argc, int* argv);

/**
* @brief Begin execution in a VM at a given instruction.
*
* @param vm Pointer to QVM object to execute
* @param instruction Instruction to begin execution at
* @param argc Number of arguments to pass to VM entry point
* @param argv Array of arguments to pass to VM entry point
* @return Return value from VM entry point
*/
int qvm_exec_ex(qvm* vm, size_t instruction, int argc, int* argv);

/**
* @brief Unload a VM.
*
* @param qvm Pointer to QVM object to unload
*/
void qvm_unload(qvm* vm);

/**
* @brief Allocate memory in a VM's hunk.
* 
* @param vm Pointer to QVM object to allocate memory in
* @param size Size of block to allocate
* @param init Initial data to copy into allocation
* @return VM-based pointer to allocated block
*/
int qvm_hunk_alloc(qvm* vm, size_t size, const void* init);

/**
* @brief Free memory in a VM's hunk.
* 
* Since the hunk is stack allocated, you can only truly free a block if it was the most recently allocated block.
* 
* This function silently fails if the provided block is not the most recent.
* 
* @param vm Pointer to QVM object to allocate memory in
* @param ptr VM-based pointer to allocated block
* @param size Size of block that was allocated
* @param out Data to copy out of the allocation
*/
void qvm_hunk_free(qvm* vm, int ptr, size_t size, void* out);

/**
* @brief Dump VM memory/stacks.
* 
* @param vm Pointer to QVM object
* @param opstack Current opstack pointer
* @param opstackhigh End of opstack
* @param instruction Current instruction pointer
*/
void qvm_dump(qvm* vm, int* opstack, int* opstackhigh, qvm_op* instruction);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // QMM2_QVM_H
