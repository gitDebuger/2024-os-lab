#include <asm/asm.h>
#include <stackframe.h>

/* 用于处理 TLB 缺失异常的入口点 */
.section .text.tlb_miss_entry
tlb_miss_entry:
	/* 直接跳转到通用异常入口点 exc_gen_entry */
	j       exc_gen_entry

/* 定义通用异常入口点 */
.section .text.exc_gen_entry
exc_gen_entry:
	/* 首先保存了所有寄存器的状态 */
	SAVE_ALL
	/*
	* Note: When EXL is set or UM is unset, the processor is in kernel mode.
	* When EXL is set, the value of EPC is not updated when a new exception occurs.
	* To keep the processor in kernel mode and enable exception reentrancy,
	* we unset UM and EXL, and unset IE to globally disable interrupts.
	*/
	/* 重置了处理器的状态 */
	/* 确保处理器处于内核模式并禁用了中断 */
	/* 清除了状态寄存器中的用户模式 UM 位 */
	/* 异常级别 EXL 位和全局中断使能 IE 位 */
	mfc0    t0, CP0_STATUS
	and     t0, t0, ~(STATUS_UM | STATUS_EXL | STATUS_IE)
	mtc0    t0, CP0_STATUS
/* Exercise 3.9: Your code here. */
	/* 获取异常原因并跳转到相应的异常处理函数 */
	mfc0    t0, CP0_CAUSE
	andi    t0, 0x7c
	lw      t0, exception_handlers(t0)
	jr      t0
