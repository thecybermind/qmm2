/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define QMM_LOGGING

#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>     // intptr_t and uint8_t
#include <malloc.h>
#include <string.h>     // memcpy and memset
#include <stdio.h>
#include <time.h>       // struct timespec and timespec_get
#include "qvm.h"

#ifdef QMM_LOGGING
void log_c(int severity, const char* tag, const char* fmt, ...);
enum { QMM_LOG_TRACE, QMM_LOG_DEBUG, QMM_LOG_INFO, QMM_LOG_NOTICE, QMM_LOG_WARNING, QMM_LOG_ERROR, QMM_LOG_FATAL };
#define QMM_LOGGING_TAG "QMM"
#else
#define log_c(...) /* */
#endif


int qvm_load(qvm* vm, const uint8_t* filemem, size_t filesize, qvm_syscall qvmsyscall, int verify_data, size_t hunk_size, qvm_alloc* allocator) {
    if (!vm || vm->memory || !filemem || !filesize || !qvmsyscall)
        return 0;

    if (filesize < sizeof(qvm_header)) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: too small for header\n");
        goto fail;
    }
    
    vm->filesize = filesize;
    vm->syscall = qvmsyscall;
    vm->verify_data = verify_data;
    // if null, use default allocator (uses malloc/free)
    vm->allocator = allocator ? allocator : &qvm_allocator_default;
    vm->hunksize = hunk_size ? hunk_size : QVM_HUNK_SIZE;

    qvm_header header;

    // grab a copy of the header
    memcpy(&header, filemem, sizeof(header));

    // check header fields for oddities
    if (header.magic != QVM_MAGIC && header.magic != QVM_MAGIC_VER2) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: incorrect magic number\n");
        goto fail;
    }
    if (filesize < sizeof(header) + header.codelen + header.datalen + header.litlen) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: filesize too small for segment sizes\n");
        goto fail;
    }
    if (header.codeoffset < sizeof(header) ||
        header.codeoffset > filesize ||
        header.codeoffset + header.codelen > filesize) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: code offset/length has invalid value\n");
        goto fail;
    }
    if (header.dataoffset < sizeof(header) ||
        header.dataoffset > filesize ||
        header.dataoffset + header.datalen + header.litlen > filesize) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: data offset/length has invalid value\n");
        goto fail;
    }
    if (header.instructioncount < header.codelen / 5 || // assume each op in the code segment is 5 bytes for a minimum
        header.instructioncount > header.codelen) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: numops has invalid value\n");
        goto fail;
    }

    // store magic number
    vm->magic = header.magic;

    // store numops in qvm object
    vm->instructioncount = header.instructioncount;

    // each opcode is 8 bytes long, calculate total size of instructions
    size_t codeseglen = header.instructioncount * sizeof(qvm_op);
    if (!codeseglen) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: code segment length is 0\n");
        goto fail;
    }
    // the q3 engine rounds the data segment size up to the next power of 2 for masking data accesses,
    // but we can also do that with code segment too. the remainder of the code segment will be filled
    // out with byte 0 (QVM_OP_UNDEF) which will immediately fail if the instruction pointer ends up
    // in that space
    QVM_NEXT_POW_2(codeseglen);
    vm->codeseglen = codeseglen;
    
    // data segment is the total size of the individual data segments
    // bsslen includes QVM_PROGRAMSTACK_SIZE 
    // also add in the size of the hunk
    size_t dataseglen = header.datalen + header.litlen + header.bsslen + vm->hunksize;
    if (!dataseglen) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: data segment length is 0\n");
        goto fail;
    }
    // save actual dataseglen before rounding up
    size_t orig_dataseglen = dataseglen;
    // round data segment size up to next power of 2 for masking data accesses
    QVM_NEXT_POW_2(dataseglen);
    vm->dataseglen = dataseglen;

    // the stack will also use all extra space from rounding up the data segment size
    vm->stacksize = QVM_PROGRAMSTACK_SIZE + (dataseglen - orig_dataseglen);

    // allocate vm memory
    vm->memorysize = vm->codeseglen + vm->dataseglen;
    vm->memory = (uint8_t*)vm->allocator->alloc(vm->memorysize, vm->allocator->ctx);
    if (!vm->memory) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Memory allocation failed for size %zu\n", vm->memorysize);
        goto fail;
    }

    // zero out memory
    memset(vm->memory, 0, vm->memorysize);

    // set segment pointers
    // the stack is allocated by q3asm as the final symbol in BSS segment
    // | CODE | DATA | LIT | BSS | extrastack from dataseg rounding | hunk |
    //                 program stack starts here and grows down ----^
    // program stack is for arguments and local variables
    vm->codesegment = (qvm_op*)vm->memory;
    vm->datasegment = vm->memory + codeseglen;

    // hunk is at the very end of the data segment
    vm->hunkptr = (int)dataseglen;
    vm->hunkhigh = vm->hunkptr;
    vm->hunklow = vm->hunkhigh - (int)vm->hunksize;

    // set bounds of stack (last chunk of bss and up to hunk)
    vm->stackptr = (int*)(vm->datasegment + dataseglen - vm->hunksize);
    vm->stackhigh = vm->stackptr;
    vm->stacklow = (int*)((uint8_t*)vm->stackhigh - vm->stacksize);

    // start loading instructions from the file's code offset into VM memory block
    const uint8_t* codeoffset = filemem + header.codeoffset;

    // loop through each op
    for (uint32_t i = 0; i < header.instructioncount; ++i) {
        // make sure we're not reading past the end of the codesegment in the file
        if (codeoffset >= filemem + header.codeoffset + header.codelen) {
            log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: can't read instruction at %ud, reached end of file\n", i);
            goto fail;
        }

        // get the opcode
        qvm_opcode opcode = (qvm_opcode)*codeoffset;

        // make sure opcode is valid
        if (opcode < 0 || opcode >= QVM_OP_NUM_OPS) {
            log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: invalid opcode value at %ud: %d\n", i, opcode);
            goto fail;
        }
        
        // write opcode (to qvmop_t)
        vm->codesegment[i].op = opcode;

        // move to next byte
        codeoffset++;

        switch (opcode) {
        case QVM_OP_EQ:
        case QVM_OP_NE:
        case QVM_OP_LTI:
        case QVM_OP_LEI:
        case QVM_OP_GTI:
        case QVM_OP_GEI:
        case QVM_OP_LTU:
        case QVM_OP_LEU:
        case QVM_OP_GTU:
        case QVM_OP_GEU:
        case QVM_OP_EQF:
        case QVM_OP_NEF:
        case QVM_OP_LTF:
        case QVM_OP_LEF:
        case QVM_OP_GTF:
        case QVM_OP_GEF:
        case QVM_OP_ENTER:
        case QVM_OP_LEAVE:
        case QVM_OP_CONST:
        case QVM_OP_LOCAL:
        case QVM_OP_BLOCK_COPY:
            // all the above instructions have 4-byte params
            // make sure we're not reading an int past the end of the codesegment in the file
            if (codeoffset + 3 >= filemem + header.codeoffset + header.codelen) {
                log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: can't read instruction %ud, reached end of file\n", i);
                goto fail;
            }
            vm->codesegment[i].param = *(int*)codeoffset;
            codeoffset += 4;
            break;

        case QVM_OP_ARG:
            // this instruction has a 1-byte param
            // make sure we're not reading past the end of the codesegment in the file
            if (codeoffset >= filemem + header.codeoffset + header.codelen) {
                log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: can't read instruction %ud, reached end of file\n", i);
                goto fail;
            }
            vm->codesegment[i].param = (int)*codeoffset;
            codeoffset++;
            break;

        default:
            // remaining instructions have no param
            vm->codesegment[i].param = 0;
            break;
        }
    }

    // copy data segment (including literals) to VM
    memcpy(vm->datasegment, filemem + header.dataoffset, header.datalen + header.litlen);

    // a winner is us
    return 1;

fail:
    // :(
    qvm_unload(vm);
    return 0;
}


static qvm qvm_empty;
void qvm_unload(qvm* vm) {
    if (!vm)
        return;
    if (vm->memory)
        vm->allocator->free(vm->memory, vm->memorysize, vm->allocator->ctx);
    *vm = qvm_empty;
}


int qvm_exec(qvm* vm, int argc, int* argv) {
    return qvm_exec_ex(vm, 0, argc, argv);
}


int qvm_exec_ex(qvm* vm, size_t instruction, int argc, int* argv) {
    if (!vm || !vm->memory) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_exec(%zu): Initialization error: given qvm is not loaded.\n", instruction);
        return 0;
    }

    // make sure instruction is in range
    if (instruction >= vm->instructioncount) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_exec(%zu): Initialization error: given instruction id %zu is out of range [0, %zu).\n", instruction, instruction, vm->instructioncount);
        return 0;
    }

    // start instruction pointer at given instruction
    qvm_op* opptr = vm->codesegment + instruction;

    // make sure instruction points to a QVM_OP_ENTER
    if (opptr->op != QVM_OP_ENTER) {
        const char* opname = "unknown";
        if (opptr->op >= 0 && opptr->op < QVM_OP_NUM_OPS)
            opname = qvm_opcodename[opptr->op];
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_exec(%zu): Initialization error: instruction at %zu is %s (%d), expected QVM_OP_ENTER (3).\n", instruction, instruction, opname, opptr->op);
        return 0;
    }

    // local copy of code segment pointer
    qvm_op* codesegment = vm->codesegment;

    // set up bitmasks for safety
    // code mask (masking qvmop_t indexes)
    size_t codemask = (vm->codeseglen / sizeof(qvm_op)) - 1;

    // local copy of data segment pointer
    uint8_t* datasegment = vm->datasegment;

    // data mask - disable if verify_data is off by using a mask with all bits 1
    size_t datamask = vm->verify_data ? vm->dataseglen - 1 : (size_t)-1;

    // local "register" copy of stack pointer. this is purely for locality/speed.
    // it gets synced to qvm object before syscalls and restored after syscalls.
    // it also gets synced back to qvm object after execution completes
    int* programstack = vm->stackptr;
    // local copies of stack boundaries
    int* stacklow = vm->stacklow;
    int* stackhigh = vm->stackhigh;

    // size of new stack frame, need to store RII, framesize, and vmMain args
    if (!argv)
        argc = 0;
    int framesize = (argc + 2) * sizeof(argv[0]);
    // create new stack frame
    QVM_STACKFRAME(framesize);
    // set up new stack frame
    programstack[0] = -1;           // sentinel return instruction index (RII)
    programstack[1] = framesize;    // store the frame size like we store param in QVM_OP_ENTER
    // copy qvm_exec arguments onto program stack starting at programstack[2]
    if (argv && argc > 0)
        memcpy(&programstack[2], argv, argc * sizeof(argv[0]));

    /* programstack frame example: a "|" separates stack cells, while a "||" separates stack frames
     *
     * || RII | size | arg0 | arg1 | local0 | local1 || RII | size | arg0 | local0 || -1 | size | cmd | arg0 | arg1 | ...
     * ^ "top" of stack, lowest address                     "bottom" of stack or "end of block", highest address --->
     *
     * When a function is to be called, the arguments are set in the current stack frame using QVM_OP_ARG in the slots
     * marked "arg#". Then the address of the function to be called is placed onto the top of the opstack (either by
     * loading a function pointer with QVM_OP_LOCAL or QVM_OP_LOADx or a real function with QVM_OP_CONST). Then, the
     * QVM_OP_CALL instruction is used. QVM_OP_CALL will place the current instruction index into the RII slot of the
     * current stack frame (programstack[0]), then it will pop the function instruction index from the opstack and
     * jump to it.
     *
     * The next instruction should be the callee's QVM_OP_ENTER instruction. QVM_OP_ENTER will add a new stack frame
     * with a hardcoded size param, then stores that size in programstack[1] and leaves programstack[0] (RII) empty.
     *
     * Just before a function exits, a return value is pushed onto the top of the opstack. The final instruction in a
     * function is QVM_OP_LEAVE. QVM_OP_LEAVE will check programstack[1] to see if it matches its own hardcoded size
     * param, and then remove the stack frame of the given size. It then looks in programstack[0] of the previous
     * frame (now topmost) for the RII and jumps to it. If the RII is <0 (-1 is set in the first stack frame created
     * before execution), it signals to end VM execution.
     *
     * Within a function, arguments are loaded by just accessing the "arg#" values from the previous stack frame with
     * QVM_OP_LOCAL.
     */

    // opstack for math/comparison/temp/etc operations (instead of using registers)
    // +2 for some extra space to "harmlessly" read 2 values (like QVM_OP_BLOCK_COPY) if opstack is empty (like at start)
    int opstacklow[QVM_OPSTACK_SIZE + 2];
    memset(opstacklow, 0, sizeof(opstacklow));
    // opstack pointer (starts at end of block, grows down)
    int* opstack = opstacklow + QVM_OPSTACK_SIZE;
    // save upper bound of opstack for bounds checking
    int* opstackhigh = opstack;

    // current op
    qvm_opcode op;
    // hardcoded param for op
    int param;

#if defined(_DEBUG)
    struct timespec time_start;
    (void)timespec_get(&time_start, TIME_UTC);
#endif
    
    // main instruction loop
    do {
        // verify program stack pointer is in program stack
        // using > to allow starting at 1 past the end of block
        if (programstack <= stacklow || programstack > stackhigh) {
            ptrdiff_t stackusage = (uint8_t*)stackhigh - (uint8_t*)programstack;
            log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: program stack overflow! Program stack size is currently %td, max is %zu.\n", instruction, opptr - codesegment, stackusage, vm->stacksize);
            goto fail;
        }
        // verify opstack pointer is in op stack
        // using > to allow starting at 1 past the end of block
        if (opstack <= opstacklow || opstack > opstackhigh) {
            ptrdiff_t stackusage = (uint8_t*)opstackhigh - (uint8_t*)opstack;
            log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: opstack overflow! Opstack size is currently %td, max is %d.\n", instruction, opptr - codesegment, stackusage, QVM_OPSTACK_SIZE);
            goto fail;
        }

        // get the instruction's opcode and param
        op = (qvm_opcode)opptr->op;
        param = opptr->param;

        // throughout the interpreter loop, opptr points to the next instruction
        opptr++;

        switch (op) {
        // miscellaneous opcodes

        case QVM_OP_UNDEF:
            // undefined - used as alignment padding at end of codesegment. treat as error
            // explicit fallthrough
        default:
            // anything else
            log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: unhandled opcode %d\n", instruction, opptr - 1 - codesegment, op);
            goto fail;

        case QVM_OP_NOP:
            // no op
            break;

        case QVM_OP_BREAK:
            // break to debugger, dump qvm info
            vm->stackptr = programstack;
            qvm_dump(vm, opstack, opstackhigh, opptr - 1);
            break;

        // functions

        case QVM_OP_ENTER:
            // enter a function:
            // prepare new stack frame on program stack (size=param).
            // store param in programstack[1]. this gets verified to match in QVM_OP_LEAVE.
            QVM_STACKFRAME(param);
            programstack[0] = 0; // leave blank. an QVM_OP_CALL within this function will place RII here
            programstack[1] = param;
            break;

        case QVM_OP_LEAVE:
            // leave a function:
            // verify the value saved in programstack[1] matches param, then remove stack frame (size=param).
            // then, grab RII from top of previous stack frame and then jump to it
            if (programstack[1] != param) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: QVM_OP_LEAVE param (%d) does not match QVM_OP_ENTER param (%d)\n", instruction, opptr - 1 - codesegment, param, programstack[1]);
                goto fail;
            }
            // clean up stack frame
            QVM_STACKFRAME(-param);
            // if RII from previous frame is our negative sentinel, signal end of instruction loop
            if (programstack[0] < 0)
                opptr = NULL;
            else
                QVM_JUMP(programstack[0]);
            break;

        case QVM_OP_CALL: {
            // call a function:
            // address in opstack[0]
            int jump_to = opstack[0];
            QVM_POP();

            // negative address means an engine trap
            if (jump_to < 0) {
                // store local program stack pointer in qvm object for re-entrancy
                vm->stackptr = programstack;

                // pass call to game-specific syscall handler which will adjust pointer arguments
                // and then call the normal QMM syscall entry point so it can be routed to plugins
                int ret = vm->syscall(datasegment, -jump_to - 1, &programstack[2]);

                // program stack pointer in qvm object may have changed if re-entrant
                programstack = vm->stackptr;

                // place return value on top of opstack like a VM function return value
                QVM_PUSH(ret);
                break;
            }
            // otherwise, normal VM function call

            // place RII in top slot of program stack
            programstack[0] = (int)(opptr - codesegment);

            // jump to VM function at address
            QVM_JUMP(jump_to);
            break;
        }

        // stack opcodes

        case QVM_OP_PUSH:
            // pushes an unused value onto the opstack (mostly for unused return values)
            QVM_PUSH(0);
            break;

        case QVM_OP_POP:
            // pops the top value off the opstack (mostly for unused return values)
            QVM_POP();
            break;

        case QVM_OP_CONST:
            // pushes a hardcoded value onto the opstack
            QVM_PUSH(param);
            break;

        case QVM_OP_LOCAL:
            // pushes a specified local variable address (relative to start of data segment) onto the opstack
            QVM_PUSH( (int)((uint8_t*)programstack + param - datasegment) );
            break;

        // branching

        case QVM_OP_JUMP:
            // jump to address in opstack[0]
            QVM_JUMP(opstack[0]);
            QVM_POP();
            break;

        case QVM_OP_EQ:
            // if opstack[1] == opstack[0], goto address in param
            QVM_JUMP_SIF( == );
            break;

        case QVM_OP_NE:
            // if opstack[1] != opstack[0], goto address in param
            QVM_JUMP_SIF( != );
            break;

        case QVM_OP_LTI:
            // if opstack[1] < opstack[0], goto address in param
            QVM_JUMP_SIF( < );
            break;

        case QVM_OP_LEI:
            // if opstack[1] <= opstack[0], goto address in param
            QVM_JUMP_SIF( <= );
            break;

        case QVM_OP_GTI:
            // if opstack[1] > opstack[0], goto address in param
            QVM_JUMP_SIF( > );
            break;

        case QVM_OP_GEI:
            // if opstack[1] >= opstack[0], goto address in param
            QVM_JUMP_SIF( >= );
            break;

        case QVM_OP_LTU:
            // if opstack[1] < opstack[0] (unsigned), goto address in param
            QVM_JUMP_UIF( < );
            break;

        case QVM_OP_LEU:
            // if opstack[1] <= opstack[0] (unsigned), goto address in param
            QVM_JUMP_UIF( <= );
            break;

        case QVM_OP_GTU:
            // if opstack[1] > opstack[0] (unsigned), goto address in param
            QVM_JUMP_UIF( > );
            break;

        case QVM_OP_GEU:
            // if opstack[1] >= opstack[0] (unsigned), goto address in param
            QVM_JUMP_UIF( >= );
            break;

        case QVM_OP_EQF:
            // if opstack[1] == opstack[0] (float), goto address in param
            QVM_JUMP_FIF( == );
            break;

        case QVM_OP_NEF:
            // if opstack[1] != opstack[0] (float), goto address in param
            QVM_JUMP_FIF( != );
            break;

        case QVM_OP_LTF:
            // if opstack[1] < opstack[0] (float), goto address in param
            QVM_JUMP_FIF( < );
            break;

        case QVM_OP_LEF:
            // if opstack[1] <= opstack[0] (float), goto address in param
            QVM_JUMP_FIF( <= );
            break;

        case QVM_OP_GTF:
            // if opstack[1] > opstack[0] (float), goto address in param
            QVM_JUMP_FIF( > );
            break;

        case QVM_OP_GEF:
            // if opstack[1] >= opstack[0] (float), goto address in param
            QVM_JUMP_FIF( >= );
            break;

        // memory/pointer management

        case QVM_OP_LOAD1: {
            // get 1-byte value at address stored in opstack[0] and store back in opstack[0]
            uint8_t* src = datasegment + (opstack[0] & datamask);
            opstack[0] = (int)*src;
            break;
        }

        case QVM_OP_LOAD2: {
            // get 2-byte value at address stored in opstack[0] and store back in opstack[0]
            uint16_t* src = (uint16_t*)(datasegment + (opstack[0] & datamask));
            opstack[0] = (int)*src;
            break;
        }

        case QVM_OP_LOAD4: {
            // get 4-byte value at address stored in opstack[0] and store back in opstack[0]
            int* src = (int*)(datasegment + (opstack[0] & datamask));
            opstack[0] = *src;
            break;
        }

        case QVM_OP_STORE1: {
            // store 1-byte value from opstack[0] into address stored in opstack[1]
            uint8_t* dst = datasegment + (opstack[1] & datamask);
            *dst = (uint8_t)(opstack[0] & 0xFF);
            QVM_POPN(2);
            break;
        }

        case QVM_OP_STORE2: {
            // store 2-byte value from opstack[0] into address stored in opstack[1] 
            uint16_t* dst = (uint16_t*)(datasegment + (opstack[1] & datamask));
            *dst = (uint16_t)(opstack[0] & 0xFFFF);
            QVM_POPN(2);
            break;
        }

        case QVM_OP_STORE4: {
            // store 4-byte value from opstack[0] into address stored in opstack[1]
            int* dst = (int*)(datasegment + (opstack[1] & datamask));
            *dst = opstack[0];
            QVM_POPN(2);
            break;
        }

        case QVM_OP_ARG:
            // set a function-call arg (offset = param) to the value on top of opstack
            *(int*)((uint8_t*)programstack + param) = opstack[0];
            QVM_POP();
            break;

        case QVM_OP_BLOCK_COPY: {
            // copy mem from address in opstack[0] to address in opstack[1] for 'param' number of bytes
            int srci = (opstack[0] & datamask);
            int dsti = (opstack[1] & datamask);

            QVM_POPN(2);

            // skip if src/dst are the same
            if (srci == dsti)
                break;

            // make sure the src and dst ranges don't go out of memory bounds
            int count = param;
            count = ((srci + count) & datamask) - srci;
            count = ((dsti + count) & datamask) - dsti;

            uint8_t* src = datasegment + srci;
            uint8_t* dst = datasegment + dsti;

            memcpy(dst, src, count);

            break;
        }

        // sign extensions

        case QVM_OP_SEX8:
            // 8-bit
            if (opstack[0] & 0x80)
                opstack[0] |= 0xFFFFFF00;
            break;

        case QVM_OP_SEX16:
            // 16-bit
            if (opstack[0] & 0x8000)
                opstack[0] |= 0xFFFF0000;
            break;

        // arithmetic/operators

        case QVM_OP_NEGI:
            // negation
            QVM_SSOP( - );
            break;

        case QVM_OP_ADD:
            // addition
            QVM_SOP( += );
            break;

        case QVM_OP_SUB:
            // subtraction
            QVM_SOP( -= );
            break;

        case QVM_OP_DIVI:
            // division
            if (opstack[0] == 0) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", instruction, opptr - 1 - codesegment, qvm_opcodename[op]);
                goto fail;
            }
            QVM_SOP( /= );
            break;

        case QVM_OP_DIVU:
            // unsigned division
            if (opstack[0] == 0) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", instruction, opptr - 1 - codesegment, qvm_opcodename[op]);
                goto fail;
            }
            QVM_UOP( /= );
            break;

        case QVM_OP_MODI:
            // modulus
            if (opstack[0] == 0) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", instruction, opptr - 1 - codesegment, qvm_opcodename[op]);
                goto fail;
            }
            QVM_SOP( %= );
            break;

        case QVM_OP_MODU:
            // unsigned modulus
            if (opstack[0] == 0) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", instruction, opptr - 1 - codesegment, qvm_opcodename[op]);
                goto fail;
            }
            QVM_UOP( %= );
            break;

        case QVM_OP_MULI:
            // multiplication
            QVM_SOP( *= );
            break;

        case QVM_OP_MULU:
            // unsigned multiplication
            QVM_UOP( *= );
            break;

        case QVM_OP_BAND:
            // bitwise AND
            QVM_SOP( &= );
            break;

        case QVM_OP_BOR:
            // bitwise OR
            QVM_SOP( |= );
            break;

        case QVM_OP_BXOR:
            // bitwise XOR
            QVM_SOP( ^= );
            break;

        case QVM_OP_BCOM:
            // bitwise one's compliment
            QVM_SSOP( ~ );
            break;

        case QVM_OP_LSH:
            // unsigned bitwise LEFTSHIFT
            QVM_UOP( <<= );
            break;

        case QVM_OP_RSHI:
            // bitwise RIGHTSHIFT
            QVM_SOP( >>= );
            break;

        case QVM_OP_RSHU:
            // unsigned bitwise RIGHTSHIFT
            QVM_UOP( >>= );
            break;

        case QVM_OP_NEGF:
            // float negation
            QVM_SFOP( - );
            break;

        case QVM_OP_ADDF:
            // float addition
            QVM_FOP( += );
            break;

        case QVM_OP_SUBF:
            // float subtraction
            QVM_FOP( -= );
            break;

        case QVM_OP_DIVF:
            // float division
            QVM_FOP( /= );
            break;

        case QVM_OP_MULF:
            // float multiplication
            QVM_FOP( *= );
            break;

        // format conversion

        case QVM_OP_CVIF:
            // convert opstack[0] int->float
            *(float*)&opstack[0] = (float)opstack[0];
            break;

        case QVM_OP_CVFI:
            // convert opstack[0] float->int
            opstack[0] = (int)*(float*)&opstack[0];
            break;
        } // switch (op)
    } while (opptr);

#if defined(_DEBUG)
    struct timespec time_end;
    (void)timespec_get(&time_end, TIME_UTC);
    int64_t timediff = (time_end.tv_sec - time_start.tv_sec) * 1000000000;
    timediff += (time_end.tv_nsec - time_start.tv_nsec);
    log_c(QMM_LOG_TRACE, QMM_LOGGING_TAG, "qvm_exec(%zu): Execution took %lld nanoseconds.\n", instruction, timediff);
#endif

    // compare stored frame size like in QVM_OP_LEAVE
    if (programstack[1] != framesize) {
        log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error after execution: stack frame size (%d) does not match entry stack frame size (%d)\n", instruction, programstack[1], framesize);
        goto fail;
    }

    // remove initial stack frame like in QVM_OP_LEAVE
    QVM_STACKFRAME(-framesize);

    // save our local program stack pointer back into the qvm object
    vm->stackptr = programstack;

    // return value is stored on the top of the opstack (pushed just before QVM_OP_LEAVE)
    return opstack[0];

fail:
    // save our local program stack pointer back into the qvm object (for dump)
    vm->stackptr = programstack;

    // generate dumps
    qvm_dump(vm, opstack, opstackhigh, opptr - 1);
    
    // unload
    qvm_unload(vm);
    return 0;
}


int qvm_hunk_alloc(qvm* vm, size_t size, const void* init) {
    if (!vm->memory) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_hunk_alloc(): VM not loaded\n");
        return 0;
    }

    // size 0 or not enough space left for size
    if (!size || (vm->hunkptr - (int)size < vm->hunklow)){
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_hunk_alloc(): Hunk allocation failed for size %zu\n", size);
        return 0;
    }

    vm->hunkptr -= (int)size;

    if (init)
        memcpy(vm->datasegment + vm->hunkptr, init, size);
    else
        memset(vm->datasegment + vm->hunkptr, 0, size);

    return vm->hunkptr;
}


void qvm_hunk_free(qvm* vm, int ptr, size_t size, void* out) {
    if (!vm->memory) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_hunk_free(): VM not loaded\n");
        return;
    }

    // "null" pointer
    if (!ptr) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_hunk_free(): Trying to free null pointer\n");
        return;
    }

    // if this ptr was not the most recently-allocated block, fail
    if (ptr != vm->hunkptr) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_hunk_free(): Trying to free out of order: got %d, expected %d\n", ptr, vm->hunkptr);
        return;
    }

    // size too big or 0
    if (!size || (ptr + (int)size > vm->hunkhigh)) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_hunk_free(): Trying to free invalid size %zu\n", size);
        return;
    }

    // get memory back out
    if (out)
        memcpy(out, vm->datasegment + ptr, size);

    vm->hunkptr += (int)size;
}


void qvm_dump(qvm* vm, int* opstack, int* opstackhigh, qvm_op* instruction) {
    if (!vm->memory)
        return;

    // dump code segment to file
    FILE* fp = fopen("qvm_dump_code.bin", "w");
    fwrite(vm->codesegment, 1, vm->codeseglen, fp);
    fclose(fp);
    
    // dump data segment to file
    fp = fopen("qvm_dump_data.bin", "w");
    fwrite(vm->datasegment, 1, vm->dataseglen, fp);
    fclose(fp);

    // dump annotated info to file
    fp = fopen("qvm_dump.txt", "w");
    fputs("VM state:\n-----\n", fp);

    // op stack
    int* opstackptr = opstack;
    fputs("Opstack (hex): ", fp);
    while (opstackptr < opstackhigh) {
        fprintf(fp, "%08x ", *opstackptr);
        opstackptr++;
    }
    opstackptr = opstack;
    fputs("\nOpstack (dec): ", fp);
    while (opstackptr < opstackhigh) {
        fprintf(fp, "%d ", *opstackptr);
        opstackptr++;
    }
    fputs("\n\n", fp);

    // IP
    fprintf(fp, "Instruction pointer offset: %td (%08tx)\nInstruction: %s %d\n\n", instruction - vm->codesegment, instruction - vm->codesegment, qvm_opcodename[instruction->op], instruction->param);
    
    // program stack
    fputs("Stack:\n-----\n", fp);
    int* stackptr = vm->stackptr;
    while (stackptr < vm->stackhigh) {
        fprintf(fp, "0x%08x (%d) (RII)\n", stackptr[0], stackptr[0]);
        fprintf(fp, "0x%08x (%d) (Framesize)\n", stackptr[1], stackptr[1]);
        for (int i = 2; i < stackptr[1] / 4; i++)
            fprintf(fp, "0x%08x (%d)\n", stackptr[i], stackptr[i]);
        stackptr += (stackptr[1] / 4);
        fputs("-----\n", fp);
        fflush(fp);
    }
    fputs("\n", fp);

    // data segment
    fputs("Data segment:\n-----\n", fp);
    uint8_t* p = vm->datasegment;
    while (p < vm->datasegment + vm->dataseglen) {
        // print offset
        fprintf(fp, "%04tX ", p - vm->datasegment);

        // print hex values
        for (int b = 0; b < 32; b++) {
            // halfway through the row, print a gap
            if (b == 16)
                fputs("   ", fp);
            // if this row runs out of data before the end, print empty spaces
            if (p + b >= vm->datasegment + vm->dataseglen)
                fputs("   ", fp);
            else
                fprintf(fp, " %02X", p[b]);
        }

        fputs("    ", fp);

        // print characters
        for (int b = 0; b < 32; b++) {
            // halfway through the row, print a gap
            if (b == 16)
                fprintf(fp, " ");
            // if this row runs out of data before the end, print empty spaces
            if (p + b >= vm->datasegment + vm->dataseglen)
                fprintf(fp, " ");
            else
                fprintf(fp, "%c", (p[b] < 32 || p[b] >= 127) ? '.' : *(char*)&p[b]);
        }

        fprintf(fp, "\n");
        fflush(fp);

        p += 32;
    }
    fclose(fp);
}


// Array of strings of opcode names
const char* qvm_opcodename[] = {
    "QVM_OP_UNDEF",
    "QVM_OP_NOP",
    "QVM_OP_BREAK",
    "QVM_OP_ENTER",
    "QVM_OP_LEAVE",
    "QVM_OP_CALL",
    "QVM_OP_PUSH",
    "QVM_OP_POP",
    "QVM_OP_CONST",
    "QVM_OP_LOCAL",
    "QVM_OP_JUMP",
    "QVM_OP_EQ",
    "QVM_OP_NE",
    "QVM_OP_LTI",
    "QVM_OP_LEI",
    "QVM_OP_GTI",
    "QVM_OP_GEI",
    "QVM_OP_LTU",
    "QVM_OP_LEU",
    "QVM_OP_GTU",
    "QVM_OP_GEU",
    "QVM_OP_EQF",
    "QVM_OP_NEF",
    "QVM_OP_LTF",
    "QVM_OP_LEF",
    "QVM_OP_GTF",
    "QVM_OP_GEF",
    "QVM_OP_LOAD1",
    "QVM_OP_LOAD2",
    "QVM_OP_LOAD4",
    "QVM_OP_STORE1",
    "QVM_OP_STORE2",
    "QVM_OP_STORE4",
    "QVM_OP_ARG",
    "QVM_OP_BLOCK_COPY",
    "QVM_OP_SEX8",
    "QVM_OP_SEX16",
    "QVM_OP_NEGI",
    "QVM_OP_ADD",
    "QVM_OP_SUB",
    "QVM_OP_DIVI",
    "QVM_OP_DIVU",
    "QVM_OP_MODI",
    "QVM_OP_MODU",
    "QVM_OP_MULI",
    "QVM_OP_MULU",
    "QVM_OP_BAND",
    "QVM_OP_BOR",
    "QVM_OP_BXOR",
    "QVM_OP_BCOM",
    "QVM_OP_LSH",
    "QVM_OP_RSHI",
    "QVM_OP_RSHU",
    "QVM_OP_NEGF",
    "QVM_OP_ADDF",
    "QVM_OP_SUBF",
    "QVM_OP_DIVF",
    "QVM_OP_MULF",
    "QVM_OP_CVIF",
    "QVM_OP_CVFI"
};


static void* qvm_alloc_default(ptrdiff_t size, void* ctx) {
    (void)ctx;
    return malloc(size);
}


static void qvm_free_default(void* ptr, ptrdiff_t size, void* ctx) {
    (void)ctx; (void)size;
    free(ptr);
}


// Default VM allocator (uses malloc/free)
qvm_alloc qvm_allocator_default = { qvm_alloc_default, qvm_free_default, NULL };
