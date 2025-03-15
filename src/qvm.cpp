/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < cybermind@gmail.com >

*/

#include <string.h>
#include <stdlib.h>
#include "qvm.h"

static bool qvm_validate_ptr(qvm_t* qvm, void* ptr, void* start = nullptr, void* end = nullptr) {
	start = start ? start : qvm->memory;
	end = end ? end : (qvm->memory + qvm->memorysize);

	return (ptr >= start || ptr < end);
}

bool qvm_load(qvm_t* qvm, byte* filemem, int filelen, vmsyscall_t vmsyscall, int stacksize) {
	if (!qvm || !filemem || !filelen || qvm->memory || !vmsyscall)
		return false;

	byte* codeoffset = nullptr;

	qvm->vmsyscall = vmsyscall;
	qvm->filesize = filelen;

	// grab a copy of the header
	memcpy(&qvm->header, filemem, sizeof(qvmheader_t));

	// check header
	if (qvm->header.magic != QVM_MAGIC ||
		qvm->header.numops <= 0 ||
		qvm->header.codelen <= 0 ||
		(unsigned int)qvm->filesize != (sizeof(qvm->header) + qvm->header.codelen + qvm->header.datalen + qvm->header.litlen) ||
		qvm->header.codeoffset < sizeof(qvm->header) ||
		qvm->header.dataoffset < sizeof(qvm->header) ||
		qvm->header.codeoffset > qvm->filesize ||
		qvm->header.dataoffset > qvm->filesize
		) {
		goto fail;
	}

	// each opcode is 8 bytes long, calculate total size of opcodes
	qvm->codeseglen = qvm->header.numops * sizeof(qvmop_t);
	// just add each data segment up
	qvm->dataseglen = qvm->header.datalen + qvm->header.litlen + qvm->header.bsslen;
	if (!stacksize)
		stacksize = 1;
	qvm->stackseglen = stacksize * (1 << 20);

	// get total memory size
	qvm->memorysize = qvm->codeseglen + qvm->dataseglen + qvm->stackseglen;

	// allocate vm memory
	if (!(qvm->memory = (byte*)malloc(qvm->memorysize))) {
		goto fail;
	}
	// init the memory
	memset(qvm->memory, 0, qvm->memorysize);

	// set pointers
	qvm->codesegment = (qvmop_t*)qvm->memory;
	qvm->datasegment = (byte*)(qvm->memory + qvm->codeseglen);
	qvm->stacksegment = (byte*)(qvm->datasegment + qvm->dataseglen);

	// setup registers
	// op is the code pointer, simple enough
	qvm->opptr = NULL;
	// stack is for general operations. it starts at end of stack segment and grows down (-4 per push)
	qvm->stackptr = (int*)(qvm->stacksegment + qvm->stackseglen);
	// argstack is for arguments and local variables. it starts halfway through the stack segment and grows down
	qvm->argbase = qvm->dataseglen + qvm->stackseglen / 2;
	// NOTE: memory segments are laid out like this:
	// | CODE | DATA | STACK |
	// OP_LOCAL stores argbase+param into *stack
	// OP_LOADx loads address at datasegment+*stack
	// this means that the argstack needs to be located just after the data segment, with the stack to follow

	// start loading ops from the code offset to VM
	codeoffset = filemem + qvm->header.codeoffset;

	// loop through each op
	for (int i = 0; i < qvm->header.numops; ++i) {
		// get the opcode
		byte opcode = *codeoffset;

		codeoffset++;

		// write opcode (to qvmop_t)
		qvm->codesegment[i].op = (qvmopcode_t)opcode;

		switch (opcode) {
			// these ops all have full 4-byte 'param's, which may need to be byteswapped
			case OP_ENTER:
			case OP_LEAVE:
			case OP_CONST:
			case OP_LOCAL:
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
			case OP_BLOCK_COPY:
				qvm->codesegment[i].param = *(int*)codeoffset;
				codeoffset += 4;
				break;
			// this op has a 1-byte 'param'
			case OP_ARG:
				qvm->codesegment[i].param = (int)*codeoffset;
				codeoffset++;
				break;
				// remaining ops require no 'param'
			default:
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

void qvm_unload(qvm_t* qvm) {
	if (qvm) {
		free(qvm->memory);
		*qvm = qvm_t();
	}
}

int qvm_exec(qvm_t* qvm, int* argv, int argc) {
	if (!qvm || !qvm->memory)
		return 0;

	// prepare local stack
	qvm->argbase -= (argc + 2) * sizeof(int);

	int* args = (int*)(qvm->datasegment + qvm->argbase);

	// push args into the new arg heap space

	args[0] = 0;	// blank for now
	// store the current code offset
	args[1] = (qvm->opptr - qvm->codesegment);
	// move arguments on arg stack starting at args[2]
	if (argv && argc > 0)
		memcpy(&args[2], argv, argc * sizeof(int));

	--qvm->stackptr;

	// vmMain's OP_ENTER will grab this and store it at the bottom of the arg stack.
	// when it is added to this->codesegment in the final OP_LEAVE, it will result in
	// opptr being NULL, terminating the execution loop
	qvm->stackptr[0] = (qvmop_t*)NULL - qvm->codesegment;

	// move opptr to start of opcodes
	qvm->opptr = qvm->codesegment;

	// local versions of registers
	qvmop_t* opptr = qvm->opptr;
	int* stack = qvm->stackptr;

	qvmopcode_t op;
	int param;

	do {
		// if code pointer is outside the code segment, something really bad happened
		if (!qvm_validate_ptr(qvm, opptr, qvm->codesegment, qvm->codesegment + qvm->codeseglen)) {
			qvm_unload(qvm);
			return INT_MIN;
		}
		op = (qvmopcode_t)opptr->op;
		param = opptr->param;

		++opptr;

		switch (op) {
			// miscellaneous opcodes

			// no op - don't raise error
			case OP_NOP:
				break;

			// undefined
			case OP_UNDEF:

			// break to debugger?
			case OP_BREAK:

			// anything else
			default:
				// todo
				//ENG_SYSCALL(QMM_ENG_MSG[QMM_G_PRINT], vaf("[QMM] ERROR: CVMMod::vmMain(%s): Unhandled opcode %s (%d)\n", g_gameinfo.game->eng_msg_names(cmd), opcodename[op], op));
				//log_write(vaf("[QMM] ERROR: CVMMod::vmMain(%s): Unhandled opcode %s (%d)\n", g_gameinfo.game->eng_msg_names(cmd), opcodename[op], op));
				break;

			// stack opcodes

			// pushes a blank value onto the end of the stack
			case OP_PUSH:
				--stack;
				*stack = 0;
				break;
			// pops the last value off the end of the stack
			case OP_POP:
				++stack;
				break;
			// pushes a specified value onto the end of the stack
			case OP_CONST:
				--stack;
				*stack = param;
				break;
			// pushes a specified local variable address onto the stack
			case OP_LOCAL:
				--stack;
				*stack = qvm->argbase + param;
				break;
			// set a function-call arg (offset = param) to the value in stack[0]
			case OP_ARG: {
				int* dst = (int*)(qvm->datasegment + qvm->argbase + param);
				if (qvm_validate_ptr(qvm, dst))
					*dst = *stack;
				++stack;
				break;
			}

			// functions / code flow
			#define JUMP(x) opptr = qvm->codesegment + (x)

			// call a function
			case OP_CALL:
				param = *stack;

				// param (really stack[0]) is the function address in number of ops
				// negative means an engine trap
				if (param < 0) {
					// save local registers for recursive execution
					qvm->stackptr = stack;
					qvm->opptr = opptr;

					int ret = qvm->vmsyscall(qvm->datasegment, -param - 1, (int*)(qvm->datasegment + qvm->argbase) + 2);

					// restore local registers
					stack = qvm->stackptr;
					opptr = qvm->opptr;

					*stack = ret;
					break;
				}

				// replace func id (in the stack) with code address to resume at
				// changed to the actual pointer rather than code segment offset
				*stack = opptr - qvm->codesegment;
				// jump to VM function at address
				JUMP(param);
				break;
			// enter a function, prepare local var heap (length=param)
			// store the return address (front of stack) in arg heap
			case OP_ENTER: {
				qvm->argbase -= param;
				int* arg = (int*)(qvm->datasegment + qvm->argbase);
				if (qvm_validate_ptr(qvm, arg))
					*arg = 0;
				if (qvm_validate_ptr(qvm, arg + 1))
					arg[1] = *stack;
				stack++;
				break;
			}
			// leave a function, move opcode pointer to previous function
			case OP_LEAVE:
				// retrieve the return code address from bottom of the arg heap
				opptr = qvm->codesegment + *((int*)(qvm->datasegment + qvm->argbase) + 1);

				// offset arg heap by same as in OP_ENTER
				qvm->argbase += param;

				break;

			// branching

			// signed integer comparison
			#define SIF(o) if (stack[1] o *stack) JUMP(param); stack += 2
			// unsigned integer comparison
			#define UIF(o) if (*(unsigned int*)&stack[1] o *(unsigned int*)stack) JUMP(param); stack += 2
			// floating point comparison
			#define FIF(o) if (*(float*)&stack[1] o *(float*)stack) JUMP(param); stack += 2

			// jump to address in stack[0]
			case OP_JUMP:
				JUMP(*stack++);
				break;
			// if stack[1] == stack[0], goto address in param
			case OP_EQ:
				SIF(== );
				break;
			// if stack[1] != stack[0], goto address in param
			case OP_NE:
				SIF(!= );
				break;
			// if stack[1] < stack[0], goto address in param
			case OP_LTI:
				SIF(< );
				break;
			// if stack[1] <= stack[0], goto address in param
			case OP_LEI:
				SIF(<= );
				break;
			// if stack[1] > stack[0], goto address in param
			case OP_GTI:
				SIF(> );
				break;
			// if stack[1] >= stack[0], goto address in param
			case OP_GEI:
				SIF(>= );
				break;
			// if stack[1] < stack[0], goto address in param (unsigned)
			case OP_LTU:
				UIF(< );
				break;
			// if stack[1] <= stack[0], goto address in param (unsigned)
			case OP_LEU:
				UIF(<= );
				break;
			// if stack[1] > stack[0], goto address in param (unsigned)
			case OP_GTU:
				UIF(> );
				break;
			// if stack[1] >= stack[0], goto address in param (unsigned)
			case OP_GEU:
				UIF(>= );
				break;
			// if stack[1] == stack[0], goto address in param (float)
			case OP_EQF:
				FIF(== );
				break;
			// if stack[1] != stack[0], goto address in param (float)
			case OP_NEF:
				FIF(!= );
				break;
			// if stack[1] < stack[0], goto address in param (float)
			case OP_LTF:
				FIF(< );
				break;
			// if stack[1] <= stack[0], goto address in param (float)
			case OP_LEF:
				FIF(<= );
				break;
			// if stack[1] > stack[0], goto address in param (float)
			case OP_GTF:
				FIF(> );
				break;
			// if stack[1] >= stack[0], goto address in param (float)
			case OP_GEF:
				FIF(>= );
				break;

			// memory/pointer management

			// store 1-byte value from stack[0] into address stored in stack[1]
			case OP_STORE1: {
				byte* dst1 = qvm->datasegment + stack[1];
				if (qvm_validate_ptr(qvm, dst1))
					*dst1 = (byte)(*stack & 0xFF);
				stack += 2;
				break;
			}
			// 2-byte
			case OP_STORE2: {
				unsigned short* dst = (unsigned short*)(qvm->datasegment + stack[1]);
				if (qvm_validate_ptr(qvm, dst))
					*dst = (unsigned short)(*stack & 0xFFFF);
				stack += 2;
				break;
			}
			// 4-byte
			case OP_STORE4: {
				int* dst = (int*)(qvm->datasegment + stack[1]);
				if (qvm_validate_ptr(qvm, dst))
					*dst = *stack;
				stack += 2;
				break;
			}
			// get 1-byte value at address stored in stack[0],
			// and store back in stack[0]
			// 1-byte
			case OP_LOAD1: {
				byte* src = qvm->datasegment + *stack;
				if (qvm_validate_ptr(qvm, src))
					*stack = *src;
				break;
			}
			// 2-byte
			case OP_LOAD2: {
				unsigned short* src = (unsigned short*)(qvm->datasegment + *stack);
				if (qvm_validate_ptr(qvm, src))
					*stack = *src;
				break;
			}
			// 4-byte
			case OP_LOAD4: {
				int* src = (int*)(qvm->datasegment + *stack);
				if (qvm_validate_ptr(qvm, src))
					*stack = *src;
				break;
			}
			// copy mem at address pointed to by stack[0] to address pointed to by stack[1]
			// for 'param' number of bytes
			case OP_BLOCK_COPY: {
				byte* src = qvm->datasegment + *stack++;
				byte* dst = qvm->datasegment + *stack++;

				// skip if src/dst are the same
				if (src == dst)
					break;
				// skip if src block goes out of VM range
				if (!qvm_validate_ptr(qvm, src) || !qvm_validate_ptr(qvm, src+param-1))
					break;
				// skip if dst block goes out of VM range
				if (!qvm_validate_ptr(qvm, dst) || !qvm_validate_ptr(qvm, dst+param-1))
					break;
				
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

			// negation
			case OP_NEGI:
				SSOP(-);
				break;
			// addition
			case OP_ADD:
				SOP(+= );
				break;
			// subtraction
			case OP_SUB:
				SOP(-= );
				break;
			// multiplication
			case OP_MULI:
				SOP(*= );
				break;
			// unsigned multiplication
			case OP_MULU:
				UOP(*= );
				break;
			// division
			case OP_DIVI:
				SOP(/= );
				break;
			// unsigned division
			case OP_DIVU:
				UOP(/= );
				break;
			// modulation
			case OP_MODI:
				SOP(%= );
				break;
			// unsigned modulation
			case OP_MODU:
				UOP(%= );
				break;
			// bitwise AND
			case OP_BAND:
				SOP(&= );
				break;
			// bitwise OR
			case OP_BOR:
				SOP(|= );
				break;
			// bitwise XOR
			case OP_BXOR:
				SOP(^= );
				break;
			// bitwise one's compliment
			case OP_BCOM:
				SSOP(~);
				break;
			// unsigned bitwise LEFTSHIFT
			case OP_LSH:
				UOP(<<= );
				break;
			// bitwise RIGHTSHIFT
			case OP_RSHI:
				SOP(>>= );
				break;
			// unsigned bitwise RIGHTSHIFT
			case OP_RSHU:
				UOP(>>= );
				break;
			// float negation
			case OP_NEGF:
				SFOP(-);
				break;
			// float addition
			case OP_ADDF:
				FOP(+= );
				break;
			// float subtraction
			case OP_SUBF:
				FOP(-= );
				break;
			// float multiplication
			case OP_MULF:
				FOP(*= );
				break;
			// float division
			case OP_DIVF:
				FOP(/= );
				break;

			// sign extensions

			// 8-bit
			case OP_SEX8:
				if (*stack & 0x80)
					*stack |= 0xFFFFFF00;
				break;
			// 16-bit
			case OP_SEX16:
				if (*stack & 0x8000)
					*stack |= 0xFFFF0000;
				break;

			// format conversion

			// convert stack[0] int->float
			case OP_CVIF:
				*(float*)stack = (float)*stack;
				break;
			// convert stack[0] float->int
			case OP_CVFI:
				*stack = (int)*(float*)stack;
				break;

		} // op switch
	} while (opptr);

	// restore previous code pointer as well as the arg heap
	qvm->opptr = qvm->codesegment + args[1];
	qvm->argbase += (argc + 2) * sizeof(int);
	qvm->stackptr = stack;

	// return value is stored on the top of the stack (pushed just before OP_LEAVE)
	return *qvm->stackptr++;
}
