# 大端EB小端EL
ENDIAN         := EL

# 使用QEMU运行项目
QEMU           := qemu-system-mipsel
# 交叉编译器前缀
CROSS_COMPILE  := mips-linux-gnu-
# 交叉编译器 mips-linux-gnu-gcc
CC             := $(CROSS_COMPILE)gcc
# 编译器编译选项
CFLAGS         += --std=gnu99 -$(ENDIAN) -G 0 -mno-abicalls -fno-pic \
                  -ffreestanding -fno-stack-protector -fno-builtin \
                  -Wa,-xgot -Wall -mxgot -mno-fix-r4000 -march=4kc
# 交叉链接器 mips-linux-gnu-ld
LD             := $(CROSS_COMPILE)ld
# 链接器链接选项
LDFLAGS        += -$(ENDIAN) -G 0 -static -n -nostdlib --fatal-warnings

# 主机系统上用于编译工具的C编译器和编译选项
# 通常用于在主机系统上构建一些辅助工具
HOST_CC        := cc
HOST_CFLAGS    += --std=gnu99 -O2 -Wall
# 检查主机系统的字节序并将结果存储在变量 HOST_ENDIAN 中
HOST_ENDIAN    := $(shell lscpu | grep -iq 'little endian' && echo EL || echo EB)

# 如果主机系统的字节序与目标系统的字节序不同
# 则添加一个编译选项 CONFIG_REVERSE_ENDIAN
ifneq ($(HOST_ENDIAN), $(ENDIAN))
# CONFIG_REVERSE_ENDIAN 在 tools/fsformat.c (lab5) 中检查
HOST_CFLAGS    += -DCONFIG_REVERSE_ENDIAN
endif
