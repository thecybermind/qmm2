/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <stdint.h>
#include <string.h>
#include <vector>
#include "log.h"
#include "format.h"
#include "qvm.h"


static bool qvm_validate_ptr(qvm_t& qvm, void* ptr, void* start = nullptr, void* end = nullptr);


bool qvm_load(qvm_t& qvm, const uint8_t* filemem, unsigned int filesize, vmsyscall_t vmsyscall, unsigned int stacksize, bool verify_data) {
    if (!qvm.memory.empty() || !filemem || !filesize || !vmsyscall)
        return false;

    const uint8_t* codeoffset = nullptr;

    if (filesize < sizeof(qvmheader_t)) {
        LOG(QMM_LOG_ERROR, "QMM") << "qvm_load(): Invalid QVM file: too small for header\n";
        goto fail;
    }
    
    qvm.filesize = filesize;
    qvm.vmsyscall = vmsyscall;
    qvm.verify_data = verify_data;

    // grab a copy of the header
    memcpy(&qvm.header, filemem, sizeof(qvmheader_t));

    // check header
    if (qvm.header.magic != QVM_MAGIC ||
        qvm.header.numops <= 0 ||
        qvm.header.codelen <= 0 ||
        qvm.filesize != (sizeof(qvm.header) + qvm.header.codelen + qvm.header.datalen + qvm.header.litlen) ||
        qvm.header.codeoffset < sizeof(qvm.header) ||
        qvm.header.dataoffset < sizeof(qvm.header) ||
        qvm.header.codeoffset > qvm.filesize ||
        qvm.header.dataoffset > qvm.filesize
        ) {
        LOG(QMM_LOG_ERROR, "QMM") << "qvm_load(): Invalid QVM file\n";
        goto fail;
    }

    // each opcode is 8 bytes long, calculate total size of opcodes
    qvm.codeseglen = qvm.header.numops * sizeof(qvmop_t);
    // just add each data segment up
    qvm.dataseglen = qvm.header.datalen + qvm.header.litlen + qvm.header.bsslen;
    // calculate stack size from config option in MiB 
    if (!stacksize)
        stacksize = 1;
    qvm.stackseglen = stacksize * (1 << 20);

    // allocate vm memory
    qvm.memory.resize(qvm.codeseglen + qvm.dataseglen + qvm.stackseglen);

    // set pointers
    qvm.codesegment = (qvmop_t*)qvm.memory.data();
    qvm.datasegment = qvm.memory.data() + qvm.codeseglen;
    qvm.stacksegment = qvm.datasegment + qvm.dataseglen;

    // setup registers
    // op is the code pointer, simple enough
    qvm.opptr = NULL;
    // stack is for general operations. it starts at end of stack segment and grows down
    qvm.stackptr = (int*)(qvm.stacksegment + qvm.stackseglen);
    // argstack is for arguments and local variables. it starts halfway through the stack segment and grows down
    qvm.argbase = qvm.dataseglen + qvm.stackseglen / 2;
    // NOTE: memory segments are laid out like this:
    // | CODE | DATA | STACK |
    // OP_LOCAL stores argbase+param into *stack
    // OP_LOADx loads address at datasegment+*stack
    // this means that the argstack needs to be located just after the data segment, with the stack to follow

    // start loading ops from the code offset to VM
    codeoffset = filemem + qvm.header.codeoffset;

    // loop through each op
    for (unsigned int i = 0; i < qvm.header.numops; ++i) {
        // get the opcode
        qvmopcode_t opcode = (qvmopcode_t)*codeoffset;

        codeoffset++;

        // write opcode (to qvmop_t)
        qvm.codesegment[i].op = opcode;

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
            if (*(unsigned int*)codeoffset > qvm.header.numops) {
                LOG(QMM_LOG_ERROR, "QMM") << fmt::format("qvm_load(): Invalid target in jump/branch instruction: {} > {}\n", *(int*)codeoffset, qvm.header.numops);
                goto fail;
            }
            SWITCH_FALLTHROUGH;	// MSVC C26819: Unannotated fallthrough between switch labels
        case OP_ENTER:
        case OP_LEAVE:
        case OP_CONST:
        case OP_LOCAL:
        case OP_BLOCK_COPY:
            // these ops all have full 4-byte params
            qvm.codesegment[i].param = *(int*)codeoffset;
            codeoffset += 4;
            break;

        case OP_ARG:
            // this op has a 1-byte param
            qvm.codesegment[i].param = (int)*codeoffset;
            codeoffset++;
            break;

        default:
            // remaining ops require no 'param'
            qvm.codesegment[i].param = 0;
            break;
        }
    }

    // copy data segment (including literals) to VM
    memcpy(qvm.datasegment, filemem + qvm.header.dataoffset, qvm.header.datalen + qvm.header.litlen);

    // a winner is us
    return true;

fail:
    qvm_unload(qvm);
    return false;
}


void qvm_unload(qvm_t& qvm) {
    qvm = qvm_t();
}


int qvm_exec(qvm_t& qvm, int argc, int* argv) {
    if (qvm.memory.empty())
        return 0;

    // grow arg stack for new arguments
    qvm.argbase -= (argc + 2) * sizeof(int);

    // args points to top of argstack
    int* args = (int*)(qvm.datasegment + qvm.argbase);

    // push args into the new argstack space
    // store the current code offset
    args[0] = (int)(qvm.opptr - qvm.codesegment);
    args[1] = 0;	// blank for now
    // move qvm_exec arguments onto arg stack starting at args[2]
    if (argv && argc > 0)
        memcpy(&args[2], argv, argc * sizeof(int));

    // store code for easier access
    int vmMain_code = args[2];

    // vmMain's OP_ENTER will grab this return address and store it on the argstack.
    // when it is added to this->codesegment in vmMain's OP_LEAVE, it will result in
    // opptr being NULL, terminating the execution loop
    --qvm.stackptr;
    qvm.stackptr[0] = (int)((qvmop_t*)NULL - qvm.codesegment);

    // start at beginning of code segment
    qvmop_t* opptr = qvm.codesegment;
    // get current stack pointer
    int* stack = qvm.stackptr;

    qvmopcode_t op;
    int param;

    do {
        // verify code pointer is in code segment. this is likely malicious?
        if (!qvm_validate_ptr(qvm, opptr, qvm.codesegment, qvm.codesegment + qvm.codeseglen)) {
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) Execution outside the VM code segment: {}\n", vmMain_code, (void*)opptr);
            goto fail;
        }
        // verify stack pointer is in top half of stack segment. this could be malicious, or an accidental stack overflow
        if (!qvm_validate_ptr(qvm, stack, qvm.stacksegment + (qvm.stackseglen / 2), qvm.stacksegment + qvm.stackseglen + 1)) {
            intptr_t stacksize = qvm.stacksegment + qvm.stackseglen - (uint8_t*)stack;
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) Stack overflow! Stack size is currently {}, max is {}. You may need to increase the \"stacksize\" config option.\n", vmMain_code, stacksize, qvm.stackseglen / 2);
            goto fail;
        }
        // verify argstack pointer is in bottom half of stack segment. this could be malicious, or an accidental stack overflow
        if (!qvm_validate_ptr(qvm, qvm.datasegment + qvm.argbase, qvm.stacksegment, qvm.stacksegment + (qvm.stackseglen / 2) + 1)) {
            intptr_t argstacksize = qvm.stacksegment + (qvm.stackseglen / 2) - (qvm.datasegment + qvm.argbase);
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) Arg stack overflow! Arg stack size is currently {}, max is {}. You may need to increase the \"stacksize\" config option.\n", vmMain_code, argstacksize, qvm.stackseglen / 2);
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

        case OP_BREAK:
            // break to debugger?
            // todo: dump stacks/memory?
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) Unhandled opcode {}\n", vmMain_code, opcodename[op]);
            goto fail;

        default:
            // anything else
            LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) Unhandled opcode {}\n", vmMain_code, (int)op);
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
            *stack = qvm.argbase + param;
            break;

        case OP_ARG: {
            // set a function-call arg (offset = param) to the value on top of stack
            int* dst = (int*)(qvm.datasegment + qvm.argbase + param);
            *dst = *stack;
            ++stack;
            break;
        }

        // functions / code flow
#define JUMP(x) opptr = qvm.codesegment + (x)

        case OP_CALL: {
            // call a function
            // top of the stack is the current instruction index in number of ops from start of code segment
            int jmp_to = *stack;

            // negative means an engine trap
            if (jmp_to < 0) {
                // save local registers for recursive execution
                qvm.stackptr = stack;
                qvm.opptr = opptr;

                // pass call to game-specific syscall handler which will adjust pointer arguments
                // and then call the normal QMM syscall entry point so it can be routed to plugins
                int ret = qvm.vmsyscall(qvm.datasegment, -jmp_to - 1, (int*)(qvm.datasegment + qvm.argbase) + 2);

                // restore local registers
                stack = qvm.stackptr;
                opptr = qvm.opptr;

                // return value on top of stack
                *stack = ret;
                break;
            }

            // replace top of stack with the current instruction index (number of ops from start of code segment)
            *stack = (int)(opptr - qvm.codesegment);
            // jump to VM function at address
            JUMP(jmp_to);
            break;
        }

        case OP_ENTER: {
            // enter a function, prepare local variable argstack space (length=param).
            // store the instruction return index (at top of stack from OP_CALL) in argstack and
            // store the param in the next argstack slot. this gets verified to match in OP_LEAVE
            qvm.argbase -= param;
            int* arg = (int*)(qvm.datasegment + qvm.argbase);
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
            int* arg = (int*)(qvm.datasegment + qvm.argbase);
            opptr = qvm.codesegment + *arg;
            // compare param with the OP_ENTER param stored on the argstack
            if (arg[1] != param) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) OP_LEAVE param ({}) does not match OP_ENTER param ({})\n", vmMain_code, param, arg[1]);
                goto fail;
            }
            // offset argstack to cleanup frame
            qvm.argbase += param;

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
            SIF(== );
            break;

        case OP_NE:
            // if stack[1] != stack[0], goto address in param
            SIF(!= );
            break;

        case OP_LTI:
            // if stack[1] < stack[0], goto address in param
            SIF(< );
            break;

        case OP_LEI:
            // if stack[1] <= stack[0], goto address in param
            SIF(<= );
            break;

        case OP_GTI:
            // if stack[1] > stack[0], goto address in param
            SIF(> );
            break;

        case OP_GEI:
            // if stack[1] >= stack[0], goto address in param
            SIF(>= );
            break;

        case OP_LTU:
            // if stack[1] < stack[0], goto address in param (unsigned)
            UIF(< );
            break;

        case OP_LEU:
            // if stack[1] <= stack[0], goto address in param (unsigned)
            UIF(<= );
            break;

        case OP_GTU:
            // if stack[1] > stack[0], goto address in param (unsigned)
            UIF(> );
            break;

        case OP_GEU:
            // if stack[1] >= stack[0], goto address in param (unsigned)
            UIF(>= );
            break;

        case OP_EQF:
            // if stack[1] == stack[0], goto address in param (float)
            FIF(== );
            break;

        case OP_NEF:
            // if stack[1] != stack[0], goto address in param (float)
            FIF(!= );
            break;

        case OP_LTF:
            // if stack[1] < stack[0], goto address in param (float)
            FIF(< );
            break;

        case OP_LEF:
            // if stack[1] <= stack[0], goto address in param (float)
            FIF(<= );
            break;

        case OP_GTF:
            // if stack[1] > stack[0], goto address in param (float)
            FIF(> );
            break;

        case OP_GEF:
            // if stack[1] >= stack[0], goto address in param (float)
            FIF(>= );
            break;

        // memory/pointer management

        case OP_STORE1: {
            // store 1-byte value from stack[0] into address stored in stack[1]
            uint8_t* dst = qvm.datasegment + stack[1];
            if (qvm.verify_data && !qvm_validate_ptr(qvm, dst)) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)dst);
                goto fail;
            }
            *dst = (uint8_t)(*stack & 0xFF);
            stack += 2;
            break;
        }

        case OP_STORE2: {
            // 2-byte
            unsigned short* dst = (unsigned short*)(qvm.datasegment + stack[1]);
            if (qvm.verify_data && !qvm_validate_ptr(qvm, dst)) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)dst);
                goto fail;
            }
            *dst = (unsigned short)(*stack & 0xFFFF);
            stack += 2;
            break;
        }

        case OP_STORE4: {
            // 4-byte
            int* dst = (int*)(qvm.datasegment + stack[1]);
            if (qvm.verify_data && !qvm_validate_ptr(qvm, dst)) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)dst);
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
            uint8_t* src = qvm.datasegment + *stack;
            if (qvm.verify_data && !qvm_validate_ptr(qvm, src)) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)src);
                goto fail;
            }
            *stack = (int)*src;
            break;
        }

        case OP_LOAD2: {
            // 2-byte
            unsigned short* src = (unsigned short*)(qvm.datasegment + *stack);
            if (qvm.verify_data && !qvm_validate_ptr(qvm, src)) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)src);
                goto fail;
            }
            *stack = (int)*src;
            break;
        }

        case OP_LOAD4: {
            // 4-byte
            int* src = (int*)(qvm.datasegment + *stack);
            if (qvm.verify_data && !qvm_validate_ptr(qvm, src)) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)src);
                goto fail;
            }
            *stack = *src;
            break;
        }

        case OP_BLOCK_COPY: {
            // copy mem at address pointed to by stack[0] to address pointed to by stack[1]
            // for 'param' number of bytes
            uint8_t* src = qvm.datasegment + *stack++;
            uint8_t* dst = qvm.datasegment + *stack++;

            // skip if src/dst are the same
            if (src == dst)
                break;

            // check if src block goes out of VM range
            if (qvm.verify_data && (!qvm_validate_ptr(qvm, src) || !qvm_validate_ptr(qvm, src + param - 1))) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} source pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)src);
                goto fail;
            }
            // check if dst block goes out of VM range
            if (qvm.verify_data && (!qvm_validate_ptr(qvm, dst) || !qvm_validate_ptr(qvm, dst + param - 1))) {
                LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} destination pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)dst);
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
            SSOP(-);
            break;

        case OP_ADD:
            // addition
            SOP(+= );
            break;

        case OP_SUB:
            // subtraction
            SOP(-= );
            break;

        case OP_MULI:
            // multiplication
            SOP(*= );
            break;

        case OP_MULU:
            // unsigned multiplication
            UOP(*= );
            break;

        case OP_DIVI:
            // division
            SOP(/= );
            break;

        case OP_DIVU:
            // unsigned division
            UOP(/= );
            break;

        case OP_MODI:
            // modulus
            SOP(%= );
            break;

        case OP_MODU:
            // unsigned modulus
            UOP(%= );
            break;

        case OP_BAND:
            // bitwise AND
            SOP(&= );
            break;

        case OP_BOR:
            // bitwise OR
            SOP(|= );
            break;

        case OP_BXOR:
            // bitwise XOR
            SOP(^= );
            break;

        case OP_BCOM:
            // bitwise one's compliment
            SSOP(~);
            break;

        case OP_LSH:
            // unsigned bitwise LEFTSHIFT
            UOP(<<= );
            break;

        case OP_RSHI:
            // bitwise RIGHTSHIFT
            SOP(>>= );
            break;

        case OP_RSHU:
            // unsigned bitwise RIGHTSHIFT
            UOP(>>= );
            break;

        case OP_NEGF:
            // float negation
            SFOP(-);
            break;

        case OP_ADDF:
            // float addition
            FOP(+= );
            break;

        case OP_SUBF:
            // float subtraction
            FOP(-= );
            break;

        case OP_MULF:
            // float multiplication
            FOP(*= );
            break;

        case OP_DIVF:
            // float division
            FOP(/= );
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

    // restore previous code pointer
    qvm.opptr = qvm.codesegment + args[0];

    // update persistent stack pointer with the current one
    qvm.stackptr = stack;

    // remove arguments from argstack like in "OP_LEAVE"
    qvm.argbase += (argc + 2) * sizeof(int);

    // return value is stored on the top of the stack (pushed just before OP_LEAVE)
    return *qvm.stackptr++;

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


static bool qvm_validate_ptr(qvm_t& qvm, void* ptr, void* start, void* end) {
    if (qvm.memory.empty())
        return false;

    // default to validating ptr is inside the data + stack segments
    start = start ? start : qvm.datasegment;
    end = end ? end : (qvm.memory.data() + qvm.memory.size());

    return (ptr >= start && ptr < end);
}
