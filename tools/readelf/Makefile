# 通配符设置编译文件
%.o: %.c
	$(CC) -c $<

# 指定伪目标 clean
.PHONY: clean

# 编译解析 32 位 ELF 文件的 readelf 程序
readelf: main.o readelf.o
	$(CC) $^ -o $@

# 编译 32 位 hello 程序
# 用于测试 readelf 程序
hello: hello.c
	$(CC) $^ -o $@ -m32 -static -g

# 清除 readelf 和 hello 程序
clean:
	rm -f *.o readelf hello
