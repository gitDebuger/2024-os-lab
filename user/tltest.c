#include <lib.h>

/* 与 Lab 3 中的测试程序以 _start 为程序入口不同 */
/* 这里使用 main 作为程序入口 */
/* 因为在 user/lib/entry.S 中定义了统一的 _start 函数 */
int main() {
	/* 该函数在用户程序的地位相当于内核中的 printk 函数 */
	/* 定义在 user/lib/debugf.c 中 */
	debugf("Smashing some kernel codes...\n"
	       "If your implementation is correct, you may see unknown exception here:\n");
	/* 试图向内核空间写入数据 */
	/* 会产生异常 */
	/* 这里会调用 kern/genex.S 中的 do_reserved 函数 */
	/* 用于处理除了定义过异常处理函数的其他异常 */
	/* 这里会产生 5 号异常 */
	/* 表示地址错误 */
	*(int *)KERNBASE = 0;
	debugf("My mission completed!\n");
	return 0;
}
