#ifndef _INC_TYPES_H_
#define _INC_TYPES_H_

#include <stddef.h>
#include <stdint.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

#define MIN(_a, _b)                                                                                \
	({                                                                                         \
		typeof(_a) __a = (_a);                                                             \
		typeof(_b) __b = (_b);                                                             \
		__a <= __b ? __a : __b;                                                            \
	})

/* Rounding; only works for n = power of two */
/* 大于等于 a 的最小的 2 的倍数 */
#define ROUND(a, n) (((((u_long)(a)) + (n)-1)) & ~((n)-1))
/* 小于等于 a 的最大的 2 的倍数 */
#define ROUNDDOWN(a, n) (((u_long)(a)) & ~((n)-1))
/* 用于内存对齐 */

#endif /* !_INC_TYPES_H_ */
