#ifndef _KCLOCK_H_
#define _KCLOCK_H_

#include <asm/asm.h>

#define TIMER_INTERVAL (500000) // WARNING: DO NOT MODIFY THIS LINE!

// clang-format off
.macro RESET_KCLOCK
	li 	 t0, TIMER_INTERVAL
	/*
	 * Hint:
	 *   Use 'mtc0' to write an appropriate value into the CP0_COUNT and CP0_COMPARE registers.
	 *   Writing to the CP0_COMPARE register will clear the timer interrupt.
	 *   The CP0_COUNT register increments at a fixed frequency. When the values of CP0_COUNT and
	 *   CP0_COMPARE registers are equal, the timer interrupt will be triggered.
	 *
	 */
	/**
	 * 使用 mtc0 向 CP0_COUNT 寄存器和 CP0_COMPARE 寄存器写入正确的值
	 * 向 CP0_COMPARE 寄存器写入将会重置时钟中断
	 * CP0_COUNT 寄存器以固定的频率递增
	 * 当 CP0_COUNT 和 CP0_COMPARE 寄存器的值相等时将触发时钟中断。
	*/
	/* Exercise 3.11: Your code here. */
	mtc0 zero, CP0_COUNT
	mtc0 t0, CP0_COMPARE

.endm
// clang-format on
#endif
