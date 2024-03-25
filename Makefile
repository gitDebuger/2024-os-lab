# 包含外部 Makefile 文件
include include.mk

# 指定实验编号
# 如果当前目录下存在名为 .mos-this-lab 文件则从中读取
# 否则指定实验编号为 6
lab                     ?= $(shell cat .mos-this-lab 2>/dev/null || echo 6)

# 指定目标文件存放目录
target_dir              := target
# 指定了生成的操作系统内核可执行文件的路径
mos_elf                 := $(target_dir)/mos
# 指定了用户磁盘镜像文件和空磁盘镜像文件的路径
user_disk               := $(target_dir)/fs.img
empty_disk              := $(target_dir)/empty.img
# 使用 grep 命令从 .qemu_log 文件中提取 QEMU 的伪终端 PTS 路径
qemu_pts                := $(shell [ -f .qemu_log ] && grep -Eo '/dev/pts/[0-9]+' .qemu_log)
# 指定链接器脚本文件路径
link_script             := kernel.lds

# 指定要编译的内核模块列表
modules                 := lib init kern
# 指定要构建的目标文件列表
targets                 := $(mos_elf)

# 指定实验编号
lab-ge = $(shell [ "$$(echo $(lab)_ | cut -f1 -d_)" -ge $(1) ] && echo true)

# 分支判断根据实验编号动态选择编译哪些模块
ifeq ($(call lab-ge,3),true)
	user_modules    += user/bare
endif

ifeq ($(call lab-ge,4),true)
	user_modules    += user
endif

ifeq ($(call lab-ge,5),true)
	user_modules    += fs
	targets         += fs-image
endif

# 指定要链接的目标文件列表
objects                 := $(addsuffix /*.o, $(modules)) $(addsuffix /*.x, $(user_modules))
# 内核模块列表添加内容
modules                 += $(user_modules)

# 添加编译选项
CFLAGS                  += -DLAB=$(shell echo $(lab) | cut -f1 -d_)
# 添加 QEMU 运行选项
QEMU_FLAGS              += -cpu 4Kc -m 64 -nographic -M malta \
						$(shell [ -f '$(user_disk)' ] && echo '-drive id=ide0,file=$(user_disk),if=ide,format=raw') \
						$(shell [ -f '$(empty_disk)' ] && echo '-drive id=ide1,file=$(empty_disk),if=ide,format=raw') \
						-no-reboot

# 定义一系列伪目标
.PHONY: all test tools $(modules) clean run dbg_run dbg_pts dbg objdump fs-image clean-and-all connect

# 使所有命令在同一个 shell 进程中执行
# 默认情况下每个目标的命令都会在单独的 shell 进程中执行
.ONESHELL:
# 先执行 clean 然后执行 all
clean-and-all: clean
	$(MAKE) all

# 设定 test_dir 变量并导出到后续命令的环境中
test: export test_dir = tests/lab$(lab)
# 指定 test 依赖 clean-and-all
test: clean-and-all

# 导入两个 Makefile 文件设定
include mk/tests.mk mk/profiles.mk
# 导出一些设定
export CC CFLAGS LD LDFLAGS lab

all: $(targets)

$(target_dir):
	mkdir -p $@

tools:
	CC="$(HOST_CC)" CFLAGS="$(HOST_CFLAGS)" $(MAKE) --directory=$@

$(modules): tools
	$(MAKE) --directory=$@

$(mos_elf): $(modules) $(target_dir)
	$(LD) $(LDFLAGS) -o $(mos_elf) -N -T $(link_script) $(objects)

fs-image: $(target_dir) user
	$(MAKE) --directory=fs image fs-files="$(addprefix ../, $(fs-files))"

fs: user
user: lib

clean:
	for d in * tools/readelf user/* tests/*; do
		if [ -f $$d/Makefile ]; then
			$(MAKE) --directory=$$d clean
		fi
	done
	rm -rf *.o *~ $(target_dir) include/generated
	find . -name '*.objdump' -exec rm {} ';'

# 使用 QEMU 运行内核程序
run:
	$(QEMU) $(QEMU_FLAGS) -kernel $(mos_elf)

# 使用调试模式运行内核程序
dbg_run:
	$(QEMU) $(QEMU_FLAGS) -kernel $(mos_elf) -s -S

dbg:
	export QEMU="$(QEMU)"
	export QEMU_FLAGS="$(QEMU_FLAGS)"
	export mos_elf="$(mos_elf)"
	setsid ./tools/run_bg.sh $$$$ &
	exec gdb-multiarch -q $(mos_elf) -ex "target remote localhost:1234"

dbg_pts: QEMU_FLAGS += -serial "pty"
dbg_pts: dbg

connect:
	[ -f .qemu_log ] && screen -R mos $(qemu_pts)

# 生成目标文件的反汇编文件
objdump:
	@find * \( -name '*.b' -o -path $(mos_elf) \) -exec sh -c \
	'$(CROSS_COMPILE)objdump {} -aldS > {}.objdump && echo {}.objdump' ';'
