# 定义编译器和编译选项
CC = gcc
CFLAGS = -Wall -Wextra

OBJ = fibo.o main.o

TARGETS: ../fibo

.PHONY: all
all: ../fibo

.PHONY: ../fibo
../fibo: main.c fibo.c
	$(CC) $(CFLAGS) -o $@ $^ -I../include -save-temps
	rm -f *.i *.s 

.PHONY: clean
clean:
	rm -f *.o
