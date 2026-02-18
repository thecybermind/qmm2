/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define QMM_LOGGING

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "qvm.h"

#ifdef QMM_LOGGING
void log_c(int severity, const char* tag, const char* fmt, ...);
enum { QMM_LOG_TRACE, QMM_LOG_DEBUG, QMM_LOG_INFO, QMM_LOG_NOTICE, QMM_LOG_WARNING, QMM_LOG_ERROR, QMM_LOG_FATAL };
#define QMM_LOGGING_TAG "QMM"
#else
#define log_c(...) /* */
#endif


int qvm_load(qvm_t* qvm, const uint8_t* filemem, size_t filesize, qvmsyscall_t qvmsyscall, int verify_data, qvm_alloc_t* allocator) {
    if (!qvm || qvm->memory || !filemem || !filesize || !qvmsyscall)
        return 0;

    if (filesize < sizeof(qvmheader_t)) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: too small for header\n");
        goto fail;
    }
    
    qvm->filesize = filesize;
    qvm->qvmsyscall = qvmsyscall;
    qvm->verify_data = verify_data;
    // if null, use default allocator (uses malloc/free)
    qvm->allocator = allocator ? allocator : &qvm_allocator_default;

    qvmheader_t header;

    // grab a copy of the header
    memcpy(&header, filemem, sizeof(qvmheader_t));

    // check header fields for oddities
    if (header.magic != QVM_MAGIC) {
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

    // store numops in qvm object
    qvm->instructioncount = header.instructioncount;

    // each opcode is 8 bytes long, calculate total size of instructions
    size_t codeseglen = header.instructioncount * sizeof(qvmop_t);
    if (!codeseglen) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: code segment length is 0\n");
        goto fail;
    }
    // the q3 engine rounds the data segment size up to the next power of 2 for masking data accesses,
    // but we can also do that with code segment too. the remainder of the code segment will be filled
    // out with byte 0 (QVM_OP_UNDEF) which will immediately fail if the instruction pointer ends up
    // in that space
    QVM_NEXT_POW_2(codeseglen);
    qvm->codeseglen = codeseglen;
    
    // data segment is all the data segment lengths combined (plus optional extra stack space)
    size_t dataseglen = header.datalen + header.litlen + header.bsslen + QVM_EXTRA_PROGRAMSTACK_SIZE;
    if (!dataseglen) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: data segment length is 0\n");
        goto fail;
    }
    // save actual dataseglen before rounding up
    size_t orig_dataseglen = dataseglen;
    // round data segment size up to next power of 2 for masking data accesses
    QVM_NEXT_POW_2(dataseglen);
    qvm->dataseglen = dataseglen;

    // allow stack to use any extra space from rounding up
    qvm->stacksize = QVM_PROGRAMSTACK_SIZE + (dataseglen - orig_dataseglen) + QVM_EXTRA_PROGRAMSTACK_SIZE;

    // allocate vm memory
    qvm->memorysize = qvm->codeseglen + qvm->dataseglen;
    qvm->memory = (uint8_t*)qvm->allocator->alloc(qvm->memorysize, qvm->allocator->ctx);
    if (!qvm->memory) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Memory allocation failed for size %zu\n", qvm->memorysize);
        goto fail;
    }

    // zero out memory
    memset(qvm->memory, 0, qvm->memorysize);

    // set segment pointers
    // | CODE | DATA | <- program stack starts here and grows down
    // program stack is for arguments and local variables
    qvm->codesegment = (qvmop_t*)qvm->memory;
    qvm->datasegment = qvm->memory + qvm->codeseglen;
    qvm->stackptr = (int*)(qvm->datasegment + qvm->dataseglen);
    
    // set bounds of stack
    qvm->stackhigh = qvm->stackptr;
    qvm->stacklow = (int*)((uint8_t*)qvm->stackhigh - qvm->stacksize);

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
        qvmopcode_t opcode = (qvmopcode_t)*codeoffset;

        // make sure opcode is valid
        if (opcode < 0 || opcode >= QVM_OP_NUM_OPS) {
            log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: invalid opcode value at %ud: %d\n", i, opcode);
            goto fail;
        }
        
        // write opcode (to qvmop_t)
        qvm->codesegment[i].op = opcode;

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
            qvm->codesegment[i].param = *(int*)codeoffset;
            codeoffset += 4;
            break;

        case QVM_OP_ARG:
            // this instruction has a 1-byte param
            // make sure we're not reading past the end of the codesegment in the file
            if (codeoffset >= filemem + header.codeoffset + header.codelen) {
                log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_load(): Invalid QVM file: can't read instruction %ud, reached end of file\n", i);
                goto fail;
            }
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
    return qvm_exec_ex(qvm, 0, argc, argv);
}


int qvm_exec_ex(qvm_t* qvm, size_t instruction, int argc, int* argv) {
    if (!qvm || !qvm->memory) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_exec(%zu): Initialization error: given qvm is not loaded.\n", instruction);
        return 0;
    }

    // make sure instruction is in range
    if (instruction >= qvm->instructioncount) {
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_exec(%zu): Initialization error: given instruction id %zu is out of range [0, %zu).\n", instruction, instruction, qvm->instructioncount);
        return 0;
    }

    // start instruction pointer at given instruction
    qvmop_t* opptr = qvm->codesegment + instruction;

    // make sure instruction points to a QVM_OP_ENTER
    if (opptr->op != QVM_OP_ENTER) {
        const char* opname = "unknown";
        if (opptr->op >= 0 && opptr->op < QVM_OP_NUM_OPS)
            opname = opcodename[opptr->op];
        log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_exec(%zu): Initialization error: instruction at %zu is %s (%d), expected QVM_OP_ENTER (3).\n", instruction, instruction, opname, opptr->op);
        return 0;
    }

    // set up bitmasks for safety
    // code mask (masking qvmop_t indexes)
    size_t codemask = (qvm->codeseglen / sizeof(qvmop_t)) - 1;
    // data mask - disable if verify_data is off by using a mask with all bits 1
    size_t datamask = qvm->verify_data ? qvm->dataseglen - 1 : (size_t)-1;

    // local "register" copy of stack pointer. this is purely for locality/speed.
    // it gets synced to qvm object before syscalls and restored after syscalls.
    // it also gets synced back to qvm object after execution completes
    int* programstack = qvm->stackptr;

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
    qvmopcode_t op;
    // hardcoded param for op
    int param;

    // main instruction loop
    do {
        // verify program stack pointer is in program stack
        // using > to allow starting at 1 past the end of block
        if (programstack <= qvm->stacklow || programstack > qvm->stackhigh) {
            ptrdiff_t stackusage = (uint8_t*)qvm->stackhigh - (uint8_t*)programstack;
            log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: program stack overflow! Program stack size is currently %td, max is %zu.\n", instruction, opptr - qvm->codesegment, stackusage, qvm->stacksize);
            goto fail;
        }
        // verify opstack pointer is in op stack
        // using > to allow starting at 1 past the end of block
        if (opstack <= opstacklow || opstack > opstackhigh) {
            ptrdiff_t stackusage = (uint8_t*)opstackhigh - (uint8_t*)opstack;
            log_c(QMM_LOG_ERROR, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: opstack overflow! Opstack size is currently %td, max is %d.\n", instruction, opptr - qvm->codesegment, stackusage, QVM_OPSTACK_SIZE);
            goto fail;
        }

        // get the instruction's opcode and param
        op = (qvmopcode_t)opptr->op;
        param = opptr->param;
        
        // throughout opcode handling, opptr points to the NEXT instruction to execute
        ++opptr;

        switch (op) {
        // miscellaneous opcodes

        case QVM_OP_UNDEF:
            // undefined - used as alignment padding at end of codesegment. treat as error
            // explicit fallthrough
        default:
            // anything else
            // todo: dump stacks/memory?
            log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: unhandled opcode %d\n", instruction, opptr - qvm->codesegment, op);
            goto fail;

        case QVM_OP_NOP:
            // no op
            // explicit fallthrough
        case QVM_OP_BREAK:
            // break to debugger, treat as no op for now
            // todo: dump stacks/memory?
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
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: QVM_OP_LEAVE param (%d) does not match QVM_OP_ENTER param (%d)\n", instruction, opptr - qvm->codesegment, param, programstack[1]);
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
                qvm->stackptr = programstack;

                // pass call to game-specific syscall handler which will adjust pointer arguments
                // and then call the normal QMM syscall entry point so it can be routed to plugins
                int ret = qvm->qvmsyscall(qvm->datasegment, -jump_to - 1, &programstack[2]);

                // program stack pointer in qvm object may have changed if re-entrant
                programstack = qvm->stackptr;

                // place return value on top of opstack like a VM function return value
                QVM_PUSH(ret);
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
            QVM_PUSH( (int)((uint8_t*)programstack + param - qvm->datasegment) );
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
            uint8_t* src = qvm->datasegment + (opstack[0] & datamask);
            opstack[0] = (int)*src;
            break;
        }

        case QVM_OP_LOAD2: {
            // get 2-byte value at address stored in opstack[0] and store back in opstack[0]
            uint16_t* src = (uint16_t*)(qvm->datasegment + (opstack[0] & datamask));
            opstack[0] = (int)*src;
            break;
        }

        case QVM_OP_LOAD4: {
            // get 4-byte value at address stored in opstack[0] and store back in opstack[0]
            int* src = (int*)(qvm->datasegment + (opstack[0] & datamask));
            opstack[0] = *src;
            break;
        }

        case QVM_OP_STORE1: {
            // store 1-byte value from opstack[0] into address stored in opstack[1]
            uint8_t* dst = qvm->datasegment + (opstack[1] & datamask);
            *dst = (uint8_t)(opstack[0] & 0xFF);
            QVM_POPN(2);
            break;
        }

        case QVM_OP_STORE2: {
            // store 2-byte value from opstack[0] into address stored in opstack[1] 
            uint16_t* dst = (uint16_t*)(qvm->datasegment + (opstack[1] & datamask));
            *dst = (uint16_t)(opstack[0] & 0xFFFF);
            QVM_POPN(2);
            break;
        }

        case QVM_OP_STORE4: {
            // store 4-byte value from opstack[0] into address stored in opstack[1]
            int* dst = (int*)(qvm->datasegment + (opstack[1] & datamask));
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

            uint8_t* src = qvm->datasegment + srci;
            uint8_t* dst = qvm->datasegment + dsti;

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
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", instruction, opptr - qvm->codesegment, opcodename[op]);
                goto fail;
            }
            QVM_SOP( /= );
            break;

        case QVM_OP_DIVU:
            // unsigned division
            if (opstack[0] == 0) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", instruction, opptr - qvm->codesegment, opcodename[op]);
                goto fail;
            }
            QVM_UOP( /= );
            break;

        case QVM_OP_MODI:
            // modulus
            if (opstack[0] == 0) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", instruction, opptr - qvm->codesegment, opcodename[op]);
                goto fail;
            }
            QVM_SOP( %= );
            break;

        case QVM_OP_MODU:
            // unsigned modulus
            if (opstack[0] == 0) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", instruction, opptr - qvm->codesegment, opcodename[op]);
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
            // float 0s are all 0 bits but with either sign bit
            if (opstack[0] == 0 || opstack[0] == 0x80000000) {
                log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error at %td: %s division by 0!\n", opptr - qvm->codesegment, opcodename[op]);
                goto fail;
            }
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

    // compare stored frame size like in QVM_OP_LEAVE
    if (programstack[1] != framesize) {
        log_c(QMM_LOG_FATAL, QMM_LOGGING_TAG, "qvm_exec(%zu): Runtime error after execution: stack frame size (%d) does not match entry stack frame size (%d)\n", opptr - qvm->codesegment, programstack[1], framesize);
        goto fail;
    }

    // remove initial stack frame like in QVM_OP_LEAVE
    QVM_STACKFRAME(-framesize);

    // save our local program stack pointer back into the qvm object
    qvm->stackptr = programstack;

    // return value is stored on the top of the opstack (pushed just before QVM_OP_LEAVE)
    return opstack[0];

fail:
    qvm_unload(qvm);
    return 0;
}


// return a string name for the VM opcode
const char* opcodename[] = {
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


qvm_alloc_t qvm_allocator_default = { qvm_alloc_default, qvm_free_default, NULL };
