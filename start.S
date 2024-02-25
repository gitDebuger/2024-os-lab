#include <asm/asm.h>
.data
str:
.asciiz "Hello World\n"  # Null-terminated string "Hello World" stored at label 'str'
.align 2 # align to 4-byte boundary (2^2)
var:
.byte 3 # correctly aligned byte: 3
/* '<x>' in the comments is the part to be replaced. */
/* use '.align <x>' to align the following words to 1-byte boundary (disabling word-alignment) */
/* so that the byte 3 and word 7 is "connected" */
/* Your code here. (1/6) */

.word 7, 8, 9

.text
/* We define '_start_mips' here as the entry of our program. */
EXPORT(_start_mips)
.set at
.set reorder
	mtc0    zero, CP0_STATUS
	li      sp, 0x84000000
	/* Load the address of the string 'str' into the first parameter register. */
	la      a0, str
	/* use 'addiu  sp, sp, <x>' to push a proper-sized frame onto the stack for Nonleaf function 'print_str'. */
	/* Your code here. (2/6) */

	jal     print_str
	/* use 'addiu  sp, sp, <x>' to restore stack pointer. */
	/* Your code here. (3/6) */

	/* Set the first four parameters. */
	li      a0, 0
	li      a1, 1
	li      a2, 2
	li      a3, 3
	/* use 'addiu  sp, sp, <x>' to push a proper-sized frame onto the stack for Nonleaf function 'hello'. */
	/* Your code here. (4/6) */

	lw      t1, var	
	li      t2, 5
	/* use 'sw  t1, <x>(sp)' to store t1 at the proper place of the stack */
	/* so that t1 is 5th argument of function hello. */
	/* Your code here. (5/6) */

	/* use 'sw  t2, <x>(sp)' to store t2 at the proper place of the stack */
	/* so that t2 is 6th argument of function hello. */
	/* Your code here. (6/6) */

	/* use 'j' to call the function 'hello', we use 'j' instead of 'jal' because 'hello' is 'noreturn' */
	j       hello
