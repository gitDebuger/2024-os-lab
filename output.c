#include <machine.h>
#include <stddef.h>
#include <output.h>

void printcharc(char ch) {
    while (!(*((volatile char *)(KSEG1 + MALTA_SERIAL_LSR)) & MALTA_SERIAL_THR_EMPTY)) {
    }
    *((volatile char *)(KSEG1 + MALTA_SERIAL_DATA)) = ch;
}

void halt(void) {
    *(volatile char *)(KSEG1 + MALTA_FPGA_HALT) = 0x42;
}

void print_str(const char *buf) {
    for (int i = 0; buf[i]; i++) {
        printcharc(buf[i]);
    }
}

void print_num(unsigned long u) {
    if (u < 0) {
        printcharc('-');
        u = -u;
    }
    char digit = '0' + u % 10;
    u /= 10;
    if (u != 0) {
        print_num(u);
    }
    printcharc(digit);
}
