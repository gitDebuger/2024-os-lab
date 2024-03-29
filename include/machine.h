#ifndef _MACHINE_H_
#define _MACHINE_H_

/* 在 /kern/machine.c 中被实现 */
void printcharc(char ch);
int scancharc(void);
void halt(void) __attribute__((noreturn));

#endif
