# 引入 include.mk
include include.mk

# 定义伪目标 all clean
.PHONY: all clean

all: $(targets)

# 使用通配符
# 在构建任意目标文件时先查找是否存在与目标文件同名的 .c 源文件
%: %.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(targets)
