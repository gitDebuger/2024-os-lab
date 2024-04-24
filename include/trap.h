#ifndef _TRAP_H_
#define _TRAP_H_

#ifndef __ASSEMBLER__

#include <types.h>

/* 保存因异常或中断陷入内核时的栈帧信息 */
/* 以便异常处理程序可以恢复处理器的状态并继续执行 */
struct Trapframe {
	/* Saved main processor registers. */
	/* 保存 32 个通用寄存器信息 */
	unsigned long regs[32];

	/* Saved special registers. */
	/* 保存一些特殊的协处理器寄存器信息 */

	/* 状态寄存器信息 */
	/* 特权级别和中断使能状态等 */
	unsigned long cp0_status;
	/* HI 寄存器 */
	/* 乘除运算高 32 位 */
	unsigned long hi;
	/* LO 寄存器 */
	/* 乘除运算低 32 位 */
	unsigned long lo;
	/* BadVAddr 寄存器 */
	/* 最近一次发生地址错误时的地址值 */
	unsigned long cp0_badvaddr;
	/* Cause 寄存器 */
	/* 最近一次异常或中断的原因和类型 */
	unsigned long cp0_cause;
	/* EPC 寄存器 */
	/* 最近一次发生异常时的指令地址 */
	/* 异常程序计数器 */
	unsigned long cp0_epc;
	/* CP0_COUNT 寄存器 */
	unsigned long cp0_clock;
};

/* 打印 Trapframe 结构体的信息 */
/* 在 /kern/printk.c 中被实现 */
void print_tf(struct Trapframe *tf);

#endif /* !__ASSEMBLER__ */

/**
 * Stack layout for all exceptions
 * 所有异常堆栈布局
*/

/* 定义 32 个通用寄存器 */

#define TF_REG0 0
#define TF_REG1 ((TF_REG0) + 4)
#define TF_REG2 ((TF_REG1) + 4)
#define TF_REG3 ((TF_REG2) + 4)
#define TF_REG4 ((TF_REG3) + 4)
#define TF_REG5 ((TF_REG4) + 4)
#define TF_REG6 ((TF_REG5) + 4)
#define TF_REG7 ((TF_REG6) + 4)
#define TF_REG8 ((TF_REG7) + 4)
#define TF_REG9 ((TF_REG8) + 4)
#define TF_REG10 ((TF_REG9) + 4)
#define TF_REG11 ((TF_REG10) + 4)
#define TF_REG12 ((TF_REG11) + 4)
#define TF_REG13 ((TF_REG12) + 4)
#define TF_REG14 ((TF_REG13) + 4)
#define TF_REG15 ((TF_REG14) + 4)
#define TF_REG16 ((TF_REG15) + 4)
#define TF_REG17 ((TF_REG16) + 4)
#define TF_REG18 ((TF_REG17) + 4)
#define TF_REG19 ((TF_REG18) + 4)
#define TF_REG20 ((TF_REG19) + 4)
#define TF_REG21 ((TF_REG20) + 4)
#define TF_REG22 ((TF_REG21) + 4)
#define TF_REG23 ((TF_REG22) + 4)
#define TF_REG24 ((TF_REG23) + 4)
#define TF_REG25 ((TF_REG24) + 4)
/**
 * $26 (k0) and $27 (k1) not saved
 * 寄存器 $26 和 $27 不保存
*/
#define TF_REG26 ((TF_REG25) + 4)
#define TF_REG27 ((TF_REG26) + 4)
#define TF_REG28 ((TF_REG27) + 4)
#define TF_REG29 ((TF_REG28) + 4)
#define TF_REG30 ((TF_REG29) + 4)
#define TF_REG31 ((TF_REG30) + 4)

/* 定义其他寄存器 */
/* 状态寄存器 */
#define TF_STATUS ((TF_REG31) + 4)
/* HI 寄存器 */
#define TF_HI ((TF_STATUS) + 4)
/* LO 寄存器 */
#define TF_LO ((TF_HI) + 4)

/* BadVAddr 寄存器 */
#define TF_BADVADDR ((TF_LO) + 4)
/* Cause 寄存器 */
#define TF_CAUSE ((TF_BADVADDR) + 4)
/* EPC 寄存器 */
#define TF_EPC ((TF_CAUSE) + 4)
/* CP0_COUNT 寄存器累加值 */
#define TF_CLOCK ((TF_EPC) + 4)
/**
 * Size of stack frame, word/double word alignment
 * 栈帧大小  字或双字对齐
*/
#define TF_SIZE ((TF_CLOCK) + 4)
#endif /* _TRAP_H_ */
