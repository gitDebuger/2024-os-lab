# 导入当前目录下的 include.mk
include ./include.mk

INCLUDES    := -I../include/

# 编译当前目录下的所有 .c 文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# 编译当前目录下的所有 .S 文件
# Lab 1 中不包含 .S 文件
# 考虑后续 Lab 中包含汇编语言文件
%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

.PHONY: all clean

all: $(targets)

clean:
	rm -rf *~ *.o
