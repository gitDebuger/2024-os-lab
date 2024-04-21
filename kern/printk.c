#include <print.h>
#include <printk.h>
#include <trap.h>

/* Lab 1 Key Code "outputk" */
/* 向控制台输出特定长度的一串字符 */
/* @param: buf 字符序列首地址 */
/* @param: len 输出字符序列的长度 */
void outputk(void *data, const char *buf, size_t len) {
	for (int i = 0; i < len; i++) {
		printcharc(buf[i]);
	}
}
/* End of Key Code "outputk" */

/* Lab 1 Key Code "printk" */
/* 初始化参数列表并调用 vprintfmt 函数 */
/* 调用结束后关闭参数列表 */
/* 将 outputk 传递给 vprintfmt 函数以便其调用 */
void printk(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintfmt(outputk, NULL, fmt, ap);
	va_end(ap);
}
/* End of Key Code "printk" */

/* 打印 Trapframe 结构体的信息 */
void print_tf(struct Trapframe *tf) {
	/* 打印 32 个通用寄存器的信息 */
	for (int i = 0; i < sizeof(tf->regs) / sizeof(tf->regs[0]); i++) {
		printk("$%2d = %08x\n", i, tf->regs[i]);
	}
	/* 打印特殊寄存器的信息 */
	printk("HI  = %08x\n", tf->hi);
	printk("LO  = %08x\n\n", tf->lo);
	printk("CP0.SR    = %08x\n", tf->cp0_status);
	printk("CP0.BadV  = %08x\n", tf->cp0_badvaddr);
	printk("CP0.Cause = %08x\n", tf->cp0_cause);
	printk("CP0.EPC   = %08x\n", tf->cp0_epc);
}
