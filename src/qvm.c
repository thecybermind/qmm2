/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define QMM_LOGGING
#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "qvm.h"
#ifdef QMM_LOGGING
#include "log.h"
#else
#define log_c(...) /* */
#endif


// check to make sure ptr is within the range [start, end)
static int qvm_validate_ptr(qvm_t* qvm, void* ptr, void* start, void* end);
// check to make sure ptr is within data or stack segments [datasegment, stacksegment+stackseglen)
static int qvm_validate_ptr_data(qvm_t* qvm, void* ptr);
// check to make sure ptr is within code segment [codesegment, codesegment+codeseglen)
static int qvm_validate_ptr_code(qvm_t* qvm, void* ptr);
// check to make sure ptr is within stack segment [stacksegment, stacksegment+stackseglen)
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
    qvm->exec_depth = 0;
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
    // | CODE | DATA | STACK |
    // stack is accessed using OP_LOADx instructions, so best to be right after data segment
    qvm->codesegment = (qvmop_t*)qvm->memory;
    qvm->datasegment = qvm->memory + qvm->codeseglen;
    qvm->stacksegment = qvm->datasegment + qvm->dataseglen;

    // setup registers
    // program stack is for arguments and local variables. it starts at end of stack segment and grows down
    qvm->stackptr = (int*)(qvm->stacksegment + qvm->stackseglen);

    // start loading ops from the file's code offset into VM memory block
    const uint8_t* codeoffset = filemem + header.codeoffset;

    // loop through each op
    for (unsigned int i = 0; i < header.numops; ++i) {
        // get the opcode
        qvmopcode_t opcode = (qvmopcode_t)*codeoffset;

        // make sure opcode is valid
        if (opcode < 0 || opcode >= OP_NUM_OPS) {
            log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: invalid opcode value at %d: %d\n", i, opcode);
            goto fail;
        }
        
        // make sure we're not reading past the end of the codesegment in the file
        if (codeoffset >= filemem + header.codeoffset + header.codelen) {
            log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid QVM file: can't write instruction at %d, VM code segment is full\n", i);
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
            // this first group of ops all have an instruction index param,
            // so perform a sanity check to make sure the param is within range
            if (*(unsigned int*)codeoffset > header.numops) {
                log_c(QMM_LOG_ERROR, "QMM", "qvm_load(): Invalid target in jump/branch instruction %s at %d: %d > %d\n", opcodename[opcode], i, *(unsigned int*)codeoffset, header.numops);
                goto fail;
            }
            // explicit fallthrough
        case OP_ENTER:
        case OP_LEAVE:
        case OP_CONST:
        case OP_LOCAL:
        case OP_BLOCK_COPY:
            // all the above instructions have 4-byte params
            qvm->codesegment[i].param = *(int*)codeoffset;
            codeoffset += 4;
            break;

        case OP_ARG:
            // this instruction has a 1-byte param
            qvm->codesegment[i].param = (int)*codeoffset;
            codeoffset++;
            break;

        default:
            // remaining instructions have no param
            qvm->codesegment[i].param = 0;
            break;
        }
    }

    // copy data segment (including literals) to VM
    memcpy(qvm->datasegment, filemem + header.dataoffset, header.datalen + header.litlen);

    // a winner is us
    return 1;

fail:
    // :(
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

    // cmd that vmMain was called with
    int vmMain_cmd = argv[0];

    // increment exec_depth
    qvm->exec_depth++;

    // instruction pointer
    qvmop_t* opptr = qvm->codesegment;

    // local "register" copy of stack pointer. this is purely for locality/speed.
    // it gets synced to qvm object before syscalls and restored after syscalls
    // it also gets synced to qvm object after execution completes
    int* programstack = qvm->stackptr;

// macro to manage stack frame in bytes
#define QVM_PROGRAMSTACK_FRAME(size) programstack = (int*)((uint8_t*)programstack - (size))

    // amount to grow stack for storing RII, framesize, and vmMain args
    int framesize = (argc + 2) * sizeof(argv[0]);

    // grow program stack for new arguments
    QVM_PROGRAMSTACK_FRAME(framesize);

    // push args into the new programstack frame
    programstack[0] = -1;           // sentinel return instruction index (RII)
    programstack[1] = framesize;    // store the frame size like we store param in OP_ENTER
    // copy qvm_exec arguments onto stack starting at programstack[2]
    if (argv && argc > 0)
        memcpy(&programstack[2], argv, argc * sizeof(argv[0]));

    // program stack frame example:
    // a "|" separates stack items
    // a "||" separates stack frames
    // || RII | size | arg0 | arg1 | local0 | local1 || RII | size | arg0 | local0 || -1 | size | cmd | arg0 | arg1 | ...
    // when a function is to be called, the arguments are set in the local stack frame in the slots marked "arg#".
    // then the address of the function to be called is placed onto the top of the opstack. then, the OP_CALL
    // instruction is used. OP_CALL will place the current instruction index into the RII slot (programstack[0]),
    // then it will pop the function instruction index from the opstack and jump to it.
    // 
    // the next instruction should be the callee's OP_ENTER instruction. OP_ENTER will add a new stack frame with
    // a hardcoded size param, then stores that size in programstack[1] and leaves programstack[0] empty.
    // 
    // just before a function exits, a return value is pushed onto the top of the opstack. then, the final
    // instruction in a function is OP_LEAVE. OP_LEAVE will check programstack[1] to see if it matches its own
    // hardcoded size param, and then remove the stack frame of the given size. it then looks in programstack[0] of
    // the previous frame (now topmost) for the RII and jumps to it. if the RII is <0 (-1 is set in the first stack
    // frame created before execution), it signals to end VM execution.
    //
    // within a function, arguments are loaded by just accessing the "arg#" values from the previous stack frame.

    // stack for math/comparison/temp/etc operations (instead of using registers)
    // +2 for some extra space to "harmlessly" read 2 values (like OP_BLOCK_COPY) if stack is empty (like at start)
    int opstack[OPSTACK_SIZE + 2];
    memset(opstack, 0, sizeof(opstack));

    // opstack pointer (starts at end of block, grows down)
    int* stack = opstack + OPSTACK_SIZE;

// macros to manage opstack
#define QVM_POPN(n)    stack += (n)
#define QVM_POP()      QVM_POPN(1)
#define QVM_PUSH()     --stack

    // current op
    qvmopcode_t op;
    // hardcoded param for op
    int param;
    // current instruction index (for logging)
    int instr_index;

    // main instruction loop
    do {
        // verify code pointer is in code segment
        if (!qvm_validate_ptr_code(qvm, opptr)) {
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Runtime error: execution outside the VM code segment (%p-%p): %p\n", vmMain_cmd, qvm->codesegment, qvm->codesegment + qvm->codeseglen - 1, opptr);
            goto fail;
        }
        // store instruction index (for logging)
        instr_index = (int)(opptr - qvm->codesegment);
        // verify program stack pointer is in stack segment
        if (!qvm_validate_ptr_stack(qvm, programstack)) {
            intptr_t stacksize = qvm->stacksegment + qvm->stackseglen - (uint8_t*)programstack;
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Runtime error at %d: program stack overflow! Program stack size is currently %d, max is %d. You may need to increase the \"stacksize\" config option.\n", vmMain_cmd, instr_index, stacksize, qvm->stackseglen);
            goto fail;
        }
        // verify op stack pointer is in op stack
        // using > to allow starting at 1 past the end of block
        if (stack <= opstack || stack > opstack + OPSTACK_SIZE) {
            intptr_t stacksize = opstack + OPSTACK_SIZE - stack;
            log_c(QMM_LOG_ERROR, "QMM", "qvm_exec(%d): Runtime error at %d: opstack overflow! Opstack size is currently %d, max is %d.\n", vmMain_cmd, instr_index, stacksize, OPSTACK_SIZE);
            goto fail;
        }

        // get the instruction's opcode and param
        op = (qvmopcode_t)opptr->op;
        param = opptr->param;
        
        // throughout opcode handling, opptr points to the NEXT instruction to execute
        ++opptr;

        switch (op) {
        // miscellaneous opcodes

        case OP_UNDEF:
            // undefined - used as alignment padding at end of codesegment in file, treat as no op
            // explicit fallthrough
        case OP_NOP:
            // no op
            // explicit fallthrough
        case OP_BREAK:
            // break to debugger, treat as no op for now
            // todo: dump stacks/memory?
            break;

        default:
            // anything else
            log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: unhandled opcode %d\n", vmMain_cmd, instr_index, op);
            goto fail;

        // functions

#define QVM_JUMP(x) opptr = qvm->codesegment + (x)

        case OP_ENTER:
            // enter a function:
            // prepare new stack frame on program stack (size=param).
            // store param in programstack[1]. this gets verified to match in OP_LEAVE.
            QVM_PROGRAMSTACK_FRAME(param);
            programstack[0] = 0; // leave blank. an OP_CALL within this function will place RII here
            programstack[1] = param;
            break;

        case OP_LEAVE:
            // leave a function:
            // verify the value saved in programstack[1] matches param, then remove stack frame (size=param).
            // then, grab RII from top of previous stack frame and then jump to it
            if (programstack[1] != param) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: OP_LEAVE param (%d) does not match OP_ENTER param (%d)\n", vmMain_cmd, instr_index, param, programstack[1]);
                goto fail;
            }
            // clean up stack frame
            QVM_PROGRAMSTACK_FRAME(-param);
            // if RII from previous frame is our negative sentinel, signal end of instruction loop
            if (programstack[0] < 0)
                opptr = NULL;
            else
                QVM_JUMP(programstack[0]);
            break;

        case OP_CALL: {
            // call a function:
            // address in stack[0]
            int jump_to = stack[0];
            QVM_POP();

            // negative address means an engine trap
            if (jump_to < 0) {
                // store local stack pointer in qvm object for re-entrancy
                qvm->stackptr = programstack;

                // pass call to game-specific syscall handler which will adjust pointer arguments
                // and then call the normal QMM syscall entry point so it can be routed to plugins
                int ret = qvm->vmsyscall(qvm->datasegment, -jump_to - 1, &programstack[2]);

                // stack pointer in qvm object may have changed
                programstack = qvm->stackptr;

                // place return value on top of stack like a VM function return value
                QVM_PUSH();
                stack[0] = ret;
                break;
            }
            // otherwise, normal VM function call

            // place RII in top slot of program stack
            programstack[0] = (int)(opptr - qvm->codesegment);

            // jump to VM function at address
            QVM_JUMP(jump_to);
            break;
        }

        // stack opcodes

        case OP_PUSH:
            // pushes an unused value onto the stack (mostly for unused return values)
            QVM_PUSH();
            stack[0] = 0;
            break;

        case OP_POP:
            // pops the top value off the stack (mostly for unused return values)
            QVM_POP();
            break;

        case OP_CONST:
            // pushes a hardcoded value onto the stack
            QVM_PUSH();
            stack[0] = param;
            break;

        case OP_LOCAL:
            // pushes a specified local variable address (relative to start of data segment) onto the stack
            QVM_PUSH();
            stack[0] = (int)((uint8_t*)programstack + param - qvm->datasegment);
            break;

        // branching

// signed integer comparison
#define QVM_JUMP_SIF(o) if (stack[1] o stack[0]) { QVM_JUMP(param); } QVM_POPN(2)
// unsigned integer comparison
#define QVM_JUMP_UIF(o) if (*(unsigned int*)&stack[1] o *(unsigned int*)&stack[0]) { QVM_JUMP(param); } QVM_POPN(2)
// floating point comparison
#define QVM_JUMP_FIF(o) if (*(float*)&stack[1] o *(float*)&stack[0]) { QVM_JUMP(param); } QVM_POPN(2)

        case OP_JUMP:
            // jump to address in stack[0]
            QVM_JUMP(stack[0]);
            QVM_POP();
            break;

        case OP_EQ:
            // if stack[1] == stack[0], goto address in param
            QVM_JUMP_SIF( == );
            break;

        case OP_NE:
            // if stack[1] != stack[0], goto address in param
            QVM_JUMP_SIF( != );
            break;

        case OP_LTI:
            // if stack[1] < stack[0], goto address in param
            QVM_JUMP_SIF( < );
            break;

        case OP_LEI:
            // if stack[1] <= stack[0], goto address in param
            QVM_JUMP_SIF( <= );
            break;

        case OP_GTI:
            // if stack[1] > stack[0], goto address in param
            QVM_JUMP_SIF( > );
            break;

        case OP_GEI:
            // if stack[1] >= stack[0], goto address in param
            QVM_JUMP_SIF( >= );
            break;

        case OP_LTU:
            // if stack[1] < stack[0], goto address in param (unsigned)
            QVM_JUMP_UIF( < );
            break;

        case OP_LEU:
            // if stack[1] <= stack[0], goto address in param (unsigned)
            QVM_JUMP_UIF( <= );
            break;

        case OP_GTU:
            // if stack[1] > stack[0], goto address in param (unsigned)
            QVM_JUMP_UIF( > );
            break;

        case OP_GEU:
            // if stack[1] >= stack[0], goto address in param (unsigned)
            QVM_JUMP_UIF( >= );
            break;

        case OP_EQF:
            // if stack[1] == stack[0], goto address in param (float)
            QVM_JUMP_FIF( == );
            break;

        case OP_NEF:
            // if stack[1] != stack[0], goto address in param (float)
            QVM_JUMP_FIF( != );
            break;

        case OP_LTF:
            // if stack[1] < stack[0], goto address in param (float)
            QVM_JUMP_FIF( < );
            break;

        case OP_LEF:
            // if stack[1] <= stack[0], goto address in param (float)
            QVM_JUMP_FIF( <= );
            break;

        case OP_GTF:
            // if stack[1] > stack[0], goto address in param (float)
            QVM_JUMP_FIF( > );
            break;

        case OP_GEF:
            // if stack[1] >= stack[0], goto address in param (float)
            QVM_JUMP_FIF( >= );
            break;

        // memory/pointer management

        case OP_LOAD1: {
            // get 1-byte value at address stored in stack[0] and store back in stack[0]
            uint8_t* src = qvm->datasegment + stack[0];
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s pointer validation failed! ptr = %p\n", vmMain_cmd, instr_index, opcodename[op], src);
                goto fail;
            }
            stack[0] = (int)*src;
            break;
        }

        case OP_LOAD2: {
            // get 2-byte value at address stored in stack[0] and store back in stack[0]
            uint16_t* src = (uint16_t*)(qvm->datasegment + stack[0]);
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s pointer validation failed! ptr = %p\n", vmMain_cmd, instr_index, opcodename[op], src);
                goto fail;
            }
            stack[0] = (int)*src;
            break;
        }

        case OP_LOAD4: {
            // get 4-byte value at address stored in stack[0] and store back in stack[0]
            int* src = (int*)(qvm->datasegment + stack[0]);
            if (!qvm_validate_ptr_data(qvm, src)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s pointer validation failed! ptr = %p\n", vmMain_cmd, instr_index, opcodename[op], src);
                goto fail;
            }
            stack[0] = *src;
            break;
        }

        case OP_STORE1: {
            // store 1-byte value from stack[0] into address stored in stack[1]
            uint8_t* dst = qvm->datasegment + stack[1];
            if (!qvm_validate_ptr_data(qvm, dst)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s pointer validation failed! ptr = %p\n", vmMain_cmd, instr_index, opcodename[op], dst);
                goto fail;
            }
            *dst = (uint8_t)(stack[0] & 0xFF);
            QVM_POPN(2);
            break;
        }

        case OP_STORE2: {
            // store 2-byte value from stack[0] into address stored in stack[1] 
            unsigned short* dst = (unsigned short*)(qvm->datasegment + stack[1]);
            if (!qvm_validate_ptr_data(qvm, dst)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s pointer validation failed! ptr = %p\n", vmMain_cmd, instr_index, opcodename[op], dst);
                goto fail;
            }
            *dst = (unsigned short)(stack[0] & 0xFFFF);
            QVM_POPN(2);
            break;
        }

        case OP_STORE4: {
            // store 4-byte value from stack[0] into address stored in stack[1]
            int* dst = (int*)(qvm->datasegment + stack[1]);
            if (!qvm_validate_ptr_data(qvm, dst)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s pointer validation failed! ptr = %p\n", vmMain_cmd, instr_index, opcodename[op], dst);
                goto fail;
            }
            *dst = stack[0];
            QVM_POPN(2);
            break;
        }

        case OP_ARG:
            // set a function-call arg (offset = param) to the value on top of stack
            *(int*)((uint8_t*)programstack + param) = stack[0];
            QVM_POP();
            break;

        case OP_BLOCK_COPY: {
            // copy mem from address in stack[0] to address in stack[1] for 'param' number of bytes
            uint8_t* src = qvm->datasegment + stack[0];
            uint8_t* dst = qvm->datasegment + stack[1];

            QVM_POPN(2);

            // skip if src/dst are the same
            if (src == dst)
                break;

            // check if src block goes out of VM range
            if (!qvm_validate_ptr_data(qvm, src) || !qvm_validate_ptr_data(qvm, src + param - 1)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s source pointer validation failed! ptr = %p\n", vmMain_cmd, instr_index, opcodename[op], src);
                goto fail;
            }
            // check if dst block goes out of VM range
            if (!qvm_validate_ptr_data(qvm, dst) || !qvm_validate_ptr_data(qvm, dst + param - 1)) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s destination pointer validation failed! ptr = %p\n", vmMain_cmd, instr_index, opcodename[op], dst);
                goto fail;
            }

            memcpy(dst, src, param);

            break;
        }

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

        // arithmetic/operators

// signed integer (stack[0] done to stack[1], stored in stack[1])
#define QVM_SOP(o) stack[1] o stack[0]; QVM_POP()
// unsigned integer (stack[0] done to stack[1], stored in stack[1])
#define QVM_UOP(o) *(unsigned int*)&stack[1] o *(unsigned int*)&stack[0]; QVM_POP()
// floating point (stack[0] done to stack[1], stored in stack[1])
#define QVM_FOP(o) *(float*)&stack[1] o *(float*)&stack[0]; QVM_POP()
// signed integer (done to self)
#define QVM_SSOP(o) stack[0] = o stack[0]
// floating point (done to self)
#define QVM_SFOP(o) *(float*)&stack[0] = o *(float*)&stack[0]

        case OP_NEGI:
            // negation
            QVM_SSOP( - );
            break;

        case OP_ADD:
            // addition
            QVM_SOP( += );
            break;

        case OP_SUB:
            // subtraction
            QVM_SOP( -= );
            break;

        case OP_DIVI:
            // division
            if (!stack[0]) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s division by 0!\n", vmMain_cmd, instr_index, opcodename[op]);
                goto fail;
            }
            QVM_SOP( /= );
            break;

        case OP_DIVU:
            // unsigned division
            if (!stack[0]) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s division by 0!\n", vmMain_cmd, instr_index, opcodename[op]);
                goto fail;
            }
            QVM_UOP( /= );
            break;

        case OP_MODI:
            // modulus
            if (!stack[0]) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s division by 0!\n", vmMain_cmd, instr_index, opcodename[op]);
                goto fail;
            }
            QVM_SOP( %= );
            break;

        case OP_MODU:
            // unsigned modulus
            if (!stack[0]) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s division by 0!\n", vmMain_cmd, instr_index, opcodename[op]);
                goto fail;
            }
            QVM_UOP( %= );
            break;

        case OP_MULI:
            // multiplication
            QVM_SOP( *= );
            break;

        case OP_MULU:
            // unsigned multiplication
            QVM_UOP( *= );
            break;

        case OP_BAND:
            // bitwise AND
            QVM_SOP( &= );
            break;

        case OP_BOR:
            // bitwise OR
            QVM_SOP( |= );
            break;

        case OP_BXOR:
            // bitwise XOR
            QVM_SOP( ^= );
            break;

        case OP_BCOM:
            // bitwise one's compliment
            QVM_SSOP( ~ );
            break;

        case OP_LSH:
            // unsigned bitwise LEFTSHIFT
            QVM_UOP( <<= );
            break;

        case OP_RSHI:
            // bitwise RIGHTSHIFT
            QVM_SOP( >>= );
            break;

        case OP_RSHU:
            // unsigned bitwise RIGHTSHIFT
            QVM_UOP( >>= );
            break;

        case OP_NEGF:
            // float negation
            QVM_SFOP( - );
            break;

        case OP_ADDF:
            // float addition
            QVM_FOP( += );
            break;

        case OP_SUBF:
            // float subtraction
            QVM_FOP( -= );
            break;

        case OP_DIVF:
            // float division
            // float 0s are all 0s but with either sign bit
            if (stack[0] == 0 || stack[0] == 0x80000000) {
                log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): Runtime error at %d: %s division by 0!\n", vmMain_cmd, instr_index, opcodename[op]);
                goto fail;
            }
            QVM_FOP( /= );
            break;

        case OP_MULF:
            // float multiplication
            QVM_FOP( *= );
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
    if (programstack[1] != framesize) {
        log_c(QMM_LOG_FATAL, "QMM", "qvm_exec(%d): exit argsize (%d) does not match enter argsize (%d)\n", vmMain_cmd, programstack[1], framesize);
        goto fail;
    }

    // remove initial stack frame like in OP_LEAVE
    QVM_PROGRAMSTACK_FRAME(-framesize);

    // save our local stack pointer back into the qvm object
    qvm->stackptr = programstack;

    // decrement exec_depth
    qvm->exec_depth--;

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
