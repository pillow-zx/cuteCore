include $(CUTECORE)/os/scripts/build.mk

KERNEL     := $(BUILD_DIR)/kernel
KERNEL_ASM := $(BUILD_DIR)/kernel.asm
KERNEL_SYM := $(BUILD_DIR)/kernel.sym
QEMU 	   := qemu-system-riscv64
QEMU_OPTS  := -machine virt -nographic -m 512M  -kernel $(KERNEL) -serial mon:stdio -nographic


CROSS_COMPILE ?= riscv64-unknown-elf-


ifeq ($(COMPILER),clang)
CC := clang
CXX := clang++
AS := clang
LD := ld.lld
OBJCOPY := llvm-objcopy
OBJDUMP := llvm-objdump
CFLAGS += --target=riscv64-unknown-elf
CFLAGS += -fno-builtin -fno-stack-protector
else
CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
AS := $(CROSS_COMPILE)gas
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
CFLAGS += -fno-builtin-strncpy -fno-builtin-strncmp -fno-builtin-strlen -fno-builtin-memset
CFLAGS += -fno-builtin-memmove -fno-builtin-memcmp -fno-builtin-log -fno-builtin-bzero
CFLAGS += -fno-builtin-strchr -fno-builtin-exit -fno-builtin-malloc -fno-builtin-putc
CFLAGS += -fno-builtin-free
CFLAGS += -fno-builtin-memcpy -Wno-main
CFLAGS += -fno-builtin-printf -fno-builtin-fprintf -fno-builtin-vprintf
endif


CFLAGS += $(INCLUDES)
CFLAGS += -Werror -Wall -Wno-unknown-attributes -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -march=rv64gc
CFLAGS += -MMD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding
CFLAGS += -fno-common -nostdlib
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

LDFLAGS = -z max-page-size=4096

LINKER := $(CUTECORE)/os/src/arch/riscv/linker.ld

C_OBJS = $(filter %.c, $(SRCS))
C_OBJS := $(C_OBJS:%.c=$(OBJ_DIR)/%.o)
S_OBJS = $(filter %.S, $(SRCS))
S_OBJS := $(S_OBJS:%.S=$(OBJ_DIR)/%.o)
OBJS = $(C_OBJS) $(S_OBJS)

$(OBJ_DIR)/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.S
	@echo + AS $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

kernel: $(OBJS)
	@$(LD) $(LDFLAGS) -T $(LINKER) -o $(KERNEL) $(OBJS)
	@$(OBJDUMP) -S $(KERNEL) > $(KERNEL_ASM)
	@$(OBJDUMP) -t $(KERNEL) | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(KERNEL_SYM)


qemu:
	@$(QEMU) $(QEMU_OPTS)

