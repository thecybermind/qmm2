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
#include "log.h"
#include "format.h"
#include "qvm.h"

static bool qvm_validate_ptr(qvm_t& qvm, void* ptr, void* start = nullptr, void* end = nullptr);


bool qvm_load(qvm_t& qvm, const std::vector<std::byte>& filemem, vmsyscall_t vmsyscall, unsigned int stacksize, bool verify_data) {
	if (!qvm.memory.empty() || filemem.empty() || !vmsyscall)
		return false;

	const std::byte* codeoffset = nullptr;

	qvm.filesize = filemem.size();
	qvm.vmsyscall = vmsyscall;
	qvm.verify_data = verify_data;

	// grab a copy of the header
	memcpy(&qvm.header, filemem.data(), sizeof(qvmheader_t));

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
	codeoffset = filemem.data() + qvm.header.codeoffset;

	// loop through each op
	for (unsigned int i = 0; i < qvm.header.numops; ++i) {
		// get the opcode
		qvmopcode_t opcode = (qvmopcode_t)*codeoffset;

		codeoffset++;

		// write opcode (to qvmop_t)
		qvm.codesegment[i].op = opcode;

		switch (opcode) {
			// these ops all have full 4-byte params
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
				// these ops all jump to an instruction, just a sanity check to make sure it's within range
				if (*(unsigned int*)codeoffset > qvm.header.numops) {
					LOG(QMM_LOG_ERROR, "QMM") << fmt::format("qvm_load(): Invalid target in jump/branch instruction: {} > {}\n", *(int*)codeoffset, qvm.header.numops);
					goto fail;
				}
				#ifdef _WIN32
				[[fallthrough]];	// MSVC C26819: Unannotated fallthrough between switch labels
				#endif
			case OP_ENTER:
			case OP_LEAVE:
			case OP_CONST:
			case OP_LOCAL:
			case OP_BLOCK_COPY:
				qvm.codesegment[i].param = *(int*)codeoffset;
				codeoffset += 4;
				break;
			// this op has a 1-byte param
			case OP_ARG:
				qvm.codesegment[i].param = (int)*codeoffset;
				codeoffset++;
				break;
			// remaining ops require no 'param'
			default:
				qvm.codesegment[i].param = 0;
				break;
		}
	}

	// copy data segment (including literals) to VM
	memcpy(qvm.datasegment, filemem.data() + qvm.header.dataoffset, qvm.header.datalen + qvm.header.litlen);

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
			intptr_t stacksize = qvm.stacksegment + qvm.stackseglen - (std::byte*)stack;
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

			// no op - don't raise error
			case OP_NOP:
				break;

			// undefined
			case OP_UNDEF:

			// break to debugger?
			case OP_BREAK:
				// todo: dump stacks/memory?

			// anything else
			default:
				LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) Unhandled opcode {}\n", vmMain_code, (int)op);
				goto fail;

			// stack opcodes

			// pushes a blank value onto the stack
			case OP_PUSH:
				--stack;
				*stack = 0;
				break;
			// pops the top value off the stack
			case OP_POP:
				++stack;
				break;
			// pushes a hardcoded value onto the stack
			case OP_CONST:
				--stack;
				*stack = param;
				break;
			// pushes a specified local variable address (relative to start of data segment) onto the stack
			case OP_LOCAL:
				--stack;
				*stack = qvm.argbase + param;
				break;
			// set a function-call arg (offset = param) to the value on top of stack
			case OP_ARG: {
				int* dst = (int*)(qvm.datasegment + qvm.argbase + param);
				*dst = *stack;
				++stack;
				break;
			}

			// functions / code flow
			#define JUMP(x) opptr = qvm.codesegment + (x)

			// call a function
			case OP_CALL: {
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
			// enter a function, prepare local variable argstack space (length=param).
			// store the instruction return index (at top of stack from OP_CALL) in argstack and
			// store the param in the next argstack slot. this gets verified to match in OP_LEAVE
			case OP_ENTER: {
				qvm.argbase -= param;
				int* arg = (int*)(qvm.datasegment + qvm.argbase);
				arg[0] = *stack;
				arg[1] = param;
				stack++;
				break;
			}
			// leave a function
			// get previous instruction index from argstack[0] and OP_ENTER param from
			// argstack[1]. verify argstack[1] matches param, and then clean up argstack frame
			case OP_LEAVE: {
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
				std::byte* dst = qvm.datasegment + stack[1];
				if (qvm.verify_data && !qvm_validate_ptr(qvm, dst)) {
					LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)dst);
					goto fail;
				}
				*dst = (std::byte)(*stack & 0xFF);
				stack += 2;
				break;
			}
			// 2-byte
			case OP_STORE2: {
				unsigned short* dst = (unsigned short*)(qvm.datasegment + stack[1]);
				if (qvm.verify_data && !qvm_validate_ptr(qvm, dst)) {
					LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)dst);
					goto fail;
				}
				*dst = (unsigned short)(*stack & 0xFFFF);
				stack += 2;
				break;
			}
			// 4-byte
			case OP_STORE4: {
				int* dst = (int*)(qvm.datasegment + stack[1]);
				if (qvm.verify_data && !qvm_validate_ptr(qvm, dst)) {
					LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)dst);
					goto fail;
				}
				*dst = *stack;
				stack += 2;
				break;
			}
			// get 1-byte value at address stored in stack[0],
			// and store back in stack[0]
			// 1-byte
			case OP_LOAD1: {
				std::byte* src = qvm.datasegment + *stack;
				if (qvm.verify_data && !qvm_validate_ptr(qvm, src)) {
					LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)src);
					goto fail;
				}
				*stack = (int)*src;
				break;
			}
			// 2-byte
			case OP_LOAD2: {
				unsigned short* src = (unsigned short*)(qvm.datasegment + *stack);
				if (qvm.verify_data && !qvm_validate_ptr(qvm, src)) {
					LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)src);
					goto fail;
				}
				*stack = (int)*src;
				break;
			}
			// 4-byte
			case OP_LOAD4: {
				int* src = (int*)(qvm.datasegment + *stack);
				if (qvm.verify_data && !qvm_validate_ptr(qvm, src)) {
					LOG(QMM_LOG_FATAL, "QMM") << fmt::format("qvm_exec({}) {} pointer validation failed! ptr = {}\n", vmMain_code, opcodename[op], (void*)src);
					goto fail;
				}
				*stack = *src;
				break;
			}
			// copy mem at address pointed to by stack[0] to address pointed to by stack[1]
			// for 'param' number of bytes
			case OP_BLOCK_COPY: {
				std::byte* src = qvm.datasegment + *stack++;
				std::byte* dst = qvm.datasegment + *stack++;

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

		} // switch (op) {
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
