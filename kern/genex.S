#include <asm/asm.h>
#include <stackframe.h>
/* 异常处理函数定义 */

/* 宏定义——用于生成异常处理函数 */
/* 宏接受两个参数 */
/* 分别是异常的名称和异常处理函数的名称 */
/* 这个宏内部定义了一个异常处理函数 */
/* 它的名字是 handle_exception */
/* 这里使用了 \exception 和 \handler 来表示传入的参数 */
/* 这个函数保存了当前的异常帧 trap_frame */
/* 然后通过 jal 指令调用指定的异常处理程序 */
/* 最后返回到异常发生的地方 */
.macro BUILD_HANDLER exception handler
NESTED(handle_\exception, TF_SIZE + 8, zero)
	move    a0, sp
	addiu   sp, sp, -8
	jal     \handler
	addiu   sp, sp, 8
	j       ret_from_exception
END(handle_\exception)
.endm

.text

/* 定义返回异常的函数 */
/* 用于从异常处理函数中返回到异常发生的地方 */
FEXPORT(ret_from_exception)
	/* 首先恢复所有寄存器的值 */
	RESTORE_ALL
	/* 返回到异常发生的位置 */
	eret

/* 定义处理中断的函数 */
NESTED(handle_int, TF_SIZE, zero)
	/* 首先读取CP0_CAUSE和CP0_STATUS寄存器的值 */
	mfc0    t0, CP0_CAUSE
	mfc0    t2, CP0_STATUS
	/* 然后将其与操作以确定是否有中断被触发 */
	and     t0, t2
	andi    t1, t0, STATUS_IM7
	/* 如果有中断被触发则跳转到 timer_irq 标签处处理定时器中断 */
	bnez    t1, timer_irq
timer_irq:
	li      a0, 0
	/* 最终调用 schedule 函数 */
	j       schedule
END(handle_int)

/* 根据条件生成其他异常处理函数 */

/* TLB异常 */
BUILD_HANDLER tlb do_tlb_refill

#if !defined(LAB) || LAB >= 4
/* 修改异常 */
BUILD_HANDLER mod do_tlb_mod
/* 系统调用异常 */
BUILD_HANDLER sys do_syscall
#endif

/* 处理保留异常 */
BUILD_HANDLER reserved do_reserved
BUILD_HANDLER ri do_ri
