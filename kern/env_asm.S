#include <asm/asm.h>
#include <mmu.h>
#include <trap.h>
#include <kclock.h>

/* 从栈中弹出保存的上下文信息 */
/* 并将控制流返回到异常处理结束后的位置 */
.text
LEAF(env_pop_tf)
/* 设置指令重排序和 $at 寄存器 */
.set reorder
.set at
	/* 将 $a1 中的值写入 CP0_ENTRYHI 寄存器 */
	/* 该寄存器用于管理 TLB */
	/* 通常用于设置虚拟地址的高位和 ASID */
	mtc0    a1, CP0_ENTRYHI
	/* 将 $a0 中的值移动到栈指针 $sp */
	/* 这样函数的返回地址就存放在栈上了 */
	move    sp, a0
	/* 调用 RESET_KCLOCK 宏重置时钟 */
	RESET_KCLOCK
	/* 跳转到异常返回函数 */
	j       ret_from_exception
END(env_pop_tf)
