#include <stdio.h>
#include <stdlib.h>

/* binary 指向文件内容的指针 */
/* size 文件的大小 */
extern int readelf(void *binary, size_t size);

/*
	Open the ELF file specified in the argument, and call the function 'readelf'
	to parse it.
	Params:
		argc: the number of parameters
		argv: array of parameters, argv[1] should be the file name.
*/
int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
		return 1;
	}

/* Lab 1 Key Code "readelf-main" */
	/* 只读二进制格式打开文件 */
	FILE *fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		perror(argv[1]);
		return 1;
	}

	/* 将文件指针移动到文件末尾 */
	if (fseek(fp, 0, SEEK_END)) {
		perror("fseek");
		goto err;
	}
	/* 获取文件大小 */
	int fsize = ftell(fp);
	if (fsize < 0) {
		perror("ftell");
		goto err;
	}

	/* 存储文件内容 */
	/* 多余的一个字节用于存储字符串结束符 */
	char *p = malloc(fsize + 1);
	if (p == NULL) {
		perror("malloc");
		goto err;
	}
	/* 将文件指针移动到文件开头 */
	if (fseek(fp, 0, SEEK_SET)) {
		perror("fseek");
		goto err;
	}
	/* 读取文件内容到指定数组 */
	if (fread(p, fsize, 1, fp) < 0) {
		perror("fread");
		goto err;
	}
	p[fsize] = 0;
/* End of Key Code "readelf-main" */

	/* 调用 readelf 函数分析 ELF 文件 */
	return readelf(p, fsize);
err:
/* 如果出现错误则关闭文件并退出 */
	fclose(fp);
	return 1;
}
