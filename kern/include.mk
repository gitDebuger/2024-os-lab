# 根据 lab 编号选择性编译
lab-ge = $(shell [ "$$(echo $(lab)_ | cut -f1 -d_)" -ge $(1) ] && echo true)

# 编译当前路径下的 machine.c printk.c panic.c 到 .o 文件
targets             := machine.o printk.o panic.o

# 根据 lab 编号选择性加入其他目标
ifeq ($(call lab-ge,2), true)
	targets     += pmap.o tlb_asm.o tlbex.o
endif

ifeq ($(call lab-ge,3), true)
	targets     += env.o env_asm.o sched.o entry.o genex.o traps.o
endif

ifeq ($(call lab-ge,4), true)
	targets     += syscall_all.o
endif
