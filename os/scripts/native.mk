include $(CUTECORE)/os/scripts/build.mk

KERNEL     := $(BUILD_DIR)/kernel
KERNEL_ASM := $(BUILD_DIR)/kernel.asm
KERNEL_SYM := $(BUILD_DIR)/kernel.sym
CLANG_FMT  := clang-format
FORMAT_ARG := -i -style=file:$(CUTECORE)/.clang-format
QEMU 	   := qemu-system-riscv64
QEMU_OPTS  := -machine virt -nographic -m 512M  -kernel $(KERNEL) -serial mon:stdio -nographic
LINKER := $(CUTECORE)/os/src/arch/riscv/linker.ld

include $(CUTECORE)/os/scripts/compiler.mk

ifeq ($(NAME),cuteCore-riscv64)
kernel: $(OBJS)
	@python3 $(CUTECORE)/build.py
	@$(LD) $(LDFLAGS) -T $(LINKER) -o $(KERNEL) $(OBJS)
	@$(OBJDUMP) -S $(KERNEL) > $(KERNEL_ASM)
	@$(OBJDUMP) -t $(KERNEL) | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(KERNEL_SYM)

user:
	@cd $(CUTECORE)/user && $(MAKE) user

format: $(SRCS)
	@$(CLANG_FMT) $(FORMAT_ARG) $(filter %.c, $(SRCS))

qemu:
	@$(QEMU) $(QEMU_OPTS)
endif


ifeq ($(NAME),cuteCore-loongarch64)
endif

