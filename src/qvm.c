/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "qvm.h"
#include "log.h"


// check to make sure ptr is within the range [start, end)
static int qvm_validate_ptr(qvm_t* qvm, void* ptr, void* start, void* end);
// check to make sure ptr is within data or stack segments [datasegment, stacksegment+stackseglen+1)
static int qvm_validate_ptr_data(qvm_t* qvm, void* ptr);
// check to make sure ptr is within code segment [codesegment, codesegment+codeseglen)
static int qvm_validate_ptr_code(qvm_t* qvm, void* ptr);
// check to make sure ptr is within stack segment [stacksegment, stacksegment+stackseglen+1)
static int qvm_validate_ptr_stack(qvm_t* qvm, void* ptr);


int qvm_load(qvm_t* qvm, const uint8_t* filemem, size_t filesize, vmsyscall_t vmsyscall, size_t stacksize, int verify_data, qvm_alloc_t* allocator) {
    if (!qvm || qvm->memory || !filemem || !filesize || !vmsyscall)
        return 0;

    if (filesize < sizeof(qvmheader_t)) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: too small for header\n");
        goto fail;
    }
    
    qvm->filesize = filesize;
    qvm->vmsyscall = vmsyscall;
    qvm->verify_data = verify_data;
    qvm->exec_layer = 0;
    // if null, use default allocator (malloc/free)
    qvm->allocator = allocator ? allocator : &qvm_allocator_default;

    qvmheader_t header;

    // grab a copy of the header
    memcpy(&header, filemem, sizeof(qvmheader_t));

    // save header in qvm_t
    qvm->header = header;

    // check header fields for oddities
    if (header.magic != QVM_MAGIC) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: incorrect magic number\n");
        goto fail;
    }
    if (filesize != sizeof(header) + header.codelen + header.datalen + header.litlen) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: filesize doesn't match segment sizes\n");
        goto fail;
    }
    if (header.codeoffset < sizeof(header) ||
        header.codeoffset > filesize ||
        header.codeoffset + header.codelen > filesize) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: code offset/length has invalid value\n");
        goto fail;
    }
    if (header.dataoffset < sizeof(header) ||
        header.dataoffset > filesize ||
        header.dataoffset + header.datalen + header.litlen > filesize) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: data offset/length has invalid value\n");
        goto fail;
    }
    if (header.numops < header.codelen / 5 || // assume each op in the code segment is 5 bytes for a minimum
        header.numops > header.codelen) {
        log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: numops has invalid value\n");
        goto fail;
    }

    // each opcode is 8 bytes long, calculate total size of opcodes
    qvm->codeseglen = header.numops * sizeof(qvmop_t);
    // just add each data segment up
    qvm->dataseglen = header.datalen + header.litlen + header.bsslen;
    // calculate stack size from config option in MiB
    if (!stacksize)
        stacksize = 1;
    // cap stack at an arbitrary 16MiB
    if (stacksize > 16)
        stacksize = 16;
    stacksize *= (1 << 20); // 1MiB
    qvm->stackseglen = stacksize;

    // allocate vm memory
    qvm->memorysize = qvm->codeseglen + qvm->dataseglen + qvm->stackseglen + qvm->stackseglen;
    qvm->memory = (uint8_t*)qvm->allocator->alloc(qvm->memorysize, qvm->allocator->ctx);

    // set segment pointers
    // NOTE: memory segments are laid out like this:
    // | CODE | DATA | <STACK< |
    // program stack grows downward from highest address
    qvm->codesegment = (qvmop_t*)qvm->memory;
    qvm->datasegment = qvm->memory + qvm->codeseglen;
    qvm->stacksegment = qvm->datasegment + qvm->dataseglen;

    // setup registers
    // program stack is for arguments and local variables. it starts at end of stack segment and grows down
    qvm->stackptr = (qvm->stacksegment + qvm->stackseglen);

    // start loading ops from the file's code offset into VM memory block
    const uint8_t* codeoffset = filemem + header.codeoffset;

    // loop through each op
    for (unsigned int i = 0; i < header.numops; ++i) {
        // get the opcode
        qvmopcode_t opcode = (qvmopcode_t)*codeoffset;

        // make sure opcode is valid
        if (opcode < 0 || opcode >= OP_NUM_OPS) {
            log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: invalid opcode value: %d\n", opcode);
            goto fail;
        }
        
        // make sure we're not reading past the end of the codesegment in the file
        if (codeoffset >= filemem + header.codeoffset + header.codelen) {
            log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: numops value too large: %d\n", header.numops);
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
            if (*(unsigned int*)codeoffset > header.numops) {
                log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid target in jump/branch instruction %s: %d -> %d\n", opcodename[opcode], *(int*)codeoffset, header.numops);
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
    memcpy(qvm->datasegment, filemem + header.dataoffset, header.datalen + header.litlen);

    // a winner is us
    return 1;

fail:
    qvm_unload(qvm);
    return 0;
}


static qvm_t qvm_empty;
void qvm_unload(qvm_t* qvm) {
    if (!qvm)
        return;
    if (qvm->memory)
        qvm->allocator->free(qvm->memory, qvm->memorysize, qvm->allocator->ctx);
    *qvm = qvm_empty;
}


int qvm_exec(qvm_t* qvm, int argc, int* argv) {
    if (!qvm || !qvm->memory)
        return 0;

    // store vmMain cmd for logging
    int vmMain_cmd = argv[0];

    // increment exec_layer
    qvm->exec_layer++;

    // start instruction pointer at beginning of code segment
    qvmop_t* opptr = qvm->codesegment;

    // get current stack pointer into a local "register"
    uint8_t* programstack = qvm->stackptr;
    // iprogramstack is an int view of programstack to set/check args
    int* iprogramstack = (int*)programstack;

// macro to manage stack frame and keep iprogramstack locked to programstack
#define PROGRAMSTACK_FRAME(size) (programstack -= (size), iprogramstack = (int*)programstack)

    // amount to grow stack
    int argsize = (argc + 2) * sizeof(argv[0]);

    // grow program stack for new arguments
    PROGRAMSTACK_FRAME(argsize);

    // push args into the new program stack space
    // sentinel return pointer to signal end of execution. this will be overwritten anyway by the first OP_ENTER
    iprogramstack[0] = -1;
    // store the arg size like param in OP_ENTER
    iprogramstack[1] = argsize;
    // move qvm_exec arguments onto stack starting at iprogramstack[2]
    if (argv && argc > 0)
        memcpy(&iprogramstack[2], argv, argc * sizeof(argv[0]));

    // stack for math/comparison/temp/etc operations (instead of using registers)
    int opstack[OPSTACK_SIZE];
    memset(opstack, 0, sizeof(opstack));

    // start stack just past the end of the block
    int* stack = opstack + OPSTACK_SIZE;

    // vmMain's OP_ENTER will grab this return address and store it in programstack[0].
    // when it is pulled in OP_LEAVE, it will signal to exit the instruction loop
    --stack;
    stack[0] = -1;

    qvmopcode_t op;
    int param;

    do {
        // verify code pointer is in code segment. this is likely malicious?
        if (!qvm_validate_ptr_code(qvm, opptr)) {
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Execution outside the VM code segment: %p\n", vmMain_cmd, opptr);
            goto fail;
        }
        // verify program stack pointer is in stack segment. this could be malicious, or an accidental stack overflow
        if (!qvm_validate_ptr_stack(qvm, programstack)) {
            intptr_t stacksize = qvm->stacksegment + qvm->stackseglen - programstack;
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Program stack overflow! Program stack size is currently %d, max is %d. You may need to increase the \"stacksize\" config option.\n", vmMain_cmd, stacksize, qvm->stackseglen);
            goto fail;
        }
        // verify op stack pointer is in op stack. this could be malicious, or an accidental stack overflow
        // using > to allow starting at 1 past the end of block
        if (stack <= opstack || stack > opstack + OPSTACK_SIZE ) {
            intptr_t stacksize = opstack + OPSTACK_SIZE - stack;
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Opstack overflow! Opstack size is currently %d, max is %d.\n", vmMain_cmd, stacksize, OPSTACK_SIZE);
            goto fail;
        }

        op = (qvmopcode_t)opptr->op;
        param = opptr->param;

        ++opptr;

        switch (op) {
        // miscellaneous opcodes

        case OP_UNDEF:
            // undefined - used as alignment padding at end of codesegment in file, treat as NOP
            // explicit fallthrough
        case OP_NOP:
            // no op - don't raise error
            break;

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
            // pushes a 0 value onto the stack
            --stack;
            stack[0] = 0;
            break;

        case OP_POP:
            // pops the top value off the stack
            ++stack;
            break;

        case OP_CONST:
            // pushes a hardcoded value onto the stack
            --stack;
            stack[0] = param;
            break;

        case OP_LOCAL:
            // pushes a specified local variable address (relative to start of data segment) onto the stack
            --stack;
            stack[0] = (int)(&programstack[param] - qvm->datasegment);
            break;

        case OP_ARG:
            // set a function-call arg (offset = param) to the value on top of stack
            *(int*)(&programstack[param]) = stack[0];
            ++stack;
            break;

        // functions / code flow

#define JUMP(x) opptr = qvm->codesegment + (x)

        case OP_CALL: {
            // call a function, instruction index in stack[0]. store return value in stack[0]
            int jmp_to = stack[0];

            // negative means an engine trap
            if (jmp_to < 0) {
                // save local registers for possible recursive execution
                qvm->stackptr = programstack;

                // pass call to game-specific syscall handler which will adjust pointer arguments
                // and then call the normal QMM syscall entry point so it can be routed to plugins
                int ret = qvm->vmsyscall(qvm->datasegment, -jmp_to - 1, &iprogramstack[2]);

                // restore local registers
                programstack = qvm->stackptr;
                // keep iprogramstack locked to programstack
                iprogramstack = (int*)programstack;

                // place return value on top of stack
                stack[0] = ret;
                break;
            }

            // replace top of stack with the current instruction index (number of ops from start of code segment)
            stack[0] = (int)(opptr - qvm->codesegment);
            // jump to VM function at address
            JUMP(jmp_to);
            break;
        }

        case OP_ENTER: {
            // enter a function, prepare local variable program stack space (length=param).
            // store the instruction return index (at top of stack from OP_CALL) in program stack and
            // store the param in the next program stack slot. this gets verified to match in OP_LEAVE
            PROGRAMSTACK_FRAME(param);
            iprogramstack[0] = stack[0];
            iprogramstack[1] = param;
            ++stack;
            break;
        }

        case OP_LEAVE: {
            // leave a function, get previous instruction index from top of program stack and OP_ENTER
            // param from programstack[1]. verify programstack[1] matches param, then retrieve the code
            // return address from programstack[0]. remove stack frame by adding param.
            if (iprogramstack[1] != param) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): OP_LEAVE param (%d) does not match OP_ENTER param (%d)\n", vmMain_cmd, param, iprogramstack[1]);
                goto fail;
            }
            // if return instruction pointer is our negative sentinel, signal end of instruction loop
            if (iprogramstack[0] < 0)
                opptr = NULL;
            else
                opptr = qvm->codesegment + iprogramstack[0];
            // offset stack to cleanup frame
            PROGRAMSTACK_FRAME(-param);
            break;
        }

        // branching

// signed integer comparison
#define SIF(o) if (stack[1] o stack[0]) JUMP(param); stack += 2
// unsigned integer comparison
#define UIF(o) if (*(unsigned int*)&stack[1] o *(unsigned int*)&stack[0]) JUMP(param); stack += 2
// floating point comparison
#define FIF(o) if (*(float*)&stack[1] o *(float*)&stack[0]) JUMP(param); stack += 2

        case OP_JUMP:
            // jump to address in stack[0]
            JUMP(stack[0]);
            ++stack;
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
            *dst = (uint8_t)(stack[0] & 0xFF);
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
            *dst = (unsigned short)(stack[0] & 0xFFFF);
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
            *dst = stack[0];
            stack += 2;
            break;
        }

        case OP_LOAD1: {
            // get 1-byte value at address stored in stack[0],
            // and store back in stack[0]
            // 1-byte
            uint8_t* src = qvm->datasegment + stack[0];
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], src);
                goto fail;
            }
            stack[0] = (int)*src;
            break;
        }

        case OP_LOAD2: {
            // 2-byte
            uint16_t* src = (uint16_t*)(qvm->datasegment + stack[0]);
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], src);
                goto fail;
            }
            stack[0] = (int)*src;
            break;
        }

        case OP_LOAD4: {
            // 4-byte
            int* src = (int*)(qvm->datasegment + stack[0]);
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): %s pointer validation failed! ptr = %p\n", vmMain_cmd, opcodename[op], src);
                goto fail;
            }
            stack[0] = *src;
            break;
        }

        case OP_BLOCK_COPY: {
            // copy mem at address pointed to by stack[0] to address pointed to by stack[1]
            // for 'param' number of bytes
            uint8_t* src = qvm->datasegment + stack[0];
            uint8_t* dst = qvm->datasegment + stack[1];

            stack += 2;

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
#define SOP(o) stack[1] o stack[0]; ++stack
// unsigned integer (stack[0] done to stack[1], stored in stack[1])
#define UOP(o) *(unsigned int*)&stack[1] o *(unsigned int*)&stack[0]; ++stack
// floating point (stack[0] done to stack[1], stored in stack[1])
#define FOP(o) *(float*)&stack[1] o *(float*)&stack[0]; ++stack
// signed integer (done to self)
#define SSOP(o) stack[0] = o stack[0]
// floating point (done to self)
#define SFOP(o) *(float*)&stack[0] = o *(float*)&stack[0]

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
            if (stack[0] & 0x80)
                stack[0] |= 0xFFFFFF00;
            break;

        case OP_SEX16:
            // 16-bit
            if (stack[0] & 0x8000)
                stack[0] |= 0xFFFF0000;
            break;

        // format conversion

        case OP_CVIF:
            // convert stack[0] int->float
            *(float*)&stack[0] = (float)stack[0];
            break;

        case OP_CVFI:
            // convert stack[0] float->int
            stack[0] = (int)*(float*)&stack[0];
            break;
        } // switch (op)
    } while (opptr);

    // compare stored argsize like in OP_LEAVE
    if (iprogramstack[1] != argsize) {
        log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): exit argsize (%d) does not match enter argsize (%d)\n", vmMain_cmd, iprogramstack[1], argsize);
        goto fail;
    }

    // remove arguments from program stack like in OP_LEAVE
    PROGRAMSTACK_FRAME(-argsize);

    // update persistent stack pointer with the temp register one
    qvm->stackptr = programstack;

    // decrement exec_layer
    qvm->exec_layer--;

    // return value is stored on the top of the stack (pushed just before OP_LEAVE)
    return stack[0];

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


static int qvm_validate_ptr(qvm_t* qvm, void* ptr, void* start, void* end) {
    if (!qvm || !qvm->memory)
        return 0;
    if (!start)
        start = qvm->memory;
    if (!end)
        end = qvm->memory + qvm->memorysize + 1;
    return ((intptr_t)ptr >= (intptr_t)start && (intptr_t)ptr < (intptr_t)end);
}


static int qvm_validate_ptr_data(qvm_t* qvm, void* ptr) {
    if (!qvm || !qvm->memory)
        return 0;
    if (!qvm->verify_data)
        return 1;
    // data access can include stack segment. don't need +1 since that is an invalid address for data
    return qvm_validate_ptr(qvm, ptr, qvm->datasegment, qvm->stacksegment + qvm->stackseglen);
}


static int qvm_validate_ptr_code(qvm_t* qvm, void* ptr) {
    return qvm_validate_ptr(qvm, ptr, qvm->codesegment, qvm->memory + qvm->codeseglen);
}


static int qvm_validate_ptr_stack(qvm_t* qvm, void* ptr) {
    return qvm_validate_ptr(qvm, ptr, qvm->stacksegment, qvm->stacksegment + qvm->stackseglen);
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
