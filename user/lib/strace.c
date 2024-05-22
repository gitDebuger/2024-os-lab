#include <lib.h>

void strace_barrier(u_int env_id) {
	int straced_bak = straced;
	straced = 0;
	while (envs[ENVX(env_id)].env_status == ENV_RUNNABLE) {
		syscall_yield();
	}
	straced = straced_bak;
}

void strace_send(int sysno) {
	if (!((SYS_putchar <= sysno && sysno <= SYS_set_tlb_mod_entry) ||
	      (SYS_exofork <= sysno && sysno <= SYS_panic)) ||
	    sysno == SYS_set_trapframe) {
		return;
	}

	// Your code here. (1/2)
	if (straced != 0) {
		straced = 0;
		struct Env *child = envs + ENVX(syscall_getenvid());
		u_int parent = child->env_parent_id;
		ipc_send(parent, sysno, NULL, 0);
		syscall_set_env_status(0, ENV_NOT_RUNNABLE);
		straced = 1;
	}
}

void strace_recv() {
	// Your code here. (2/2)
	int flag = 1;
	u_int child;
	int sysno;
	while (flag) {
		if ((sysno = ipc_recv(&child, NULL, NULL)) == SYS_env_destroy) {
			flag = 0;
		}
		strace_barrier(child);
		recv_sysno(child, sysno);
		syscall_set_env_status(child, ENV_RUNNABLE);
	}
}
