/*
 * operations on IDE disk.
 */
/* 此文件是一些关于 IDE 磁盘的操作 */

#include "serv.h"
#include <lib.h>
#include <malta.h>
#include <mmu.h>

/* Overview:
 *   Wait for the IDE device to complete previous requests and be ready
 *   to receive subsequent requests.
 */
/* 等待 IDE 设备完成先前的请求并准备好接收后续请求 */
static uint8_t wait_ide_ready() {
	uint8_t flag;
	while (1) {
		/* 宏 MALTA_IDE_STATUS 表示设备状态 */
		panic_on(syscall_read_dev(&flag, MALTA_IDE_STATUS, 1));
		/* 设备状态的 BUSY 位为 0 说明设备不忙 */
		/* 则退出循环 */
		if ((flag & MALTA_IDE_BUSY) == 0) {
			break;
		}
		/* 否则阻塞 */
		syscall_yield();
	}
	/* 返回设备状态 */
	return flag;
}

/* Overview:
 *  read data from IDE disk. First issue a read request through
 *  disk register and then copy data from disk buffer
 *  (512 bytes, a sector) to destination array.
 *
 * Parameters:
 *  diskno: disk number.
 *  secno: start sector number.
 *  dst: destination for data read from IDE disk.
 *  nsecs: the number of sectors to read.
 *
 * Post-Condition:
 *  Panic if any error occurs. (you may want to use 'panic_on')
 *
 * Hint: Use syscalls to access device registers and buffers.
 * Hint: Use the physical address and offsets defined in 'include/malta.h'.
 */
/* 从 IDE 磁盘设备读取数据 */
/* 首先通过磁盘寄存器发送读取请求 */
/* 然后从磁盘缓冲区将数据拷贝到目标数组中 */
/* 缓冲区大小 512 字节即一个扇区的大小 */
void ide_read(u_int diskno/* 磁盘编号 */, u_int secno/* 开始扇区编号 */, void *dst/* 目标数组 */, u_int nsecs/* 需要读取的扇区数 */) {
	uint8_t temp;
	u_int offset = 0, max = nsecs + secno;
	/* 这里需要注意 diskno < 2 的部分有其他用处 */
	panic_on(diskno >= 2);

	// Read the sector in turn
	// 循环读取扇区
	while (secno < max) {
		/* 等待设备准备就绪 */
		temp = wait_ide_ready();
		// Step 1: Write the number of operating sectors to NSECT register
		/* 将需要操作的扇区数写入 NSECT 寄存器 */
		temp = 1;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_NSECT, 1));

		// Step 2: Write the 7:0 bits of sector number to LBAL register
		/* 将扇区号的 0-7 位写入 LBAL 寄存器 */
		temp = secno & 0xff;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAL, 1));

		// Step 3: Write the 15:8 bits of sector number to LBAM register
		/* 将扇区号的 8-15 位写入 LBAM 寄存器 */
		/* Exercise 5.3: Your code here. (1/9) */
		temp = (secno >> 8) & 0xff;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAM, 1));

		// Step 4: Write the 23:16 bits of sector number to LBAH register
		/* 将扇区号的 16-23 位写入 LBAH 寄存器 */
		/* Exercise 5.3: Your code here. (2/9) */
		temp = (secno >> 16) & 0xff;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAH, 1));

		// Step 5: Write the 27:24 bits of sector number, addressing mode
		/* 将扇区号的 24-27 位写入寄存器 */
		/* 设置寻址模式 */
		/* 设置磁盘编号 */
		// and diskno to DEVICE register
		temp = ((secno >> 24) & 0x0f) | MALTA_IDE_LBA | (diskno << 4);
		panic_on(syscall_write_dev(&temp, MALTA_IDE_DEVICE, 1));

		// Step 6: Write the working mode to STATUS register
		/* 设置工作模式为读模式 */
		temp = MALTA_IDE_CMD_PIO_READ;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_STATUS, 1));

		// Step 7: Wait until the IDE is ready
		/* 等待 IDE 设备准备就绪 */
		temp = wait_ide_ready();

		// Step 8: Read the data from device
		/* 循环从设备中读取数据 */
		for (int i = 0; i < SECT_SIZE / 4; i++) {
			/* 实验中 IDE 磁盘一次只能传输四个字节 */
			/* 所以需要循环读取 */
			panic_on(syscall_read_dev(dst + offset + i * 4, MALTA_IDE_DATA, 4));
		}

		// Step 9: Check IDE status
		/* 检查 IDE 磁盘状态 */
		panic_on(syscall_read_dev(&temp, MALTA_IDE_STATUS, 1));

		/* 偏移量增加一个扇区的大小 */
		offset += SECT_SIZE;
		/* 扇区编号增加一 */
		secno += 1;
	}
}

/* Overview:
 *  write data to IDE disk.
 *
 * Parameters:
 *  diskno: disk number.
 *  secno: start sector number.
 *  src: the source data to write into IDE disk.
 *  nsecs: the number of sectors to write.
 *
 * Post-Condition:
 *  Panic if any error occurs.
 *
 * Hint: Use syscalls to access device registers and buffers.
 * Hint: Use the physical address and offsets defined in 'include/malta.h'.
 */
/* 向 IDE 磁盘写入数据 */
void ide_write(u_int diskno, u_int secno, void *src, u_int nsecs) {
	/* 这部分内容和 ide_read 基本相同 */
	uint8_t temp;
	u_int offset = 0, max = nsecs + secno;
	panic_on(diskno >= 2);

	// Write the sector in turn
	while (secno < max) {
		temp = wait_ide_ready();
		// Step 1: Write the number of operating sectors to NSECT register
		/* Exercise 5.3: Your code here. (3/9) */
		temp = 1;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_NSECT, 1));

		// Step 2: Write the 7:0 bits of sector number to LBAL register
		/* Exercise 5.3: Your code here. (4/9) */
		temp = secno & 0xff;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAL, 1));

		// Step 3: Write the 15:8 bits of sector number to LBAM register
		/* Exercise 5.3: Your code here. (5/9) */
		temp = (secno >> 8) & 0xff;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAM, 1));

		// Step 4: Write the 23:16 bits of sector number to LBAH register
		/* Exercise 5.3: Your code here. (6/9) */
		temp = (secno >> 16) & 0xff;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAH, 1));

		// Step 5: Write the 27:24 bits of sector number, addressing mode
		// and diskno to DEVICE register
		/* Exercise 5.3: Your code here. (7/9) */
		temp = ((secno >> 24) & 0x0f) | MALTA_IDE_LBA | (diskno << 4);
		panic_on(syscall_write_dev(&temp, MALTA_IDE_DEVICE, 1));

		// Step 6: Write the working mode to STATUS register
		/* Exercise 5.3: Your code here. (8/9) */
		/* 第一处不同是这里的工作模式设置为写模式 */
		temp = MALTA_IDE_CMD_PIO_WRITE;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_STATUS, 1));

		// Step 7: Wait until the IDE is ready
		temp = wait_ide_ready();

		// Step 8: Write the data to device
		/* 第二处不同是这里循环向设备写入数据 */
		for (int i = 0; i < SECT_SIZE / 4; i++) {
			/* Exercise 5.3: Your code here. (9/9) */
			panic_on(syscall_write_dev(src + offset + i * 4, MALTA_IDE_DATA, 4));
		}

		// Step 9: Check IDE status
		panic_on(syscall_read_dev(&temp, MALTA_IDE_STATUS, 1));

		offset += SECT_SIZE;
		secno += 1;
	}
}
