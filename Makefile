ENDIAN                  := EL

QEMU                    := qemu-system-mipsel
CROSS_COMPILE           := mips-linux-gnu-
CC                      := $(CROSS_COMPILE)gcc
CFLAGS                  += --std=gnu99 -$(ENDIAN) -G 0 -mno-abicalls -fno-pic \
                        -ffreestanding -fno-stack-protector -fno-builtin \
                        -Wa,-xgot -Wall -mxgot -mno-fix-r4000 -march=4kc -g -ggdb
LD                      := $(CROSS_COMPILE)ld
LDFLAGS                 += -EL -G 0 -static -n -nostdlib --fatal-warnings
INCLUDES                := -I./include/
target                  := hello
objects                 := hello.o output.o start.o
qemu_files              += hello
qemu_flags              += -cpu 4Kc -m 64 -nographic -M malta -no-reboot

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

.PHONY: all clean dbg

all:    hello

hello:  linker.lds $(objects)
	$(LD) $(LDFLAGS) -o hello -N -T linker.lds $(objects)

clean:
	rm -rf *~ *.o hello *.objdump

dbg_run: qemu_flags += -s -S
dbg_run: run

dbg:
	make dbg_run >/dev/null 2>&1 &
	gdb-multiarch $(target) -ex "target remote localhost:1234"
	killall $(QEMU)

run:
	$(QEMU) $(qemu_flags)  -kernel $(qemu_files)

objdump:
	$(CROSS_COMPILE)objdump $(target) -aldS > hello.objdump
