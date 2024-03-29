# 指定头文件所在目录
INCLUDES    := -I../include/

# 目标文件
targets     := elfloader.o print.o string.o

# 从 .c 文件编译目标文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# 从 .S 文件编译目标文件
%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<
# 这里的 CC 是神奇的交叉编译器

.PHONY: all clean

all: $(targets)

clean:
	rm -rf *~ *.o
