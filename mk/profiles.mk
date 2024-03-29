# 设定一些编译和链接选项
RELEASE_CFLAGS   := $(CFLAGS) -O2
RELEASE_LDFLAGS  := $(LDFLAGS) -O --gc-sections
DEBUG_CFLAGS     := $(CFLAGS) -O0 -g -ggdb -DMOS_DEBUG
DEBUG_LDFLAGS    := $(LDFLAGS)

# 分支判断指定编译选项
# 调试模式或者发布模式
ifeq ($(MOS_PROFILE),release)
	CFLAGS   := $(RELEASE_CFLAGS)
	LDFLAGS  := $(RELEASE_LDFLAGS)
else
	CFLAGS   := $(DEBUG_CFLAGS)
endif

# 设定伪目标
.PHONY: debug release

# 调试模式编译选项和链接选项
debug: CFLAGS    := $(DEBUG_CFLAGS)
debug: LDFLAGS   := $(DEBUG_LDFLAGS)
debug: all

# 发布模式编译选项和链接选项
release: CFLAGS  := $(RELEASE_CFLAGS)
release: LDFLAGS := $(RELEASE_LDFLAGS)
release: all
