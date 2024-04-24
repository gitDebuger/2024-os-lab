#include <env.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

/* 声明了一些异常处理函数 */

extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);
extern void handle_ri(void);

/* 异常向量组 */
/* 存放了异常处理函数的地址 */
void (*exception_handlers[32])(void) = {
    /* 默认的保留异常处理函数 */
    [0 ... 31] = handle_reserved,
    /* 中断异常使用 handle_int */
    [0] = handle_int,
    /* TLB 异常使用 handle_tlb */
    [2 ... 3] = handle_tlb,
    [10] = handle_ri,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};

/* Overview:
 *   The fallback handler when an unknown exception code is encountered.
 *   'genex.S' wraps this function in 'handle_reserved'.
 */
/* 定义保留异常处理函数 */
/* 当遇到未知的异常号时会调用这个函数 */
void do_reserved(struct Trapframe *tf) {
    /* 首先打印异常帧的信息 */
	print_tf(tf);
    /* 然后通过panic函数输出错误信息并终止程序执行 */
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}

void do_ri(struct Trapframe *tf) {
	u_int x = *((u_int *)(tf->cp0_epc));
	u_int high = x >> 26;
	u_int low = x & 0x3f;
	if (high != 0) {
		tf->cp0_epc += 4;
		return;
	} else if (low != 0x3f && low != 0x3e) {
		tf->cp0_epc += 4;
		return;
	} else if ((x >> 6 & 0x1f) != 0) {
		tf->cp0_epc += 4;
		return;
	}
	u_int rs, rt, rd;
	rs = x >> 21 & 0x1f;
	rt = x >> 16 & 0x1f;
	rd = x >> 11 & 0x1f;
	
	u_int nrs = tf->regs[rs];
	u_int nrt = tf->regs[rt];
	
	if (low == 0x3f) {
		u_int nrd = 0;
		for (int i = 0; i < 32; i += 8) {
			u_int rs_i = nrs & (0xff << i);
			u_int rt_i = nrt & (0xff << i);
			if (rs_i < rt_i) {
				nrd = nrd | rt_i;
			} else {
				nrd = nrd | rs_i;
			}
		}
		tf->regs[rd] = nrd;
	} else {
		u_int tmp = *((u_int *)nrs);
		if (tmp == nrt) {
			*((u_int *)nrs) = tf->regs[rd];
		}
		tf->regs[rd] = tmp;
	}

	tf->cp0_epc += 4;
}
