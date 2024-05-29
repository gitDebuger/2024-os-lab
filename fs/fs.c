#include "serv.h"
#include <mmu.h>

struct Super *super;

uint32_t *bitmap;

void file_flush(struct File *);
int block_is_free(u_int);

// Overview:
//  Return the virtual address of this disk block in cache.
// Hint: Use 'DISKMAP' and 'BLOCK_SIZE' to calculate the address.
/* 返回磁盘块在缓存中的虚拟地址 */
/* 这里需要说明一点 */
/* 所有的磁盘块都有且只有唯一的虚拟地址与之对应 */
/* 也就是说不会有虚拟地址对应多个磁盘块 */
void *disk_addr(u_int blockno) {
	/* Exercise 5.6: Your code here. */
	return DISKMAP + blockno * BLOCK_SIZE;
}

// Overview:
//  Check if this virtual address is mapped to a block. (check PTE_V bit)
/* 检查虚拟地址是否映射到磁盘块 */
int va_is_mapped(void *va) {
	return (vpd[PDX(va)] & PTE_V) && (vpt[VPN(va)] & PTE_V);
}

// Overview:
//  Check if this disk block is mapped in cache.
//  Returns the virtual address of the cache page if mapped, 0 otherwise.
// 检查磁盘块是否被映射
void *block_is_mapped(u_int blockno) {
	void *va = disk_addr(blockno);
	// 通过检查虚拟地址是否被映射来判断磁盘块是否被映射
	if (va_is_mapped(va)) {
		return va;
	}
	return NULL;
}

// Overview:
//  Check if this virtual address is dirty. (check PTE_DIRTY bit)
/* 通过检查页表的脏位以检查虚拟地址是否被修改 */
int va_is_dirty(void *va) {
	return vpt[VPN(va)] & PTE_DIRTY;
}

// Overview:
//  Check if this block is dirty. (check corresponding `va`)
/* 检查磁盘块是否被修改 */
/* 检查磁盘块是否被映射且虚拟地址是否被修改 */
int block_is_dirty(u_int blockno) {
	void *va = disk_addr(blockno);
	return va_is_mapped(va) && va_is_dirty(va);
}

// Overview:
//  Mark this block as dirty (cache page has changed and needs to be written back to disk).
/* 标记磁盘块被修改 */
/* 缓存页已经被修改且需要被写回到磁盘中 */
int dirty_block(u_int blockno) {
	void *va = disk_addr(blockno);

	if (!va_is_mapped(va)) {
		return -E_NOT_FOUND;
	}

	if (va_is_dirty(va)) {
		return 0;
	}

	return syscall_mem_map(0, va, 0, va, PTE_D | PTE_DIRTY);
}

// Overview:
//  Write the current contents of the block out to disk.
/* 将当前块的内容写出到磁盘 */
void write_block(u_int blockno) {
	// Step 1: detect is this block is mapped, if not, can't write it's data to disk.
	/* 检查磁盘块是否被映射 */
	if (!block_is_mapped(blockno)) {
		user_panic("write unmapped block %08x", blockno);
	}

	// Step2: write data to IDE disk. (using ide_write, and the diskno is 0)
	/* 将数据写入到磁盘 */
	void *va = disk_addr(blockno);
	ide_write(0, blockno * SECT2BLK, va, SECT2BLK);
}

// Overview:
//  Make sure a particular disk block is loaded into memory.
//
// Post-Condition:
//  Return 0 on success, or a negative error code on error.
//
//  If blk!=0, set *blk to the address of the block in memory.
//
//  If isnew!=0, set *isnew to 0 if the block was already in memory, or
//  to 1 if the block was loaded off disk to satisfy this request. (Isnew
//  lets callers like file_get_block clear any memory-only fields
//  from the disk blocks when they come in off disk.)
//
// Hint:
//  use disk_addr, block_is_mapped, syscall_mem_alloc, and ide_read.
// 读取对应磁盘块编号的磁盘块数据到内存
int read_block(u_int blockno, void **blk, u_int *isnew) {
	// Step 1: validate blockno. Make file the block to read is within the disk.
	/* 验证 blockno 确保将要读取的文件块位于磁盘内 */
	/* 也就是说这里检查越界错误 */
	if (super && blockno >= super->s_nblocks) {
		user_panic("reading non-existent block %08x\n", blockno);
	}

	// Step 2: validate this block is used, not free.
	// Hint:
	//  If the bitmap is NULL, indicate that we haven't read bitmap from disk to memory
	//  until now. So, before we check if a block is free using `block_is_free`, we must
	//  ensure that the bitmap blocks are already read from the disk to memory.
	/* 验证磁盘块已经被使用而不是空闲 */
	if (bitmap && block_is_free(blockno)) {
		user_panic("reading free block %08x\n", blockno);
	}

	// Step 3: transform block number to corresponding virtual address.
	// 获取磁盘块对应的虚拟地址
	void *va = disk_addr(blockno);

	// Step 4: read disk and set *isnew.
	/* 读磁盘并设置 *isnew 的值 */
	// Hint:
	//  If this block is already mapped, just set *isnew, else alloc memory and
	//  read data from IDE disk (use `syscall_mem_alloc` and `ide_read`).
	//  We have only one IDE disk, so the diskno of ide_read should be 0.
	// 判断磁盘块是否已经被映射
	if (block_is_mapped(blockno)) { // the block is in memory
		if (isnew) {
			*isnew = 0;
		}
	} else { // the block is not in memory
		// 如果磁盘块没有被映射
		if (isnew) {
			*isnew = 1;
		}
		// 如果没有映射
		// 则分配内存
		try(syscall_mem_alloc(0, va, PTE_D));
		// 并将磁盘中的数据读入该内存空间
		ide_read(0, blockno * SECT2BLK, va, SECT2BLK);
	}

	// Step 5: if blk != NULL, assign 'va' to '*blk'.
	// 返回读取的磁盘块数据对应的内存地址
	if (blk) {
		*blk = va;
	}
	return 0;
}

// Overview:
//  Allocate a page to cache the disk block.
// 申请一个页面用于缓存磁盘块内容
int map_block(u_int blockno) {
	// Step 1: If the block is already mapped in cache, return 0.
	// Hint: Use 'block_is_mapped'.
	/* Exercise 5.7: Your code here. (1/5) */
	if (block_is_mapped(blockno)) {
		return 0;
	}

	// Step 2: Alloc a page in permission 'PTE_D' via syscall.
	// Hint: Use 'disk_addr' for the virtual address.
	/* Exercise 5.7: Your code here. (2/5) */
	try(syscall_mem_alloc(env->env_id, disk_addr(blockno), PTE_D));
}

// Overview:
//  Unmap a disk block in cache.
/* 取消磁盘块在缓存中的映射 */
void unmap_block(u_int blockno) {
	// Step 1: Get the mapped address of the cache page of this block using 'block_is_mapped'.
	void *va;
	/* Exercise 5.7: Your code here. (3/5) */
	/* 获取该磁盘块映射到的缓存页 */
	va = block_is_mapped(blockno);

	// Step 2: If this block is used (not free) and dirty in cache, write it back to the disk
	// first.
	// Hint: Use 'block_is_free', 'block_is_dirty' to check, and 'write_block' to sync.
	/* Exercise 5.7: Your code here. (4/5) */
	/* 如果磁盘块不是空闲且已经被修改 */
	if (!block_is_free(blockno) && block_is_dirty(blockno)) {
		// 将内存中对磁盘块的修改写回磁盘
		write_block(blockno);
	}

	// Step 3: Unmap the virtual address via syscall.
	/* Exercise 5.7: Your code here. (5/5) */
	/* 取消虚拟地址的映射 */
	syscall_mem_unmap(env->env_id, va);

	/* 断言取消映射是否成功 */
	user_assert(!block_is_mapped(blockno));
}

// Overview:
//  Check if the block 'blockno' is free via bitmap.
//
// Post-Condition:
//  Return 1 if the block is free, else 0.
/* 使用位视图检查磁盘块是否空闲 */
int block_is_free(u_int blockno) {
	if (super == 0 || blockno >= super->s_nblocks) {
		return 0;
	}

	if (bitmap[blockno / 32] & (1 << (blockno % 32))) {
		return 1;
	}

	return 0;
}

// Overview:
//  Mark a block as free in the bitmap.
/* 在位视图中标记磁盘块空闲 */
void free_block(u_int blockno) {
	// You can refer to the function 'block_is_free' above.
	// Step 1: If 'blockno' is invalid (0 or >= the number of blocks in 'super'), return.
	/* Exercise 5.4: Your code here. (1/2) */
	if (super == 0 || blockno == 0 || blockno >= super->s_nblocks) {
		return;
	}

	// Step 2: Set the flag bit of 'blockno' in 'bitmap'.
	// Hint: Use bit operations to update the bitmap, such as b[n / W] |= 1 << (n % W).
	/* Exercise 5.4: Your code here. (2/2) */
	bitmap[blockno / 32] |= 1 << (blockno % 32);
}

// Overview:
//  Search in the bitmap for a free block and allocate it.
//
// Post-Condition:
//  Return block number allocated on success,
//  Return -E_NO_DISK if we are out of blocks.
/* 寻找空闲磁盘块并分配 */
int alloc_block_num(void) {
	int blockno;
	// walk through this bitmap, find a free one and mark it as used, then sync
	// this block to IDE disk (using `write_block`) from memory.
	/* 遍历位视图寻找空闲磁盘块并将其标记为已使用 */
	/* 然后将位视图的内容从内存同步到 IDE 磁盘 */
	for (blockno = 3; blockno < super->s_nblocks; blockno++) {
		if (bitmap[blockno / 32] & (1 << (blockno % 32))) { // the block is free
			bitmap[blockno / 32] &= ~(1 << (blockno % 32));
			write_block(blockno / BLOCK_SIZE_BIT + 2); // write to disk.
			return blockno;
		}
	}
	// no free blocks.
	/* 没有空闲磁盘块 */
	return -E_NO_DISK;
}

// Overview:
//  Allocate a block -- first find a free block in the bitmap, then map it into memory.
// 首先调用 alloc_block_num 在磁盘块管理位图上找到空闲的磁盘块
// 更新位图并将位图写入内存
int alloc_block(void) {
	int r, bno;
	// Step 1: find a free block.
	// 首先调用 alloc_block_num 在磁盘块管理位图上找到空闲的磁盘块
	// 更新位图并将位图写入内存
	if ((r = alloc_block_num()) < 0) { // failed.
		return r;
	}
	bno = r;

	// Step 2: map this block into memory.
	// 调用 map_block 将获取的编号所对应的空闲磁盘块从磁盘中读入内存
	if ((r = map_block(bno)) < 0) {
		// 重新将位图对应位置置 1 表示空闲。
		free_block(bno);
		return r;
	}

	// Step 3: return block number.
	// 返回磁盘块编号
	return bno;
}

// Overview:
//  Read and validate the file system super-block.
//
// Post-condition:
//  If error occurred during read super block or validate failed, panic.
// 读取并验证文件系统超级块
// 如果出错直接 panic 即可
void read_super(void) {
	int r;
	void *blk;

	// Step 1: read super block.
	// 读取超级块
	if ((r = read_block(1, &blk, 0)) < 0) {
		user_panic("cannot read superblock: %d", r);
	}

	// 将第一个磁盘块的内容赋值给全局变量
	// 也就是说现在可以通过全局变量 super 读取超级块了
	super = blk;

	// Step 2: Check fs magic nunber.
	// 检查超级块魔数
	if (super->s_magic != FS_MAGIC) {
		user_panic("bad file system magic number %x %x", super->s_magic, FS_MAGIC);
	}

	// Step 3: validate disk size.
	// 校验磁盘大小
	if (super->s_nblocks > DISKMAX / BLOCK_SIZE) {
		user_panic("file system is too large");
	}

	debugf("superblock is good\n");
}

// Overview:
//  Read and validate the file system bitmap.
//
// Hint:
//  Read all the bitmap blocks into memory.
//  Set the 'bitmap' to point to the first bitmap block.
//  For each block i, user_assert(!block_is_free(i))) to check that they're all marked as in use.
// 读取并检查磁盘块分配位图是否有效
void read_bitmap(void) {
	u_int i;
	void *blk = NULL;

	// Step 1: Calculate the number of the bitmap blocks, and read them into memory.
	// 对于 super->s_nblocks 是 BLOCK_SIZE_BIT 整倍数的情况会多计算一个磁盘块
	// 可能会有一点小问题
	// 但是问题不会很大
	// 因为各个磁盘块数据在内存中的空间并不重合
	u_int nbitmap = super->s_nblocks / BLOCK_SIZE_BIT + 1;
	for (i = 0; i < nbitmap; i++) {
		// 将磁盘中的数据读取到内存缓冲区
		read_block(i + 2, blk, 0);
	}

	// 设定全局变量 bitmap 的值为位图的首地址
	bitmap = disk_addr(2);

	// Step 2: Make sure the reserved and root blocks are marked in-use.
	// Hint: use `block_is_free`
	/* 确保保留块和根磁盘块标记为已使用 */
	user_assert(!block_is_free(0));
	user_assert(!block_is_free(1));

	// Step 3: Make sure all bitmap blocks are marked in-use.
	/* 确保所有的存储位视图的磁盘块标记为已使用 */
	for (i = 0; i < nbitmap; i++) {
		user_assert(!block_is_free(i + 2));
	}

	debugf("read_bitmap is good\n");
}

// Overview:
//  Test that write_block works, by smashing the superblock and reading it back.
/* 一段测试代码 */
void check_write_block(void) {
	super = 0;

	// backup the super block.
	// copy the data in super block to the first block on the disk.
	panic_on(read_block(0, 0, 0));
	memcpy((char *)disk_addr(0), (char *)disk_addr(1), BLOCK_SIZE);

	// smash it
	strcpy((char *)disk_addr(1), "OOPS!\n");
	write_block(1);
	user_assert(block_is_mapped(1));

	// clear it out
	panic_on(syscall_mem_unmap(0, disk_addr(1)));
	user_assert(!block_is_mapped(1));

	// validate the data read from the disk.
	panic_on(read_block(1, 0, 0));
	user_assert(strcmp((char *)disk_addr(1), "OOPS!\n") == 0);

	// restore the super block.
	memcpy((char *)disk_addr(1), (char *)disk_addr(0), BLOCK_SIZE);
	write_block(1);
	super = (struct Super *)disk_addr(1);
}

// Overview:
//  Initialize the file system.
// Hint:
//  1. read super block.
//  2. check if the disk can work.
//  3. read bitmap blocks from disk to memory.
// 初始化文件系统
void fs_init(void) {
	// 读取超级块
	// 主要功能是检查
	read_super();
	// 插入到基本流程中的测试代码
	check_write_block();
	// 将管理磁盘块分配的位图读取到内存中
	read_bitmap();
}

// Overview:
//  Like pgdir_walk but for files.
//  Find the disk block number slot for the 'filebno'th block in file 'f'. Then, set
//  '*ppdiskbno' to point to that slot. The slot will be one of the f->f_direct[] entries,
//  or an entry in the indirect block.
//  When 'alloc' is set, this function will allocate an indirect block if necessary.
//
// Post-Condition:
//  Return 0 on success, and set *ppdiskbno to the pointer to the target block.
//  Return -E_NOT_FOUND if the function needed to allocate an indirect block, but alloc was 0.
//  Return -E_NO_DISK if there's no space on the disk for an indirect block.
//  Return -E_NO_MEM if there's not enough memory for an indirect block.
//  Return -E_INVAL if filebno is out of range (>= NINDIRECT).
int file_block_walk(struct File *f, u_int filebno, uint32_t **ppdiskbno, u_int alloc) {
	int r;
	uint32_t *ptr;
	uint32_t *blk;

	/* 根据文件中磁盘块编号找到总磁盘块编号 */
	if (filebno < NDIRECT) {
		// Step 1: if the target block is corresponded to a direct pointer, just return the
		// disk block number.
		ptr = &f->f_direct[filebno];
	} else if (filebno < NINDIRECT) {
		// Step 2: if the target block is corresponded to the indirect block, but there's no
		//  indirect block and `alloc` is set, create the indirect block.
		if (f->f_indirect == 0) {
			if (alloc == 0) {
				return -E_NOT_FOUND;
			}

			if ((r = alloc_block()) < 0) {
				return r;
			}
			f->f_indirect = r;
		}

		// Step 3: read the new indirect block to memory.
		// 使用 read_block 将该磁盘块数据读入内存中
		if ((r = read_block(f->f_indirect, (void **)&blk, 0)) < 0) {
			return r;
		}
		/* 这里使用 blk + filebno 而不是 blk + filebno - 10 */
		/* 因为页面的前十个 u_int 保留不用从而方便计算 */
		ptr = blk + filebno;
	} else {
		return -E_INVAL;
	}

	// Step 4: store the result into *ppdiskbno, and return 0.
	// 将结果存入 *ppdiskbno 中并返回
	*ppdiskbno = ptr;
	return 0;
}

// OVerview:
//  Set *diskbno to the disk block number for the filebno'th block in file f.
//  If alloc is set and the block does not exist, allocate it.
//
// Post-Condition:
//  Returns 0: success, < 0 on error.
//  Errors are:
//   -E_NOT_FOUND: alloc was 0 but the block did not exist.
//   -E_NO_DISK: if a block needed to be allocated but the disk is full.
//   -E_NO_MEM: if we're out of memory.
//   -E_INVAL: if filebno is out of range.
/* 寻找文件中的第 filebno 个磁盘块映射到的磁盘块 */
/* 如果允许创建则创建之 */
int file_map_block(struct File *f, u_int filebno, u_int *diskbno, u_int alloc) {
	int r;
	uint32_t *ptr;

	// Step 1: find the pointer for the target block.
	// 首先调用 file_block_walk 获取对应的磁盘块编号
	if ((r = file_block_walk(f, filebno, &ptr, alloc)) < 0) {
		return r;
	}
	// 文件的第 filebno 个磁盘块对应的磁盘块编号现在已经被存储在了 *ptr

	// Step 2: if the block not exists, and create is set, alloc one.
	// 考虑未找到且允许创建时再调用 alloc_block 申请一个磁盘块的情况
	if (*ptr == 0) {
		if (alloc == 0) {
			return -E_NOT_FOUND;
		}

		if ((r = alloc_block()) < 0) {
			return r;
		}
		*ptr = r;
	}

	// Step 3: set the pointer to the block in *diskbno and return 0.
	// 最后将文件的第 filebno 个磁盘块对应的磁盘块编号传给 *diskbno
	*diskbno = *ptr;
	return 0;
}

// Overview:
//  Remove a block from file f. If it's not there, just silently succeed.
/* 从文件中移除一个磁盘块 */
/* 如果不存在映射的磁盘块则默认成功 */
int file_clear_block(struct File *f, u_int filebno) {
	int r;
	uint32_t *ptr;

	if ((r = file_block_walk(f, filebno, &ptr, 0)) < 0) {
		return r;
	}

	if (*ptr) {
		free_block(*ptr);
		*ptr = 0;
	}

	return 0;
}

// Overview:
//  Set *blk to point at the filebno'th block in file f.
//
// Hint: use file_map_block and read_block.
//
// Post-Condition:
//  return 0 on success, and read the data to `blk`, return <0 on error.
// 用于获取文件中第 filebno 个磁盘块
int file_get_block(struct File *f, u_int filebno, void **blk) {
	int r;
	u_int diskbno;
	u_int isnew;

	// Step 1: find the disk block number is `f` using `file_map_block`.
	// 首先调用 file_map_block 获取了文件中使用的第 filebno 个磁盘块对应的磁盘块编号
	if ((r = file_map_block(f, filebno, &diskbno, 1)) < 0) {
		return r;
	}

	// Step 2: read the data in this disk to blk.
	// 调用 read_block 将该磁盘块的内容从磁盘中读取到内存
	if ((r = read_block(diskbno, blk, &isnew)) < 0) {
		return r;
	}
	return 0;
}

// Overview:
//  Mark the offset/BLOCK_SIZE'th block dirty in file f.
/* 标记文件偏移量为 offset 处对应的磁盘块已修改 */
int file_dirty(struct File *f, u_int offset) {
	int r;
	u_int diskbno;

	if ((r = file_map_block(f, offset / BLOCK_SIZE, &diskbno, 0)) < 0) {
		return r;
	}

	return dirty_block(diskbno);
}

// Overview:
//  Find a file named 'name' in the directory 'dir'. If found, set *file to it.
//
// Post-Condition:
//  Return 0 on success, and set the pointer to the target file in `*file`.
//  Return the underlying error if an error occurs.
// 获取文件的所有磁盘块
// 遍历其中所有的文件控制块
// 返回指定名字的文件对应的文件控制块
int dir_lookup(struct File *dir, char *name, struct File **file) {
	// Step 1: Calculate the number of blocks in 'dir' via its size.
	u_int nblock;
	/* Exercise 5.8: Your code here. (1/3) */
	nblock = dir->f_size / BLOCK_SIZE;

	// Step 2: Iterate through all blocks in the directory.
	for (int i = 0; i < nblock; i++) {
		// Read the i'th block of 'dir' and get its address in 'blk' using 'file_get_block'.
		void *blk;
		/* Exercise 5.8: Your code here. (2/3) */
		// 获取文件中第 i 个磁盘块
		try(file_get_block(dir, i, &blk));

		struct File *files = (struct File *)blk;

		// Find the target among all 'File's in this block.
		/* 在这一磁盘块的所有 File 结构体中寻找匹配的磁盘块 */
		for (struct File *f = files; f < files + FILE2BLK; ++f) {
			// Compare the file name against 'name' using 'strcmp'.
			// If we find the target file, set '*file' to it and set up its 'f_dir'
			// field.
			/* Exercise 5.8: Your code here. (3/3) */
			if (strcmp(name, f->f_name) == 0) {
				*file = f;
				f->f_dir = dir;
				return 0;
			}
		}
	}

	return -E_NOT_FOUND;
}

// Overview:
//  Alloc a new File structure under specified directory. Set *file
//  to point at a free File structure in dir.
/* 在特定目录下申请新的 File 结构体 */
int dir_alloc_file(struct File *dir, struct File **file) {
	int r;
	u_int nblock, i, j;
	void *blk;
	struct File *f;

	nblock = dir->f_size / BLOCK_SIZE;

	for (i = 0; i < nblock; i++) {
		// read the block.
		if ((r = file_get_block(dir, i, &blk)) < 0) {
			return r;
		}

		f = blk;

		for (j = 0; j < FILE2BLK; j++) {
			/* 找到空闲的 File 结构体  */
			if (f[j].f_name[0] == '\0') { // found free File structure.
				*file = &f[j];
				return 0;
			}
		}
	}

	// no free File structure in exists data block.
	// new data block need to be created.
	/* 如果没有空闲的 File 结构体 */
	/* 那么就创建新的数据块用于存储 File 结构体 */
	dir->f_size += BLOCK_SIZE;
	if ((r = file_get_block(dir, i, &blk)) < 0) {
		return r;
	}
	/* 这块就是新的数据块的地址和新的 File 结构体地址相同 */
	f = blk;
	/* 这个也很奇怪 */
	/* 为什么不直接写 *file = f 呢 */
	*file = &f[0];

	return 0;
}

// Overview:
//  Skip over slashes.
/* 跳过文件路径分隔符 */
char *skip_slash(char *p) {
	while (*p == '/') {
		p++;
	}
	return p;
}

// Overview:
//  Evaluate a path name, starting at the root.
//
// Post-Condition:
//  On success, set *pfile to the file we found and set *pdir to the directory
//  the file is in.
//  If we cannot find the file but find the directory it should be in, set
//  *pdir and copy the final path element into lastelem.
// 解析路径并根据路径不断找到目录下的文件
// 找到最后得到的就是路径对应的文件的文件控制块
int walk_path(char *path, struct File **pdir, struct File **pfile, char *lastelem) {
	char *p;
	char name[MAXNAMELEN];
	struct File *dir, *file;
	int r;

	// start at the root.
	path = skip_slash(path);
	file = &super->s_root;
	dir = 0;
	name[0] = 0;

	if (pdir) {
		*pdir = 0;
	}

	*pfile = 0;

	// find the target file by name recursively.
	while (*path != '\0') {
		dir = file;
		p = path;

		while (*path != '/' && *path != '\0') {
			path++;
		}

		if (path - p >= MAXNAMELEN) {
			return -E_BAD_PATH;
		}

		memcpy(name, p, path - p);
		name[path - p] = '\0';
		path = skip_slash(path);
		if (dir->f_type != FTYPE_DIR) {
			return -E_NOT_FOUND;
		}

		// 找到指定目录下的指定名字的文件
		if ((r = dir_lookup(dir, name, &file)) < 0) {
			if (r == -E_NOT_FOUND && *path == '\0') {
				if (pdir) {
					*pdir = dir;
				}

				/* 保存最后一个路径节点 */
				/* 比如 /root/dev/www/index.html */
				/* 如果当前是在 dev 下寻找 www */
				/* 则保存 www 到 lastelem 中 */
				if (lastelem) {
					strcpy(lastelem, name);
				}

				*pfile = 0;
			}

			return r;
		}
	}

	if (pdir) {
		/* 保存要寻找的文件所在目录 */
		*pdir = dir;
	}

	/* 保存要寻找的文件的 File 结构体 */
	*pfile = file;
	return 0;
}

// Overview:
//  Open "path".
//
// Post-Condition:
//  On success set *pfile to point at the file and return 0.
//  On error return < 0.
/* 打开 path 路径指定的文件 */
/* 不保存文件所在目录 */
/* 不保存最后一个路径节点 */
int file_open(char *path, struct File **file) {
	return walk_path(path, 0, file, 0);
}

// Overview:
//  Create "path".
//
// Post-Condition:
//  On success set *file to point at the file and return 0.
//  On error return < 0.
/* 创建文件 */
/* 注意这个函数没有递归创建目录的能力 */
int file_create(char *path, struct File **file) {
	char name[MAXNAMELEN];
	int r;
	struct File *dir, *f;

	/* 检查是否存在同名文件 */
	if ((r = walk_path(path, &dir, &f, name)) == 0) {
		return -E_FILE_EXISTS;
	}

	/* 如果不是未找到文件或者 dir 没有赋值成功 */
	/* 这个检查是不是不太充分？ */
	if (r != -E_NOT_FOUND || dir == 0) {
		return r;
	}

	/* 在路径下申请文件结构体 */
	if (dir_alloc_file(dir, &f) < 0) {
		return r;
	}

	/* 拷贝文件名到 File 结构体并返回 */
	strcpy(f->f_name, name);
	*file = f;
	return 0;
}

// Overview:
//  Truncate file down to newsize bytes.
//
//  Since the file is shorter, we can free the blocks that were used by the old
//  bigger version but not by our new smaller self. For both the old and new sizes,
//  figure out the number of blocks required, and then clear the blocks from
//  new_nblocks to old_nblocks.
//
//  If the new_nblocks is no more than NDIRECT, free the indirect block too.
//  (Remember to clear the f->f_indirect pointer so you'll know whether it's valid!)
//
// Hint: use file_clear_block.
/* 将文件截断到 newsize 字节 */
void file_truncate(struct File *f, u_int newsize) {
	u_int bno, old_nblocks, new_nblocks;

	old_nblocks = ROUND(f->f_size, BLOCK_SIZE) / BLOCK_SIZE;
	new_nblocks = ROUND(newsize, BLOCK_SIZE) / BLOCK_SIZE;

	if (newsize == 0) {
		new_nblocks = 0;
	}

	if (new_nblocks <= NDIRECT) {
		for (bno = new_nblocks; bno < old_nblocks; bno++) {
			/* 移除多余的磁盘块 */
			panic_on(file_clear_block(f, bno));
		}
		if (f->f_indirect) {
			/* 移除间接指针指向的磁盘块 */
			free_block(f->f_indirect);
			f->f_indirect = 0;
		}
	} else {
		for (bno = new_nblocks; bno < old_nblocks; bno++) {
			panic_on(file_clear_block(f, bno));
		}
	}
	/* 更新文件大小 */
	f->f_size = newsize;
}

// Overview:
//  Set file size to newsize.
/* 将文件大小设置为 newsize */
int file_set_size(struct File *f, u_int newsize) {
	if (f->f_size > newsize) {
		/* 调用截断函数 */
		file_truncate(f, newsize);
	}

	f->f_size = newsize;

	if (f->f_dir) {
		/* 刷新文件所在目录 */
		/* WHY flush the directory where the file is ? */
		file_flush(f->f_dir);
	}

	return 0;
}

// Overview:
//  Flush the contents of file f out to disk.
//  Loop over all the blocks in file.
//  Translate the file block number into a disk block number and then
//  check whether that disk block is dirty. If so, write it out.
//
// Hint: use file_map_block, block_is_dirty, and write_block.
/* 将文件内容刷新到磁盘 */
void file_flush(struct File *f) {
	u_int nblocks;
	u_int bno;
	u_int diskno;
	int r;

	nblocks = ROUND(f->f_size, BLOCK_SIZE) / BLOCK_SIZE;

	for (bno = 0; bno < nblocks; bno++) {
		if ((r = file_map_block(f, bno, &diskno, 0)) < 0) {
			continue;
		}
		/* 如果对应磁盘块已经修改则将其刷新到磁盘 */
		if (block_is_dirty(diskno)) {
			write_block(diskno);
		}
	}
}

// Overview:
//  Sync the entire file system.  A big hammer.
/* 同步整个文件系统 */
/* 非常巨量的操作 */
void fs_sync(void) {
	int i;
	for (i = 0; i < super->s_nblocks; i++) {
		if (block_is_dirty(i)) {
			write_block(i);
		}
	}
}

// Overview:
//  Close a file.
/* 关闭文件 */
void file_close(struct File *f) {
	// Flush the file itself, if f's f_dir is set, flush it's f_dir.
	/* 刷新文件 */
	file_flush(f);
	/* 刷新文件目录 */
	if (f->f_dir) {
		file_flush(f->f_dir);
	}
}

// Overview:
//  Remove a file by truncating it and then zeroing the name.
/* 移除文件并将其文件名清零 */
/* 看起来这个函数同样不具备递归删除的功能 */
int file_remove(char *path) {
	int r;
	struct File *f;

	// Step 1: find the file on the disk.
	/* 找到文件在磁盘中的位置 */
	if ((r = walk_path(path, 0, &f, 0)) < 0) {
		return r;
	}

	// Step 2: truncate it's size to zero.
	/* 将文件截断到零 */
	file_truncate(f, 0);

	// Step 3: clear it's name.
	/* 清空文件名 */
	f->f_name[0] = '\0';

	// Step 4: flush the file.
	/* 刷新文件 */
	file_flush(f);
	/* 刷新文件目录 */
	if (f->f_dir) {
		file_flush(f->f_dir);
	}

	return 0;
}

// lab5-2-exam

int copy_file_content(struct File *src, struct File *dst) {
	void *src_blk, *dst_blk;
	int r;
	int nblock;
	// Calculate the total number of blocks in the source file.
	nblock = ROUND(src->f_size, BLOCK_SIZE) / BLOCK_SIZE;
	for (u_int i = 0; i < nblock; i++) {
		// Lab 5-2-Exam: Your code here. (3/6)
		// debugf("OK");
		try(file_get_block(src, i, &src_blk));
		try(file_get_block(dst, i, &dst_blk));
		// debugf("OK");
		memcpy(dst_blk, src_blk, BLOCK_SIZE);
		file_dirty(dst, i * BLOCK_SIZE);
	}
	// Flush the changes to the destination file
	file_flush(dst);
	return 0;
}

int copy_directory_contents(struct File *src, struct File *dst) {
	struct File *dir_content;
	void *blk;
	int r;
	// Iterate over each block in the source directory
	for (u_int i = 0; i < ROUND(src->f_size, BLOCK_SIZE) / BLOCK_SIZE; i++) {
		if ((r = file_get_block(src, i, &blk)) < 0) {
			return r;
		}
		dir_content = (struct File *)blk;
		for (int j = 0; j < FILE2BLK; j++) {
			if (dir_content[j].f_name[0] == '\0') continue;
			struct File *dst_file;
			// Step1: Alloc dst_file using 'dir_alloc_file'
			// Lab 5-2-Exam: Your code here. (4/6)
			try(dir_alloc_file(dst, &dst_file));
			// Step2: Assign corresponding values of 'f_name', 'f_dir', 'f_size', 'f_type' to dst_file
			strcpy(dst_file->f_name, dir_content[j].f_name);
			dst_file->f_dir = dst;
			dst_file->f_size = dir_content[j].f_size;
			dst_file->f_type = dir_content[j].f_type;

			// Step3: Invoke either 'copy_directory_contents' or 'copy_file_content',
			// depending on the value of 'f_type'.
			// Lab 5-2-Exam: Your code here. (5/6)
			if (dst_file->f_type == FTYPE_DIR) {
				try(copy_directory_contents(&dir_content[j], dst_file));
			} else if (dst_file->f_type == FTYPE_REG) {
				try(copy_file_content(&dir_content[j], dst_file));
			}
		}
	}
	file_flush(dst);
	return 0;
}

int directory_copy(char *src_path, char *dst_path) {
	struct File *src_dir, *dst_dir;
	int r;
	if ((r = file_open(src_path, &src_dir)) < 0) {
		return r;
	}
	if ((r = file_create(dst_path, &dst_dir)) < 0) {
		return r;
	}
	dst_dir->f_type = FTYPE_DIR;
	return copy_directory_contents(src_dir, dst_dir);
}
