#include <asm/asm.h>
#include <mmu.h>

.text
# 导出 _start 以便和 C 语言程序连接
EXPORT(_start)
.set at
.set reorder
# 循环清空 .bss 段的数据
/* Lab 1 Key Code "enter-kernel" */
	/* clear .bss segment */
	la      v0, bss_start
	la      v1, bss_end
clear_bss_loop:
	beq     v0, v1, clear_bss_done
	sb      zero, 0(v0)
	addiu   v0, v0, 1
	j       clear_bss_loop
/* End of Key Code "enter-kernel" */

clear_bss_done:
	/* disable interrupts */
	mtc0    zero, CP0_STATUS

	/* hint: you can refer to the memory layout in include/mmu.h */
	/* set up the kernel stack */
	/* Exercise 1.3: Your code here. (1/2) */
    lui       t0, 0x00008040
    ori       sp, t0, 0x00000000
	# la        sp, 0x80400000
	/* jump to mips_init */
	/* Exercise 1.3: Your code here. (2/2) */
	# addi    sp, sp, -1024
	j       mips_init
