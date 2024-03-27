#include <asm/asm.h>
#include <mmu.h>

.text
# 导出 _start 以便和 C 语言程序连接
# 将 _start 设置为全局符号
EXPORT(_start)
# 允许汇编器使用 at 寄存器
.set at
# 允许汇编器进行指令重排
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
	# mtc0 s, $n
	# 将通用寄存器 s 的内容拷贝到协处理器 0 寄存器 n 中
	# 32位架构时最多有 32 个协处理器寄存器
	# 数据位数是 32 位
	# mfc0 d, $n
	# 类似的 mfc0 读取协处理器 0 寄存器 n 的值到通用寄存器 s 中
	# $8 vaddr virtual address
	# $12 status
	# $13 cause
	# $14 epc
	# 再类似的 mtc1 和 mfc1 负责操作协处理器 1 的寄存器
	# 协处理器 1 是 FPU 用于处理浮点数
	# 也就是浮点运算单元
	# 定义在 /include/asm/regdef.h 中
	mtc0    zero, CP0_STATUS

	/* hint: you can refer to the memory layout in include/mmu.h */
	/* set up the kernel stack */
	/* Exercise 1.3: Your code here. (1/2) */
	# 伪指令 li sp, 0x80400000
	# 由于 16 位立即数不足以容纳 0x80400000
	# 所以需要使用 lui-ori 指令组完成
    lui       t0, 0x00008040
    ori       sp, t0, 0x00000000
	# la        sp, 0x80400000
	/* jump to mips_init */
	/* Exercise 1.3: Your code here. (2/2) */
	# addi    sp, sp, -1024
	# 使用 j 而不是 jal 因为 mips_init 函数不会返回
	j       mips_init
