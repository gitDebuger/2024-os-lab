#include <print.h>
// #include <ctype.h>

/* forward declaration */
/* @param: fmt_callback_t 回调函数 */
/* void *data 目前并没有使用 */
/* 只是单纯地传入了一个 NULL 值 */
/* 有理由猜测后续可能会把 data 作为输出目的地 */
/* 当传入指针非空时将输出到 data 指向的地方 */
static void print_char(fmt_callback_t, void *, char, int, int);
static void print_str(fmt_callback_t, void *, const char *, int, int);
static void print_num(fmt_callback_t, void *, unsigned long, int, int, int, int, char, int);

void vprintfmt(fmt_callback_t out, void *data, const char *fmt, va_list ap) {
	char c;
	const char *s;
	long num;

	/* 格式化字符串格式如下 */
	/* %[flags][width][length]<specifier> */
	/* 打印数字的最小宽度 */
	/* 通过 [width] 指定 */
	int width;
	/* 数据类型的长度 */
	/* 1 代表 (unsigned) long int */
	int long_flag; // output is long (rather than int)
	/* 是否需要打印负号 */
	/* 传入 print_num 函数的是无符号整数 */
	int neg_flag;  // output is negative
	/* 是否左对齐 */
	/* 默认右对齐 */
	/* 1 代表左对齐 */
	int ladjust;   // output is left-aligned
	/* 右对齐时使用的填充字符 */
	/* 默认为空格字符 */
	/* 可指定为字符 0 */
	char padc;     // padding char

	for (;;) {
		/* scan for the next '%' */
		/* Exercise 1.4: Your code here. (1/8) */
		/* 扫描字符串寻找 % 字符 */
		const char *p = fmt;
		while (*p && *p != '%') {
			++p;
		}
		
		/* flush the string found so far */
		/* Exercise 1.4: Your code here. (2/8) */
		/* 将前面不包含 % 的普通字符部分打印输出 */
		/* typedef void (*fmt_callback_t)(void *data, const char *buf, size_t len) */
		out(data, fmt, p - fmt);
		/* 更新 fmt 的位置到 p 的位置 */
		/* 更新后 fmt 和 p 都位于 % 字符的位置或者字符串结束符的位置 */
		fmt = p;

		/* check "are we hitting the end?" */
		/* Exercise 1.4: Your code here. (3/8) */
		/* if (*fmt == 0) break; */
		/* 检测当前位置是字符串末尾还是 % 字符位置 */
		/* 如果是字符串末尾则直接退出 */
		if (!(*fmt)) break;

		/* we found a '%' */
		/* Exercise 1.4: Your code here. (4/8) */
		/* 调整 fmt 的位置到 % 字符的下一个字符的位置 */
		++fmt;

		/* check format flag */
		/* Exercise 1.4: Your code here. (5/8) */
		/* 默认右对齐 */
		ladjust = 0; // default right-aligned
		/* 默认使用空格填充 */
		padc = ' ';  // default filled with space
		/* 检查是否有 flag 控制符 */
		/* 字符 - 和字符 0 互斥 */
		/* 如果检测到 flag 控制符则设置对应的参数并将 fmt 移动到下一个位置 */
		/* 如果未检测到 flag 控制符则不移动位置 */
		if (*fmt == '-') {
			// set left-aligned
			ladjust = 1;
			++fmt;
		} else if (*fmt == '0') {
			// filled with character 0
			padc = '0';
			++fmt;
		}

		/* get width */
		/* Exercise 1.4: Your code here. (6/8) */
		/* 获取 width 控制符 */
		/* 默认控制符为 0 */
		width = 0;
		/* 循环获取 width 控制符的值 */
		while (*fmt && *fmt >= '0' && *fmt <= '9') {
			width = width * 10 + *fmt - '0';
			++fmt;
		}
		/* 循环结束后 fmt 位于 width 控制符的后一个字符 */

		/* check for long */
		/* Exercise 1.4: Your code here. (7/8) */
		/* 检查是否存在 l 控制符 */
		/* 如果存在 l 将 long_flag 设置为 1 表示长整型并将 fmt 后移一位 */
		/* 如果不存在则不移动 */
		if (*fmt == 'l') {
			long_flag = 1;
			++fmt;
		} else {
			long_flag = 0;
		}

		/* 检测 specifier 控制符以确定输出变量的类型 */
		/* 设置变量 neg_flag 表示是否需要输出负号 */
		/* 默认不输出负号 */
		neg_flag = 0;
		/* va_list ap 已经在 vprintfmt 函数的调用者 printk 中被初始化 */
		/* 并且会在 printk */
		switch (*fmt) {
		/* 无符号二进制格式输出 */
		case 'b':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			/* 目前不知道 data 参数的用意 */
			print_num(out, data, num, 2, 0, width, ladjust, padc, 0);
			break;

		/* 有符号十进制格式输出 */
		case 'd':
		case 'D':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}

			/*
			 * Refer to other parts (case 'b', case 'o', etc.) and func 'print_num' to
			 * complete this part. Think the differences between case 'd' and the
			 * others. (hint: 'neg_flag').
			 */
			/* Exercise 1.4: Your code here. (8/8) */
			/* 如果 num 小于零则取其相反数 */
			/* 并将 neg_flag 设置为 1 表示需要输出负号 */
			if (num < 0) {
				num = -num;
				neg_flag = 1;
			}
			print_num(out, data, num, 10, neg_flag, width, ladjust, padc, 0);

			break;

		/* 无符号八进制格式输出 */
		case 'o':
		case 'O':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 8, 0, width, ladjust, padc, 0);
			break;

		/* 无符号十进制格式输出 */
		case 'u':
		case 'U':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 10, 0, width, ladjust, padc, 0);
			break;

		/* 小写十六进制格式输出 */
		case 'x':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 16, 0, width, ladjust, padc, 0);
			break;

		/* 大写十六进制格式输出 */
		case 'X':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 16, 0, width, ladjust, padc, 1);
			break;

		/* 字符格式输出 */
		case 'c':
			/* 能够发现这里使用的是 int 类型获取变量 */
			/* 也就是说在 va_list 中 char 类型会被扩展为 int 类型 */
			c = (char)va_arg(ap, int);
			/* 打印输出字符 */
			/* 这里不再需要填充字符 */
			print_char(out, data, c, width, ladjust);
			break;

		case 'P':
			print_char(out, data, '(', 1, ladjust);
			long x, y, z;
			if (long_flag) {
				x = va_arg(ap, long int);
				y = va_arg(ap, long int);
			} else {
				x = va_arg(ap, int);
				y = va_arg(ap, int);
			}
			if (x < 0) {
				x = -x;
				neg_flag = 1;
			}
			print_num(out, data, x, 10, neg_flag, width, ladjust, padc, 0);
			print_char(out, data, ',', 1, ladjust);
			if (y < 0) {
				y = -y;
				neg_flag = 1;
			}
			print_num(out, data, y, 10, neg_flag, width, ladjust, padc, 0);
			print_char(out, data, ',', 1, ladjust);
			z = (x + y) * (x - y);
			if (z < 0) {
				z = -z;
			}
			print_num(out, data, z, 10, 0, width, ladjust, padc, 0);
			print_char(out, data, ')', 1, ladjust);
			break;


		/* 字符串格式输出 */
		case 's':
			/* 从 va_list 中获取字符串 */
			s = (char *)va_arg(ap, char *);
			print_str(out, data, s, width, ladjust);
			break;

		/* 如果是字符串结束符则将 fmt 回退一位 */
		case '\0':
			fmt--;
			/* 这里直接 return 也是可以的 */
			break;

		/* 不符合上述条件则打印 fmt 指向的字符 */
		default:
			/* output this char as it is */
			out(data, fmt, 1);
			/* 这里也可以 fmt-- 等待下次再输出当前字符 */
		}
		/* 上面两种情况说明本次 % 引导的字符序列并没有指向任何变量 */
		/* 直接进入下一轮循环 */
		/* 处理完成之后将 fmt 后移一位准备下一轮循环 */
		fmt++;
	}
}

/* --------------- local help functions --------------------- */
void print_char(fmt_callback_t out, void *data, char c, int length, int ladjust) {
	int i;

	/* 输出宽度至少为 1 */
	if (length < 1) {
		length = 1;
	}
	/* 填充字符为空格 */
	const char space = ' ';
	if (ladjust) {
		/* 处理左对齐 */
		/* 先输出传入的字符 */
		out(data, &c, 1);
		/* 然后循环填充空格占位符 */
		for (i = 1; i < length; i++) {
			out(data, &space, 1);
		}
	} else {
		/* 处理右对齐 */
		/* 先循环输出空白符 */
		for (i = 0; i < length - 1; i++) {
			out(data, &space, 1);
		}
		/* 输入传入的字符 */
		out(data, &c, 1);
	}
}

void print_str(fmt_callback_t out, void *data, const char *s, int length, int ladjust) {
	int i;
	int len = 0;
	/* 确定需要输出的长度 */
	/* 其实调用 strlen 函数是更好的选择 */
	const char *s1 = s;
	while (*s1++) {
		len++;
	}
	if (length < len) {
		length = len;
	}

	/* 根据左对齐和右对齐的不同分别输出字符串内容 */
	if (ladjust) {
		out(data, s, len);
		for (i = len; i < length; i++) {
			out(data, " ", 1);
		}
	} else {
		for (i = 0; i < length - len; i++) {
			out(data, " ", 1);
		}
		out(data, s, len);
	}
}

void print_num(fmt_callback_t out, void *data, unsigned long u, int base, int neg_flag, int length,
		   int ladjust, char padc, int upcase) {
	/* algorithm :
	 *  1. prints the number from left to right in reverse form.
	 *  2. fill the remaining spaces with padc if length is longer than
	 *     the actual length
	 *     TRICKY : if left adjusted, no "0" padding.
	 *		    if negtive, insert  "0" padding between "0" and number.
	 *  3. if (!ladjust) we reverse the whole string including paddings
	 *  4. otherwise we only reverse the actual string representing the num.
	 */

	int actualLength = 0;
	char buf[length + 70];
	char *p = buf;
	int i;

	/* 将数字倒序存放在数组中 */
	do {
		int tmp = u % base;
		if (tmp <= 9) {
			*p++ = '0' + tmp;
		} else if (upcase) {
			*p++ = 'A' + tmp - 10;
		} else {
			*p++ = 'a' + tmp - 10;
		}
		u /= base;
	} while (u != 0);

	/* 根据 neg_flag 存入负号 */
	if (neg_flag) {
		*p++ = '-';
	}

	/* figure out actual length and adjust the maximum length */
	/* 确定输出长度 */
	actualLength = p - buf;
	if (length < actualLength) {
		length = actualLength;
	}

	/* add padding */
	if (ladjust) {
		padc = ' ';
	}
	/* 负数并且右对齐并且使用字符 0 填充 */
	if (neg_flag && !ladjust && (padc == '0')) {
		/* 在数字高位补 0 */
		for (i = actualLength - 1; i < length - 1; i++) {
			buf[i] = padc;
		}
		/* 然后加入负号 */
		buf[length - 1] = '-';
	} else {
		/* 否则直接填充 */
		for (i = actualLength; i < length; i++) {
			buf[i] = padc;
		}
	}

	/* prepare to reverse the string */
	/* 翻转字符串 */
	int begin = 0;
	int end;
	/* 左对齐只翻转数字部分 */
	/* 不翻转后面的空白符 */
	if (ladjust) {
		end = actualLength - 1;
	} else {
		/* 右对齐连同空白符一起翻转 */
		end = length - 1;
	}

	/* adjust the string pointer */
	/* 双指针翻转字符串 */
	while (end > begin) {
		char tmp = buf[begin];
		buf[begin] = buf[end];
		buf[end] = tmp;
		begin++;
		end--;
	}

	/* 调用 out 输出字符串 */
	out(data, buf, length);
}
