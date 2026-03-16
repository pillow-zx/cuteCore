COMPILER ?= clang

ifeq ($(COMPILER),clang)
CC = clang
AS = clang
LD = ld.lld
OBJCOPY = llvm-objcopy
OBJDUMP = llvm-objdump
TARGET = riscv64-unknown-elf
else
TOOLPREFIX = riscv64-unknown-elf-
CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
endif

QEMU = qemu-system-riscv64

CFLAGS = -Wall -Werror -Wextra -Wno-unknown-attributes -g -O -fno-omit-frame-pointer
CFLAGS += -std=gnu23
CFLAGS += -march=rv64gc
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding
CFLAGS += -fno-common -nostdlib
CFLAGS += -fno-builtin-strncpy -fno-builtin-strncmp -fno-builtin-strlen -fno-builtin-memset
CFLAGS += -fno-builtin-memmove -fno-builtin-memcmp -fno-builtin-log -fno-builtin-bzero
CFLAGS += -fno-builtin-strchr -fno-builtin-exit -fno-builtin-malloc -fno-builtin-putc
CFLAGS += -fno-builtin-free
CFLAGS += -fno-builtin-memcpy -fno-builtin-strcat -Wno-main
CFLAGS += -fno-builtin-printf -fno-builtin-fprintf -fno-builtin-vprintf
CFLAGS += -Iinclude

ifeq ($(COMPILER),clang)
CFLAGS += -gdwarf-4
else
CFLAGS += -ggdb -gdwarf-2
endif

ifeq ($(COMPILER),clang)
CFLAGS += --target=$(TARGET)
CFLAGS += -fno-stack-protector
CFLAGS += -fno-pie
else
CFLAGS += -Wno-old-style-declaration
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif
endif

LDFLAGS = -z max-page-size=4096

export CC AS LD OBJCOPY OBJDUMP CFLAGS LDFLAGS

K = kernel
BUILD = build

K_CSRCS := $(patsubst ./%, %, $(shell find . -name '*.c' | grep -v '/build/'))
K_SSRCS := $(patsubst ./%, %, $(shell find . -name '*.S' | grep -v '/build/'))

K_OBJS := $(patsubst %.c, $(BUILD)/%.o, $(K_CSRCS))
K_OBJS += $(patsubst %.S, $(BUILD)/%.o, $(K_SSRCS))

K_DEPS := $(K_OBJS:.o=.d)
-include $(K_DEPS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

kernel: $(K_OBJS) $K/linker.ld
	$(LD) $(LDFLAGS) -T $K/linker.ld -o $(BUILD)/kernel.elf $(K_OBJS)
	$(OBJDUMP) -S $(BUILD)/kernel.elf > $(BUILD)/kernel.asm
	$(OBJDUMP) -t $(BUILD)/kernel.elf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(BUILD)/kernel.sym
	@./mkdisk.sh

format:
	clang-format -i -style=file:./.clang-format ./**/*.h
	clang-format -i -style=file:./.clang-format ./**/*.c

.PHONY: all clean qemu qemu-gdb

all: kernel

clean:
	rm -rf build disk.img

ifndef CPUS
CPUS := 1
endif

QEMUOPTS = -machine virt -kernel $(BUILD)/kernel.elf
QEMUOPTS += -m 128M -smp $(CPUS) -serial mon:stdio -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=off
QEMUOPTS += -drive file=disk.img,format=raw,if=none,id=blk0 -device virtio-blk-device,bus=virtio-mmio-bus.0,drive=blk0

qemu: kernel
	$(QEMU) $(QEMUOPTS)

GDBPORT = $(shell expr `id -u` % 5000 + 25000)
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)

.gdbinit: .gdbinit.tmpl
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: kernel .gdbinit
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)
