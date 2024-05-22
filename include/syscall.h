#ifndef SYSCALL_H
#define SYSCALL_H

#ifndef __ASSEMBLER__

/* 用于 kern/syscall_all.c 中的系统调用向量表 */
/* if you need to add other syscall */
/* remember to add Sys_ into this enum */
enum {
	SYS_putchar,
	SYS_print_cons,
	SYS_getenvid,
	SYS_yield,
	SYS_env_destroy,
	SYS_set_tlb_mod_entry,
	SYS_mem_alloc,
	SYS_mem_map,
	SYS_mem_unmap,
	SYS_exofork,
	SYS_set_env_status,
	SYS_set_trapframe,
	SYS_panic,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_cgetc,
	SYS_write_dev,
	SYS_read_dev,
	SYS_clone,
	MAX_SYSNO,
};

#endif

#endif
