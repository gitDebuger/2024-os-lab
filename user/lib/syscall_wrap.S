#include <asm/asm.h>

LEAF(msyscall)
	// Just use 'syscall' instruction and return.
	// 使用 syscall 指令并返回
	/* Exercise 4.1: Your code here. */
	syscall
	jr       ra
END(msyscall)
