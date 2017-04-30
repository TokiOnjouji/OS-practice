/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

TRAPHANDLER_NOEC(f0,0)
TRAPHANDLER_NOEC(f1,1)
TRAPHANDLER_NOEC(f3,3)
TRAPHANDLER_NOEC(f4,4)
TRAPHANDLER_NOEC(f5,5)
TRAPHANDLER_NOEC(f6,6)
TRAPHANDLER_NOEC(f7,7)
TRAPHANDLER(f8,8)
TRAPHANDLER_NOEC(f9,9)
TRAPHANDLER(f10,10)
TRAPHANDLER(f11,11)
TRAPHANDLER(f12,12)
TRAPHANDLER(f13,13)
TRAPHANDLER(f14,14)
TRAPHANDLER_NOEC(f16,16)
TRAPHANDLER_NOEC(f32,32)
TRAPHANDLER_NOEC(f33,33)
TRAPHANDLER_NOEC(f34,34)
TRAPHANDLER_NOEC(f35,35)
TRAPHANDLER_NOEC(f36,36)
TRAPHANDLER_NOEC(f37,37)
TRAPHANDLER_NOEC(f38,38)
TRAPHANDLER_NOEC(f39,39)
TRAPHANDLER_NOEC(f40,40)
TRAPHANDLER_NOEC(f41,41)
TRAPHANDLER_NOEC(f42,42)
TRAPHANDLER_NOEC(f43,43)
TRAPHANDLER_NOEC(f44,44)
TRAPHANDLER_NOEC(f45,45)
TRAPHANDLER_NOEC(f46,46)
TRAPHANDLER_NOEC(f47,47)
TRAPHANDLER_NOEC(f48,48)
/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
pushl %ds
pushl %es
pushal
movl $GD_KD,%eax
movl %eax,%ds
movl %eax,%es
pushl %esp
call trap

