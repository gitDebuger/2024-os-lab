# 根据不同的实验编号选择性地向 targets 变量添加额外的目标
lab-ge = $(shell [ "$$(echo $(lab)_ | cut -f1 -d_)" -ge $(1) ] && echo true)

# 实验编号大于等于 3 则将 targets 变量添加 bintoc 目标
ifeq ($(call lab-ge,3), true)
	targets  += bintoc
endif

# 实验编号大于等于 5 则将 targets 变量添加 fsformat 目标。
ifeq ($(call lab-ge,5), true)
	targets  += fsformat
endif
