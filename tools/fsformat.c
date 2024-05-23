#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#include "../user/include/fs.h"

/* Static assert, for compile-time assertion checking */
#define static_assert(c) (void)(char(*)[(c) ? 1 : -1])0

#define nelem(x) (sizeof(x) / sizeof((x)[0]))
typedef struct Super Super;
typedef struct File File;

#define NBLOCK 1024 // The number of blocks in the disk.
uint32_t nbitblock; // the number of bitmap blocks.
uint32_t nextbno;   // next availiable block.

struct Super super; // super block.

enum {
	BLOCK_FREE = 0,
	BLOCK_BOOT = 1,
	BLOCK_BMAP = 2,
	BLOCK_SUPER = 3,
	BLOCK_DATA = 4,
	BLOCK_FILE = 5,
	BLOCK_INDEX = 6,
};

// 在构筑磁盘镜像时暂存磁盘数据
// 等到构筑完成之后再将 `disk` 中 `data` 的内容拼接并输出为二进制镜像文件
struct Block {
	uint8_t data[BLOCK_SIZE]; // 存储磁盘块数据
	uint32_t type; // 磁盘块类型，取值为上方枚举类型的值
} disk[NBLOCK];

// reverse: mutually transform between little endian and big endian.
void reverse(uint32_t *p) {
	uint8_t *x = (uint8_t *)p;
	uint32_t y = *(uint32_t *)x;
	x[3] = y & 0xFF;
	x[2] = (y >> 8) & 0xFF;
	x[1] = (y >> 16) & 0xFF;
	x[0] = (y >> 24) & 0xFF;
}

// reverse_block: reverse proper filed in a block.
void reverse_block(struct Block *b) {
	int i, j;
	struct Super *s;
	struct File *f, *ff;
	uint32_t *u;

	switch (b->type) {
	case BLOCK_FREE:
	case BLOCK_BOOT:
		break; // do nothing.
	case BLOCK_SUPER:
		s = (struct Super *)b->data;
		reverse(&s->s_magic);
		reverse(&s->s_nblocks);

		ff = &s->s_root;
		reverse(&ff->f_size);
		reverse(&ff->f_type);
		for (i = 0; i < NDIRECT; ++i) {
			reverse(&ff->f_direct[i]);
		}
		reverse(&ff->f_indirect);
		break;
	case BLOCK_FILE:
		f = (struct File *)b->data;
		for (i = 0; i < FILE2BLK; ++i) {
			ff = f + i;
			if (ff->f_name[0] == 0) {
				break;
			} else {
				reverse(&ff->f_size);
				reverse(&ff->f_type);
				for (j = 0; j < NDIRECT; ++j) {
					reverse(&ff->f_direct[j]);
				}
				reverse(&ff->f_indirect);
			}
		}
		break;
	case BLOCK_INDEX:
	case BLOCK_BMAP:
		u = (uint32_t *)b->data;
		for (i = 0; i < BLOCK_SIZE / 4; ++i) {
			reverse(u + i);
		}
		break;
	}
}

// Initial the disk. Do some work with bitmap and super block.
// 初始化磁盘
// 处理 bitmap 和 super block
void init_disk() {
	int i, diff;

	// Step 1: Mark boot sector block.
	// 第一步：标记主引导扇区
	disk[0].type = BLOCK_BOOT;

	// Step 2: Initialize boundary.
	// 第二步：初始化边界
	nextbno = 1;
	// (1024 + 4096 * 8 - 1) / (4096 * 8) = 1
	nbitblock = (NBLOCK + BLOCK_SIZE_BIT - 1) / BLOCK_SIZE_BIT;
	// 2 + 1 = 3
	// 前面的块为超级块
	nextbno = 2 + nbitblock;

	// Step 2: Initialize bitmap blocks.
	// 初始化存储位图的磁盘块
	for (i = 0; i < nbitblock; ++i) {
		// 标记为 BLOCK_BMAP
		disk[2 + i].type = BLOCK_BMAP;
	}
	for (i = 0; i < nbitblock; ++i) {
		// 使用 1 表示磁盘块空闲
		memset(disk[2 + i].data, 0xff, BLOCK_SIZE);
	}
	// 最后如果位图不足以占用全部空间
	if (NBLOCK != nbitblock * BLOCK_SIZE_BIT) {
		// 将最后一个磁盘块末尾不作为位图使用的部分置 0
		diff = NBLOCK % BLOCK_SIZE_BIT / 8;
		memset(disk[2 + (nbitblock - 1)].data + diff, 0x00, BLOCK_SIZE - diff);
	}

	// Step 3: Initialize super block.
	// 初始化超级块
	// 文件系统的起点
	disk[1].type = BLOCK_SUPER; // 类型为超级块
	super.s_magic = FS_MAGIC; // 魔数
	super.s_nblocks = NBLOCK; // 磁盘块个数
	super.s_root.f_type = FTYPE_DIR; // 根目录文件起点 s_root
	strcpy(super.s_root.f_name, "/");
}

// Get next block id, and set `type` to the block's type.
// 获取下一个磁盘块的编号
// 并设置磁盘块的类型
int next_block(int type) {
	disk[nextbno].type = type;
	return nextbno++;
}

// Flush disk block usage to bitmap.
// 刷新位图中磁盘块的使用情况
void flush_bitmap() {
	int i;
	// update bitmap, mark all bit where corresponding block is used.
	// 刷新位图文件
	// 标记相关磁盘块已使用
	for (i = 0; i < nextbno; ++i) {
		((uint32_t *)disk[2 + i / BLOCK_SIZE_BIT].data)[(i % BLOCK_SIZE_BIT) / 32] &=
		    ~(1 << (i % 32));
	}
}

// Finish all work, dump block array into physical file.
// 完成所有工作
// 将 block 数组的内容写入物理文件中
void finish_fs(char *name) {
	int fd, i;

	// Prepare super block.
	// 将超级块写入磁盘镜像中
	memcpy(disk[1].data, &super, sizeof(super));

	// Dump data in `disk` to target image file.
	// 将 disk 数组的数据写入到目标镜像文件
	fd = open(name, O_RDWR | O_CREAT, 0666);
	for (i = 0; i < 1024; ++i) {
// 用于处理宿主机和MOS操作系统大小端不一致的情况
#ifdef CONFIG_REVERSE_ENDIAN
		reverse_block(disk + i);
#endif
		ssize_t n = write(fd, disk[i].data, BLOCK_SIZE);
		assert(n == BLOCK_SIZE);
	}

	// Finish.
	// 完成写入操作并关闭镜像文件
	close(fd);
}

// Save block link.
// 保存磁盘块链接
void save_block_link(struct File *f, int nblk, int bno) {
	// 文件过大无法保存
	assert(nblk < NINDIRECT); // if not, file is too large !

	if (nblk < NDIRECT) {
		f->f_direct[nblk] = bno;
	} else {
		if (f->f_indirect == 0) {
			// create new indirect block.
			// 创建新的非直接链接磁盘块
			f->f_indirect = next_block(BLOCK_INDEX);
		}
		((uint32_t *)(disk[f->f_indirect].data))[nblk] = bno;
	}
}

// Make new block contains link to files in a directory.
// 在目录中创建包含到文件的链接的磁盘块
int make_link_block(struct File *dirf, int nblk) {
	// 申请新的磁盘块
	int bno = next_block(BLOCK_FILE);
	// 将新申请的磁盘块设置到 f_direct 或 f_indirect 中的相应位置
	save_block_link(dirf, nblk, bno);
	dirf->f_size += BLOCK_SIZE;
	return bno;
}

// Overview:
//  Allocate an unused 'struct File' under the specified directory.
//
//  Note that when we delete a file, we do not re-arrange all
//  other 'File's, so we should reuse existing unused 'File's here.
//
// Post-Condition:
//  Return a pointer to an unused 'struct File'.
//  We assume that this function will never fail.
//
// Hint:
//  Use 'make_link_block' to allocate a new block for the directory if there are no existing unused
//  'File's.
// 申请一个特定目录下没有使用过的文件控制块结构体
// 注意当我们删除一个文件时
// 我们不会重新排列其他文件
// 所以我们应当重用未使用的文件控制块
struct File *create_file(struct File *dirf) {
	int nblk = dirf->f_size / BLOCK_SIZE;

	// Step 1: Iterate through all existing blocks in the directory.
	// 第一步：遍历目录中所有已经存在的磁盘块
	for (int i = 0; i < nblk; ++i) {
		// 磁盘块编号
		int bno; // the block number
		// If the block number is in the range of direct pointers (NDIRECT), get the 'bno'
		// directly from 'f_direct'. Otherwise, access the indirect block on 'disk' and get
		// the 'bno' at the index.
		// 如果磁盘块编号在直接指针的范围内则直接从数组中获取
		// 否则到非直接磁盘块的对应索引位置获取磁盘块编号
		/* Exercise 5.5: Your code here. (1/3) */
		if (i < NDIRECT) {
			bno = dirf->f_direct[i];
		} else {
			bno = ((uint32_t *)(disk[dirf->f_indirect].data))[i];
		}

		// Get the directory block using the block number.
		// 使用磁盘块编号获取目录磁盘块
		struct File *blk = (struct File *)(disk[bno].data);

		// Iterate through all 'File's in the directory block.
		// 遍历目录磁盘块中的所有文件
		for (struct File *f = blk; f < blk + FILE2BLK; ++f) {
			// If the first byte of the file name is null, the 'File' is unused.
			// Return a pointer to the unused 'File'.
			// 如果文件名的第一个字节是空则该文件控制块未使用
			// 返回未使用的文件控制块的指针
			/* Exercise 5.5: Your code here. (2/3) */
			if (f->f_name[0] == NULL) {
				return f;
			}
		}
	}

	// Step 2: If no unused file is found, allocate a new block using 'make_link_block' function
	// 如果没有找到未使用的文件控制块
	// 则使用 make_link_block 申请一个新的磁盘块
	// 然后返回磁盘中新的磁盘块的指针
	// and return a pointer to the new block on 'disk'.
	/* Exercise 5.5: Your code here. (3/3) */
	return (struct File *)(disk[make_link_block(dirf, nblk)].data);
}

// Write file to disk under specified dir.
// 将特定目录下的文件写入到磁盘镜像中
void write_file(struct File *dirf, const char *path) {
	int iblk = 0, r = 0, n = sizeof(disk[0].data);
	// 在 dirf 目录下创建新文件
	struct File *target = create_file(dirf);

	/* in case `create_file` is't filled */
	if (target == NULL) {
		return;
	}

	// 打开宿主机上的文件
	int fd = open(path, O_RDONLY);

	// Get file name with no path prefix.
	// 复制文件名
	// 不知为何不使用 basename
	const char *fname = strrchr(path, '/');
	if (fname) {
		fname++;
	} else {
		fname = path;
	}
	strcpy(target->f_name, fname);

	// 获取并设置文件大小
	target->f_size = lseek(fd, 0, SEEK_END);
	target->f_type = FTYPE_REG;

	// Start reading file.
	// 开始读取文件内容并写入磁盘镜像文件中
	lseek(fd, 0, SEEK_SET);
	// 循环逐块读取并写入
	// 值得注意的是这里先写入后申请
	// 因为这里写入的其实是内存中暂存的磁盘数据
	// 只有成功读取并写入才能申请新的磁盘空间
	while ((r = read(fd, disk[nextbno].data, n)) > 0) {
		save_block_link(target, iblk++, next_block(BLOCK_DATA));
	}
	// 关闭文件
	close(fd); // Close file descriptor.
}

// Overview:
//  Write directory to disk under specified dir.
//  Notice that we may use POSIX library functions to operate on
//  directory to get file infomation.
//
// Post-Condition:
//  We ASSUME that this funcion will never fail
// 将目录写入特定目录下的磁盘镜像中
// 注意我们将使用 POSIX 库函数操作目录以获取相应信息
// 我们假定该函数永远不会出错
void write_directory(struct File *dirf, char *path) {
	DIR *dir = opendir(path);
	if (dir == NULL) {
		perror("opendir");
		return;
	}
	// 在 dirf 文件下创建新文件
	struct File *pdir = create_file(dirf);
	// 设置新的目录文件的名称和文件类型
	strncpy(pdir->f_name, basename(path), MAXNAMELEN - 1);
	if (pdir->f_name[MAXNAMELEN - 1] != 0) {
		fprintf(stderr, "file name is too long: %s\n", path);
		// File already created, no way back from here.
		exit(1);
	}
	pdir->f_type = FTYPE_DIR;
	// 遍历宿主机上该路径下所有文件
	// 如果是目录则递归执行 write_directory 函数
	// 如果是普通文件则执行 write_file 函数
	// 直到目录下所有文件都被写入到磁盘镜像中
	for (struct dirent *e; (e = readdir(dir)) != NULL;) {
		if (strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0) {
			char *buf = malloc(strlen(path) + strlen(e->d_name) + 2);
			sprintf(buf, "%s/%s", path, e->d_name);
			if (e->d_type == DT_DIR) {
				write_directory(pdir, buf);
			} else {
				write_file(pdir, buf);
			}
			free(buf);
		}
	}
	closedir(dir);
}

int main(int argc, char **argv) {
	// 确保文件控制块结构体大小符合预期
	static_assert(sizeof(struct File) == FILE_STRUCT_SIZE);
	// 初始化磁盘
	init_disk();

	if (argc < 3) {
		fprintf(stderr, "Usage: fsformat <img-file> [files or directories]...\n");
		exit(1);
	}

	// 循环读取命令行参数
	for (int i = 2; i < argc; i++) {
		char *name = argv[i];
		struct stat stat_buf;
		// 调用 stat 函数判断文件类型
		int r = stat(name, &stat_buf);
		assert(r == 0);
		// 将文件内容写入磁盘镜像
		// 可以看到写入目的地都是根目录
		if (S_ISDIR(stat_buf.st_mode)) {
			printf("writing directory '%s' recursively into disk\n", name);
			// 递归将目录内文件写入磁盘镜像
			write_directory(&super.s_root, name);
		} else if (S_ISREG(stat_buf.st_mode)) {
			printf("writing regular file '%s' into disk\n", name);
			// 将普通文件写入磁盘镜像
			write_file(&super.s_root, name);
		} else {
			fprintf(stderr, "'%s' has illegal file mode %o\n", name, stat_buf.st_mode);
			exit(2);
		}
	}

	// 刷新位图
	flush_bitmap();
	// 根据 disk 生成磁盘镜像文件
	finish_fs(argv[1]);

	// 完成镜像文件的创建
	return 0;
}
