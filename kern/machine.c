#include <malta.h>
#include <mmu.h>
#include <printk.h>

/* Lab 1 Key Code "printcharc" */
/* Overview:
 *   Send a character to the console. If Transmitter Holding Register is currently occupied by
 *   some data, wait until the register becomes available.
 *
 * Pre-Condition:
 *   'ch' is the character to be sent.
 */
void printcharc(char ch) {
	if (ch == '\n') {
		printcharc('\r');
	}
	/* 等待相应寄存器可用 */
	/* 等待内存地址有效 */
	/* 不太懂这几个宏定义的含义 */
	while (!(*((volatile uint8_t *)(KSEG1 + MALTA_SERIAL_LSR)) & MALTA_SERIAL_THR_EMPTY)) {
	}
	/* 向特定内存地址写入要输出的字符 */
	*((volatile uint8_t *)(KSEG1 + MALTA_SERIAL_DATA)) = ch;
}
/* End of Key Code "printcharc" */

/* Overview:
 *   Read a character from the console.
 *
 * Post-Condition:
 *   If the input character data is ready, read and return the character.
 *   Otherwise, i.e. there's no input data available, 0 is returned immediately.
 */
/* 限时 lab 有可能利用该函数实现 scank 函数 */
/* 相关函数有 scancharc inputk scank */
int scancharc(void) {
	/* 从特定内存地址读出一个字符并返回 */
	/* 如果没有有效的输入数据 */
	/* 则立即返回 0 */
	if (*((volatile uint8_t *)(KSEG1 + MALTA_SERIAL_LSR)) & MALTA_SERIAL_DATA_READY) {
		return *((volatile uint8_t *)(KSEG1 + MALTA_SERIAL_DATA));
	}
	return 0;
}

/* Overview:
 *   Halt/Reset the whole system. Write the magic value GORESET(0x42) to SOFTRES register of the
 *   FPGA on the Malta board, initiating a board reset. In QEMU emulator, emulation will stop
 *   instead of rebooting with the parameter '-no-reboot'.
 *
 * Post-Condition:
 *   If the current device doesn't support such halt method, print a warning message and enter
 *   infinite loop.
 */
/* QEMU 停机 */
void halt(void) {
	/* 向特定内存地址写入 GORESET(0x42) 魔数以停机 */
	*(volatile uint8_t *)(KSEG1 + MALTA_FPGA_HALT) = 0x42;
	printk("machine.c:\thalt is not supported in this machine!\n");
	/* 死循环用来做什么的 */
	/* 等待 QEMU 关机吗 */
	while (1) {
	}
}
