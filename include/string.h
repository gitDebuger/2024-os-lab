#ifndef _STRING_H_
#define _STRING_H_

#include <types.h>

/* 一些字符串操作函数 */
/* 定义在 string.c 文件中 */
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
const char *strchr(const char *s, int c);
int strcmp(const char *p, const char *q);

#endif
