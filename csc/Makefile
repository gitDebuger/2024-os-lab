# 定义编译器和编译选项
CC = gcc
CFLAGS = -Wall -Wextra

TARGETS: fibo

.PHONY: all
fibo: code/main.c code/fibo.c
	$(CC) $(CFLAGS) -o $@ $^ -Iinclude -save-temps
	rm -f *.i *.s
	mv *.o code/ 

.PHONY: clean
clean:
	rm -f code/*.o
