#include <fs.h>
#include <lib.h>

#define debug 0

static int file_close(struct Fd *fd);
static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset);
static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset);
static int file_stat(struct Fd *fd, struct Stat *stat);

// Dot represents choosing the member within the struct declaration
// to initialize, with no need to consider the order of members.

/* 文件系统设备 */
struct Dev devfile = {
    .dev_id = 'f',
    .dev_name = "file",
    .dev_read = file_read,
    .dev_write = file_write,
    .dev_close = file_close,
    .dev_stat = file_stat,
};

// Overview:
//  Open a file (or directory).
//
// Returns:
//  the file descriptor on success,
//  the underlying error on failure.
// 打开文件或目录
// 成功返回文件描述符否则返回错误码
int open(const char *path, int mode) {
	int r;

	// Step 1: Alloc a new 'Fd' using 'fd_alloc' in fd.c.
	// Hint: return the error code if failed.
	// 使用 fd_alloc 申请文件描述符
	// 如果失败返回错误码
	struct Fd *fd;
	/* Exercise 5.9: Your code here. (1/5) */
	try(fd_alloc(&fd));

	// Step 2: Prepare the 'fd' using 'fsipc_open' in fsipc.c.
	// 使用 fsipc.c 中的 fsipc_open 函数准备 fd 变量
	/* Exercise 5.9: Your code here. (2/5) */
	try(fsipc_open(path, mode, fd));

	// Step 3: Set 'va' to the address of the page where the 'fd''s data is cached, using
	// 'fd2data'. Set 'size' and 'fileid' correctly with the value in 'fd' as a 'Filefd'.
	// 虽然我们获得了服务进程共享给用户进程的文件描述符
	// 可文件的内容还没有被一同共享过来
	// 还需要使用 fsipc_map 进行映射
	char *va;
	struct Filefd *ffd;
	u_int size, fileid;
	/* Exercise 5.9: Your code here. (3/5) */
	// 通过 fd2data 获取文件内容应该映射到的地址
	// 该函数为不同的文件描述符提供不同的地址用于映射
	va = fd2data(fd);
	ffd = (struct Filefd *)fd;
	size = ffd->f_file.f_size;
	fileid = ffd->f_fileid;

	// Step 4: Map the file content using 'fsipc_map'.
	for (int i = 0; i < size; i += PTMAP) {
		/* Exercise 5.9: Your code here. (4/5) */
		// 接着我们将文件所有的内容都从磁盘中映射到内存
		// 注意这里要映射到的地址为 va + i 而非 va
		// 使用后者在 Lab6 中加载更大文件时会出现 bug
		try(fsipc_map(fileid, i, va + i));
	}

	// Step 5: Return the number of file descriptor using 'fd2num'.
	/* Exercise 5.9: Your code here. (5/5) */
	// 使用 fd2num 方法获取文件描述符在文件描述符数组中的索引
	return fd2num(fd);
	// 所以说我们在Linux中使用的文件操作系列函数时使用的整型文件描述符
	// 其本质是结构体型文件描述符在文件描述符数组中的索引
}

// Overview:
//  Close a file descriptor
/* 关闭文件描述符 */
int file_close(struct Fd *fd) {
	int r;
	struct Filefd *ffd;
	void *va;
	u_int size, fileid;
	u_int i;

	ffd = (struct Filefd *)fd;
	fileid = ffd->f_fileid;
	size = ffd->f_file.f_size;

	// Set the start address storing the file's content.
	va = fd2data(fd);

	// Tell the file server the dirty page.
	for (i = 0; i < size; i += PTMAP) {
		if ((r = fsipc_dirty(fileid, i)) < 0) {
			debugf("cannot mark pages as dirty\n");
			return r;
		}
	}

	// Request the file server to close the file with fsipc.
	if ((r = fsipc_close(fileid)) < 0) {
		debugf("cannot close the file\n");
		return r;
	}

	// Unmap the content of file, release memory.
	if (size == 0) {
		return 0;
	}
	for (i = 0; i < size; i += PTMAP) {
		if ((r = syscall_mem_unmap(0, (void *)(va + i))) < 0) {
			debugf("cannont unmap the file\n");
			return r;
		}
	}
	return 0;
}

// Overview:
//  Read 'n' bytes from 'fd' at the current seek position into 'buf'. Since files
//  are memory-mapped, this amounts to a memcpy() surrounded by a little red
//  tape to handle the file size and seek pointer.
// 读取被映射到内存中的文件内容
static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset) {
	u_int size;
	struct Filefd *f;
	f = (struct Filefd *)fd;

	// Avoid reading past the end of file.
	size = f->f_file.f_size;

	if (offset > size) {
		return 0;
	}

	if (offset + n > size) {
		n = size - offset;
	}

	memcpy(buf, (char *)fd2data(fd) + offset, n);
	return n;
}

// Overview:
//  Find the virtual address of the page that maps the file block
//  starting at 'offset'.
/* 查找映射从 offset 开始的文件块的页面的虚拟地址 */
int read_map(int fdnum, u_int offset, void **blk) {
	int r;
	void *va;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	if (fd->fd_dev_id != devfile.dev_id) {
		return -E_INVAL;
	}

	va = fd2data(fd) + offset;

	if (offset >= MAXFILESIZE) {
		return -E_NO_DISK;
	}

	if (!(vpd[PDX(va)] & PTE_V) || !(vpt[VPN(va)] & PTE_V)) {
		return -E_NO_DISK;
	}

	*blk = (void *)va;
	return 0;
}

// Overview:
//  Write 'n' bytes from 'buf' to 'fd' at the current seek position.
/* 从缓冲区向 fd 中文件指针指向的位置写 n 字节 */
static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset) {
	int r;
	u_int tot;
	struct Filefd *f;

	f = (struct Filefd *)fd;

	// Don't write more than the maximum file size.
	tot = offset + n;

	if (tot > MAXFILESIZE) {
		return -E_NO_DISK;
	}
	// Increase the file's size if necessary
	if (tot > f->f_file.f_size) {
		if ((r = ftruncate(fd2num(fd), tot)) < 0) {
			return r;
		}
	}

	// Write the data
	memcpy((char *)fd2data(fd) + offset, buf, n);
	return n;
}

/* 设置 State 中字段的值 */
/* 包括 st_name 设置为文件名 */
/* st_size 设置为文件大小 */
/* st_isdir 表示文件是否为目录 */
static int file_stat(struct Fd *fd, struct Stat *st) {
	struct Filefd *f;

	f = (struct Filefd *)fd;

	strcpy(st->st_name, f->f_file.f_name);
	st->st_size = f->f_file.f_size;
	st->st_isdir = f->f_file.f_type == FTYPE_DIR;
	return 0;
}

// Overview:
//  Truncate or extend an open file to 'size' bytes
/* 截断或者扩展一个打开的文件到 size 字节大小 */
int ftruncate(int fdnum, u_int size) {
	int i, r;
	struct Fd *fd;
	struct Filefd *f;
	u_int oldsize, fileid;

	if (size > MAXFILESIZE) {
		return -E_NO_DISK;
	}

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	if (fd->fd_dev_id != devfile.dev_id) {
		return -E_INVAL;
	}

	f = (struct Filefd *)fd;
	fileid = f->f_fileid;
	oldsize = f->f_file.f_size;
	f->f_file.f_size = size;

	if ((r = fsipc_set_size(fileid, size)) < 0) {
		return r;
	}

	void *va = fd2data(fd);

	// Map any new pages needed if extending the file
	for (i = ROUND(oldsize, PTMAP); i < ROUND(size, PTMAP); i += PTMAP) {
		if ((r = fsipc_map(fileid, i, va + i)) < 0) {
			int _r = fsipc_set_size(fileid, oldsize);
			if (_r < 0) {
				return _r;
			}
			return r;
		}
	}

	// Unmap pages if truncating the file
	for (i = ROUND(size, PTMAP); i < ROUND(oldsize, PTMAP); i += PTMAP) {
		if ((r = syscall_mem_unmap(0, (void *)(va + i))) < 0) {
			user_panic("ftruncate: syscall_mem_unmap %08x: %d\n", va + i, r);
		}
	}

	return 0;
}

// Overview:
//  Delete a file or directory.
/* 删除一个文件或目录 */
int remove(const char *path) {
	// Call fsipc_remove.

	/* Exercise 5.13: Your code here. */
	return fsipc_remove(path);
}

// Overview:
//  Synchronize disk with buffer cache
/* 将磁盘与缓冲区同步 */
int sync(void) {
	return fsipc_sync();
}
