/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "qvm.h"
#include "log.h"


static bool qvm_validate_ptr(qvm_t* qvm, void* ptr, void* start, void* end);
static bool qvm_validate_ptr_data(qvm_t* qvm, void* ptr);
static bool qvm_validate_ptr_code(qvm_t* qvm, void* ptr);
static bool qvm_validate_ptr_stack(qvm_t* qvm, void* ptr);
static bool qvm_validate_ptr_argstack(qvm_t* qvm, void* ptr);


bool qvm_load(qvm_t* qvm, const uint8_t* filemem, size_t filesize, vmsyscall_t vmsyscall, size_t stacksize, bool verify_data, qvm_alloc_t* allocator) {
    if (!qvm || qvm->memory || !filemem || !filesize || !vmsyscall)
        return false;

    const uint8_t* codeoffset = NULL;

    if (filesize < sizeof(qvmheader_t)) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: too small for header\n");
        goto fail;
    }
    
    // store args
    qvm->filesize = filesize;
    qvm->vmsyscall = vmsyscall;
    qvm->verify_data = verify_data;
    qvm->alloc = allocator ? allocator : &qvm_allocator_default;

    // grab a copy of the header
    memcpy(&qvm->header, filemem, sizeof(qvmheader_t));

    // check header fields for oddities
    if (qvm->header.magic != QVM_MAGIC) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: incorrect magic number\n");
        goto fail;
    }
    if (qvm->filesize != sizeof(qvm->header) + qvm->header.codelen + qvm->header.datalen + qvm->header.litlen) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: filesize doesn't match segment sizes\n");
        goto fail;
    }
    if (qvm->header.codeoffset < sizeof(qvm->header) ||
        qvm->header.codeoffset > qvm->filesize ||
        qvm->header.codeoffset + qvm->header.codelen > qvm->filesize) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: code offset/length has invalid value\n");
        goto fail;
    }
    if (qvm->header.dataoffset < sizeof(qvm->header) ||
        qvm->header.dataoffset > qvm->filesize ||
        qvm->header.dataoffset + qvm->header.datalen + qvm->header.litlen > qvm->filesize) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: data offset/length has invalid value\n");
        goto fail;
    }
    if (qvm->header.numops < qvm->header.codelen / 5 || // assume each op in the code segment is 5 bytes for a minimum
        qvm->header.numops > qvm->header.codelen) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: numops has invalid value\n");
        goto fail;
    }

    // each opcode is 8 bytes long, calculate total size of opcodes
    qvm->codeseglen = qvm->header.numops * sizeof(qvmop_t);
    // just add each data segment up
    qvm->dataseglen = qvm->header.datalen + qvm->header.litlen + qvm->header.bsslen;
    // calculate stack size from config option in MiB. both argstack and stack are half the total size
    if (!stacksize)
        stacksize = 1;
    stacksize *= (1 << 19); // 512KiB 
    qvm->argstackseglen = stacksize;
    qvm->stackseglen = stacksize;

    // allocate vm memory
    qvm->memorysize = qvm->codeseglen + qvm->dataseglen + qvm->argstackseglen + qvm->stackseglen;
    qvm->memory = (uint8_t*)qvm->alloc->alloc(qvm->memorysize, qvm->alloc->ctx);

    // set pointers
    qvm->codesegment = (qvmop_t*)qvm->memory;
    qvm->datasegment = qvm->memory + qvm->codeseglen;
    qvm->argstacksegment = qvm->datasegment + qvm->dataseglen;
    qvm->stacksegment = qvm->argstacksegment + qvm->argstackseglen;

    // setup registers
    // op is the code pointer, simple enough
    qvm->opptr = NULL;
    // argstack is for arguments and local variables. it starts at end of argstack segment and grows down
    qvm->argstackptr = (qvm->argstacksegment + qvm->argstackseglen);
    // stack is for general operations. it starts at end of stack segment and grows down
    qvm->stackptr = (int*)(qvm->stacksegment + qvm->stackseglen);
    // NOTE: memory segments are laid out like this:
    // | CODE | DATA | ARGSTACK | STACK |
    // OP_LOADx loads 1/2/4-bytes from datasegment+stack[0]
    // OP_LOCAL stores (argstackptr - datasegment)+param into *stack

    // start loading ops from the code offset to VM
    codeoffset = filemem + qvm->header.codeoffset;

    // loop through each op
    for (unsigned int i = 0; i < qvm->header.numops; ++i) {
        // get the opcode
        qvmopcode_t opcode = (qvmopcode_t)*codeoffset;
        if (codeoffset > filemem + qvm->header.codeoffset + qvm->header.codelen) {
            log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: numops value too large\n");
            goto fail;
        }

        codeoffset++;

        // write opcode (to qvmop_t)
        qvm->codesegment[i].op = opcode;

        switch (opcode) {
        case OP_EQ:
        case OP_NE:
        case OP_LTI:
        case OP_LEI:
        case OP_GTI:
        case OP_GEI:
        case OP_LTU:
        case OP_LEU:
        case OP_GTU:
        case OP_GEU:
        case OP_EQF:
        case OP_NEF:
        case OP_LTF:
        case OP_LEF:
        case OP_GTF:
        case OP_GEF:
            // this first group of ops all jump to an instruction, so
            // perform a sanity check to make sure the arg is within range
            if (*(unsigned int*)codeoffset > qvm->header.numops) {
                log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid target in jump/branch instruction: %d -> %d\n", *(int*)codeoffset, qvm->header.numops);
                goto fail;
            }
            // explicit fallthrough
        case OP_ENTER:
        case OP_LEAVE:
        case OP_CONST:
        case OP_LOCAL:
        case OP_BLOCK_COPY:
            // all the above ops all have full 4-byte params
            qvm->codesegment[i].param = *(int*)codeoffset;
            codeoffset += 4;
            break;

        case OP_ARG:
            // this op has a 1-byte param
            qvm->codesegment[i].param = (int)*codeoffset;
            codeoffset++;
            break;

        default:
            // remaining ops require no 'param'
            qvm->codesegment[i].param = 0;
            break;
        }
    }

    // copy data segment (including literals) to VM
    memcpy(qvm->datasegment, filemem + qvm->header.dataoffset, qvm->header.datalen + qvm->header.litlen);

    // a winner is us
    return true;

fail:
    qvm_unload(qvm);
    return false;
}


static qvm_t qvm_empty;
void qvm_unload(qvm_t* qvm) {
    if (!qvm)
        return;
    if (qvm->memory)
        qvm->alloc->free(qvm->memory, qvm->memorysize, qvm->alloc->ctx);
    *qvm = qvm_empty;
}


int qvm_exec(qvm_t* qvm, int argc, int* argv) {
    if (!qvm || !qvm->memory)
        return 0;

    // store vmMain cmd for logging
    int vmMain_cmd = argv[0];

    int argsize = (argc + 2) * sizeof(int);

    // grow arg stack for new arguments
    qvm->argstackptr -= argsize;

    // args is an int* view of argstack for setting arguments
    int* args = (int*)qvm->argstackptr;

    // push args into the new argstack space
    // store the current code offset
    args[0] = (int)(qvm->opptr - qvm->codesegment);
    // store the arg size like param in OP_ENTER
    args[1] = argsize;
    // move qvm_exec arguments onto arg stack starting at args[2]
    if (argv && argc > 0)
        memcpy(&args[2], argv, argc * sizeof(int));

    // vmMain's OP_ENTER will grab this return address and store it on the argstack.
    // when it is pulled in OP_LEAVE, it will signal to exit the instruction loop
    --qvm->stackptr;
    qvm->stackptr[0] = -1;

    // start at beginning of code segment
    qvmop_t* opptr = qvm->codesegment;
    // get current argstack pointer
    uint8_t* argstack = qvm->argstackptr;
    // get current stack pointer
    int* stack = qvm->stackptr;

    qvmopcode_t op;
    int param;

    do {
        // verify code pointer is in code segment. this is likely malicious?
        if (!qvm_validate_ptr_code(qvm, opptr)) {
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Execution outside the VM code segment: %p\n", vmMain_cmd, opptr);
            goto fail;
        }
        // verify argstack pointer is in argstack segment. this could be malicious, or an accidental stack overflow
        if (!qvm_validate_ptr_argstack(qvm, argstack)) {
            intptr_t argstacksize = qvm->argstacksegment + qvm->argstackseglen - argstack;
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Arg stack overflow! Arg stack size is currently %d, max is %d. You may need to increase the \"stacksize\" config option.\n", vmMain_cmd, argstacksize, qvm->stackseglen / 2);
            goto fail;
        }
        // verify stack pointer is in stack segment. this could be malicious, or an accidental stack overflow
        if (!qvm_validate_ptr_stack(qvm, stack)) {
            intptr_t stacksize = qvm->stacksegment + qvm->stackseglen - (uint8_t*)stack;
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Stack overflow! Stack size is currently %d, max is %d. You may need to increase the \"stacksize\" config option.\n", vmMain_cmd, stacksize, qvm->stackseglen / 2);
            goto fail;
        }

        op = (qvmopcode_t)opptr->op;
        param = opptr->param;

        ++opptr;

        switch (op) {
        // miscellaneous opcodes

        case OP_NOP:
            // no op - don't raise error
            break;

        case OP_UNDEF:
            // undefined
            // explicit fallthrough

        case OP_BREAK:
            // break to debugger?
            // todo: dump stacks/memory?
            log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Unhandled opcode %s\n", vmMain_cmd, opcodename[op]);
            goto fail;

        default:
            // anything else
            log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Unhandled opcode %d\n", vmMain_cmd, op);
            goto fail;

        // stack opcodes

        case OP_PUSH:
            // pushes a blank value onto the stack
            --stack;
            *stack = 0;
            break;

        case OP_POP:
            // pops the top value off the stack
            ++stack;
            break;

        case OP_CONST:
            // pushes a hardcoded value onto the stack
            --stack;
            *stack = param;
            break;

        case OP_LOCAL:
            // pushes a specified local variable address (relative to start of data segment) onto the stack
            --stack;
            *stack = (int)(&argstack[param] - qvm->datasegment);
            break;

        case OP_ARG:
            // set a function-call arg (offset = param) to the value on top of stack
            *(int*)(&argstack[param]) = *stack;
            ++stack;
            break;

        // functions / code flow

#define JUMP(x) opptr = qvm->codesegment + (x)

        case OP_CALL: {
            // call a function
            // top of the stack is the current instruction index in number of ops from start of code segment
            int jmp_to = *stack;

            // negative means an engine trap
            if (jmp_to < 0) {
                // save local registers for recursive execution
                qvm->argstackptr = argstack;
                qvm->stackptr = stack;
                qvm->opptr = opptr;

                // pass call to game-specific syscall handler which will adjust pointer arguments
                // and then call the normal QMM syscall entry point so it can be routed to plugins
                int ret = qvm->vmsyscall(qvm->datasegment, -jmp_to - 1, (int*)argstack + 2);

                // restore local registers
                argstack = qvm->argstackptr;
                stack = qvm->stackptr;
                opptr = qvm->opptr;

                // return value on top of stack
                *stack = ret;
                break;
            }

            // replace top of stack with the current instruction index (number of ops from start of code segment)
            *stack = (int)(opptr - qvm->codesegment);
            // jump to VM function at address
            JUMP(jmp_to);
            break;
        }

        case OP_ENTER: {
            // enter a function, prepare local variable argstack space (length=param).
            // store the instruction return index (at top of stack from OP_CALL) in argstack and
            // store the param in the next argstack slot. this gets verified to match in OP_LEAVE
            argstack -= param;
            int* arg = (int*)argstack;
            arg[0] = *stack;
            arg[1] = param;
            stack++;
            break;
        }

        case OP_LEAVE: {
            // leave a function
            // get previous instruction index from argstack[0] and OP_ENTER param from
            // argstack[1]. verify argstack[1] matches param, and then clean up argstack frame
            // retrieve the code return address from the argstack
            int* arg = (int*)argstack;
            // compare param with the OP_ENTER param stored on the argstack
            if (arg[1] != param) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): OP_LEAVE param (%d) does not match OP_ENTER param (%d)\n", vmMain_cmd, param, arg[1]);
                goto fail;
            }
            // if return instruction pointer is our negative sentinel, signal end of instruction loop
            if (arg[0] < 0)
                opptr = NULL;
            else
                opptr = qvm->codesegment + arg[0];
            // offset argstack to cleanup frame
            argstack += param;
            break;
        }

        // branching

// signed integer comparison
#define SIF(o) if (stack[1] o *stack) JUMP(param); stack += 2
// unsigned integer comparison
#define UIF(o) if (*(unsigned int*)&stack[1] o *(unsigned int*)stack) JUMP(param); stack += 2
// floating point comparison
#define FIF(o) if (*(float*)&stack[1] o *(float*)stack) JUMP(param); stack += 2

        case OP_JUMP:
            // jump to address in stack[0]
            JUMP(*stack++);
            break;

        case OP_EQ:
            // if stack[1] == stack[0], goto address in param
            SIF( == );
            break;

        case OP_NE:
            // if stack[1] != stack[0], goto address in param
            SIF( != );
            break;

        case OP_LTI:
            // if stack[1] < stack[0], goto address in param
            SIF( < );
            break;

        case OP_LEI:
            // if stack[1] <= stack[0], goto address in param
            SIF( <= );
            break;

        case OP_GTI:
            // if stack[1] > stack[0], goto address in param
            SIF( > );
            break;

        case OP_GEI:
            // if stack[1] >= stack[0], goto address in param
            SIF( >= );
            break;

        case OP_LTU:
            // if stack[1] < stack[0], goto address in param (unsigned)
            UIF( < );
            break;

        case OP_LEU:
            // if stack[1] <= stack[0], goto address in param (unsigned)
            UIF( <= );
            break;

        case OP_GTU:
            // if stack[1] > stack[0], goto address in param (unsigned)
            UIF( > );
            break;

        case OP_GEU:
            // if stack[1] >= stack[0], goto address in param (unsigned)
            UIF( >= );
            break;

        case OP_EQF:
            // if stack[1] == stack[0], goto address in param (float)
            FIF( == );
            break;

        case OP_NEF:
            // if stack[1] != stack[0], goto address in param (float)
            FIF( != );
            break;

        case OP_LTF:
            // if stack[1] < stack[0], goto address in param (float)
            FIF( < );
            break;

        case OP_LEF:
            // if stack[1] <= stack[0], goto address in param (float)
            FIF( <= );
            break;

        case OP_GTF:
            // if stack[1] > stack[0], goto address in param (float)
            FIF( > );
            break;

        case OP_GEF:
            // if stack[1] >= stack[0], goto address in param (float)
            FIF( >= );
            break;

        // memory/pointer management

        case OP_STORE1: {
            // store 1-byte value from stack[0] into address stored in stack[1]
            uint8_t* dst = qvm->datasegment + stack[1];
            if (!qvm_validate_ptr_data(qvm, dst)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], dst);
                goto fail;
            }
            *dst = (uint8_t)(*stack & 0xFF);
            stack += 2;
            break;
        }

        case OP_STORE2: {
            // 2-byte
            unsigned short* dst = (unsigned short*)(qvm->datasegment + stack[1]);
            if (!qvm_validate_ptr_data(qvm, dst)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], dst);
                goto fail;
            }
            *dst = (unsigned short)(*stack & 0xFFFF);
            stack += 2;
            break;
        }

        case OP_STORE4: {
            // 4-byte
            int* dst = (int*)(qvm->datasegment + stack[1]);
            if (!qvm_validate_ptr_data(qvm, dst)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], dst);
                goto fail;
            }
            *dst = *stack;
            stack += 2;
            break;
        }

        case OP_LOAD1: {
            // get 1-byte value at address stored in stack[0],
            // and store back in stack[0]
            // 1-byte
            uint8_t* src = qvm->datasegment + *stack;
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], src);
                goto fail;
            }
            *stack = (int)*src;
            break;
        }

        case OP_LOAD2: {
            // 2-byte
            uint16_t* src = (uint16_t*)(qvm->datasegment + *stack);
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], src);
                goto fail;
            }
            *stack = (int)*src;
            break;
        }

        case OP_LOAD4: {
            // 4-byte
            int* src = (int*)(qvm->datasegment + *stack);
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], src);
                goto fail;
            }
            *stack = *src;
            break;
        }

        case OP_BLOCK_COPY: {
            // copy mem at address pointed to by stack[0] to address pointed to by stack[1]
            // for 'param' number of bytes
            uint8_t* src = qvm->datasegment + *stack++;
            uint8_t* dst = qvm->datasegment + *stack++;

            // skip if src/dst are the same
            if (src == dst)
                break;

            // check if src block goes out of VM range
            if (!qvm_validate_ptr_data(qvm, src) || !qvm_validate_ptr_data(qvm, src + param - 1)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s source pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], src);
                goto fail;
            }
            // check if dst block goes out of VM range
            if (!qvm_validate_ptr_data(qvm, dst) || !qvm_validate_ptr_data(qvm, dst + param - 1)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s destination pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], dst);
                goto fail;
            }

            memcpy(dst, src, param);

            break;
        }

        // arithmetic/operators

// signed integer (stack[0] done to stack[1], stored in stack[1])
#define SOP(o) stack[1] o *stack; stack++
// unsigned integer (stack[0] done to stack[1], stored in stack[1])
#define UOP(o) *(unsigned int*)&stack[1] o *(unsigned int*)stack; stack++
// floating point (stack[0] done to stack[1], stored in stack[1])
#define FOP(o) *(float*)&stack[1] o *(float*)stack; stack++
// signed integer (done to self)
#define SSOP(o) *stack =o *stack
// floating point (done to self)
#define SFOP(o) *(float*)stack =o *(float*)stack

        case OP_NEGI:
            // negation
            SSOP( - );
            break;

        case OP_ADD:
            // addition
            SOP( += );
            break;

        case OP_SUB:
            // subtraction
            SOP( -= );
            break;

        case OP_MULI:
            // multiplication
            SOP( *= );
            break;

        case OP_MULU:
            // unsigned multiplication
            UOP( *= );
            break;

        case OP_DIVI:
            // division
            SOP( /= );
            break;

        case OP_DIVU:
            // unsigned division
            UOP( /= );
            break;

        case OP_MODI:
            // modulus
            SOP( %= );
            break;

        case OP_MODU:
            // unsigned modulus
            UOP( %= );
            break;

        case OP_BAND:
            // bitwise AND
            SOP( &= );
            break;

        case OP_BOR:
            // bitwise OR
            SOP( |= );
            break;

        case OP_BXOR:
            // bitwise XOR
            SOP( ^= );
            break;

        case OP_BCOM:
            // bitwise one's compliment
            SSOP( ~ );
            break;

        case OP_LSH:
            // unsigned bitwise LEFTSHIFT
            UOP( <<= );
            break;

        case OP_RSHI:
            // bitwise RIGHTSHIFT
            SOP( >>= );
            break;

        case OP_RSHU:
            // unsigned bitwise RIGHTSHIFT
            UOP( >>= );
            break;

        case OP_NEGF:
            // float negation
            SFOP( - );
            break;

        case OP_ADDF:
            // float addition
            FOP( += );
            break;

        case OP_SUBF:
            // float subtraction
            FOP( -= );
            break;

        case OP_MULF:
            // float multiplication
            FOP( *= );
            break;

        case OP_DIVF:
            // float division
            FOP( /= );
            break;

        // sign extensions

        case OP_SEX8:
            // 8-bit
            if (*stack & 0x80)
                *stack |= 0xFFFFFF00;
            break;

            // 16-bit
        case OP_SEX16:
            if (*stack & 0x8000)
                *stack |= 0xFFFF0000;
            break;

        // format conversion

        case OP_CVIF:
            // convert stack[0] int->float
            *(float*)stack = (float)*stack;
            break;

        case OP_CVFI:
            // convert stack[0] float->int
            *stack = (int)*(float*)stack;
            break;
        } // switch (op)
    } while (opptr);

    // int view into argstack to check args
    args = (int*)argstack;

    // restore previous code pointer
    qvm->opptr = qvm->codesegment + args[0];

    // compare stored argsize like in OP_LEAVE
    if (args[1] != argsize) {
        log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): exit argsize (%d) does not match enter argsize (%d)\n", vmMain_cmd, args[1], argsize);
        goto fail;
    }

    // remove arguments from argstack like in OP_LEAVE
    argstack += argsize;

    // update persistent argstack pointer with the current one
    qvm->argstackptr = argstack;

    // update persistent stack pointer with the current one
    qvm->stackptr = stack;

    // return value is stored on the top of the stack (pushed just before OP_LEAVE)
    return *qvm->stackptr++;

fail:
    qvm_unload(qvm);
    return 0;
}


// return a string name for the VM opcode
const char* opcodename[] = {
    "OP_UNDEF",
    "OP_NOP",
    "OP_BREAK",
    "OP_ENTER",
    "OP_LEAVE",
    "OP_CALL",
    "OP_PUSH",
    "OP_POP",
    "OP_CONST",
    "OP_LOCAL",
    "OP_JUMP",
    "OP_EQ",
    "OP_NE",
    "OP_LTI",
    "OP_LEI",
    "OP_GTI",
    "OP_GEI",
    "OP_LTU",
    "OP_LEU",
    "OP_GTU",
    "OP_GEU",
    "OP_EQF",
    "OP_NEF",
    "OP_LTF",
    "OP_LEF",
    "OP_GTF",
    "OP_GEF",
    "OP_LOAD1",
    "OP_LOAD2",
    "OP_LOAD4",
    "OP_STORE1",
    "OP_STORE2",
    "OP_STORE4",
    "OP_ARG",
    "OP_BLOCK_COPY",
    "OP_SEX8",
    "OP_SEX16",
    "OP_NEGI",
    "OP_ADD",
    "OP_SUB",
    "OP_DIVI",
    "OP_DIVU",
    "OP_MODI",
    "OP_MODU",
    "OP_MULI",
    "OP_MULU",
    "OP_BAND",
    "OP_BOR",
    "OP_BXOR",
    "OP_BCOM",
    "OP_LSH",
    "OP_RSHI",
    "OP_RSHU",
    "OP_NEGF",
    "OP_ADDF",
    "OP_SUBF",
    "OP_DIVF",
    "OP_MULF",
    "OP_CVIF",
    "OP_CVFI"
};


static bool qvm_validate_ptr(qvm_t* qvm, void* ptr, void* start, void* end) {
    if (!qvm || !qvm->memory)
        return false;

    return (ptr >= start && ptr < end);
}


static bool qvm_validate_ptr_data(qvm_t* qvm, void* ptr) {
    if (!qvm || !qvm->memory)
        return false;
    if (!qvm->verify_data)
        return true;
    return qvm_validate_ptr(qvm, ptr, qvm->datasegment, qvm->memory + qvm->memorysize);
}


static bool qvm_validate_ptr_code(qvm_t* qvm, void* ptr) {
    return qvm_validate_ptr(qvm, ptr, qvm->codesegment, qvm->memory + qvm->codeseglen);
}


static bool qvm_validate_ptr_argstack(qvm_t* qvm, void* ptr) {
    // stacks start off pointing just above the segment and are immediately subtracted on first use. +1 to allow exec to start
    return qvm_validate_ptr(qvm, ptr, qvm->argstacksegment, qvm->argstacksegment + qvm->argstackseglen + 1);
}


static bool qvm_validate_ptr_stack(qvm_t* qvm, void* ptr) {
    // stacks start off pointing just above the segment and are immediately subtracted on first use. +1 to allow exec to start
    return qvm_validate_ptr(qvm, ptr, qvm->stacksegment, qvm->stacksegment + qvm->stackseglen + 1);
}


static void* qvm_alloc_default(ptrdiff_t size, void* ctx) {
    (void)ctx;
    return (void*)malloc(size);
}


static void qvm_free_default(void* ptr, ptrdiff_t size, void* ctx) {
    (void)ctx; (void)size;
    free(ptr);
}


qvm_alloc_t qvm_allocator_default = { qvm_alloc_default, qvm_free_default, NULL };
