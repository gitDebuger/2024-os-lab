# 定义编译器和编译选项
CC = gcc
CFLAGS = -Wall -Wextra

# 目标文件和依赖关系
TARGETS = casegen calc
OBJ = casegen.o calc.o

# 默认目标
.PHONY: all
all: out

# 编译生成可执行文件
casegen: casegen.c
	$(CC) $(CFLAGS) -o $@ $^

calc: calc.c
	$(CC) $(CFLAGS) -o $@ $^

# 生成测试样例
.PHONY: case_add case_sub case_mul case_div case_all
case_add: casegen
	./casegen add 100 > case_add

case_sub: casegen
	./casegen sub 100 > case_sub

case_mul: casegen
	./casegen mul 100 > case_mul

case_div: casegen
	./casegen div 100 > case_div

case_all: casegen
	./casegen add 100 > case_add
	./casegen sub 100 > case_sub
	./casegen mul 100 > case_mul
	./casegen div 100 > case_div
	cat case_add case_sub case_mul case_div > case_all

# 计算结果
.PHONY: out
out: case_all calc
	./calc < case_all > out

# 清理生成的文件
.PHONY: clean
clean:
	rm -f $(TARGETS) case_* out

