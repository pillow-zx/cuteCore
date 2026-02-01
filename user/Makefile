CROSS_COMPILE = riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS += -march=rv64gc -mcmodel=medany -ffreestanding
CFLAGS += -nostdlib -fno-common -static -Ilib -ggdb

LDFLAGS += -Tlinker.ld

LIB_DIR = lib
SRC_DIR = bin
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
ELF_DIR = $(BUILD_DIR)/elf
BIN_DIR = $(BUILD_DIR)/bin

LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS = $(patsubst $(LIB_DIR)/%.c,$(OBJ_DIR)/lib_%.o,$(LIB_SRCS))

APP_SRCS = $(wildcard $(SRC_DIR)/*.c)
APP_NEMAS = $(patsubst $(SRC_DIR)/%.c,%,$(APP_SRCS))
ELFS = $(patsubst %,$(ELF_DIR)/%.elf,$(APP_NEMAS))
BINS = $(patsubst %,$(BIN_DIR)/%.bin,$(APP_NEMAS))

.PHONY: all clean elf bin

all: elf bin

elf: $(ELFS)
bin: $(BINS)

.PRECIOUS: $(OBJ_DIR)/%.o $(OBJ_DIR)/lib_%.o

$(OBJ_DIR)/lib_%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(ELF_DIR)/%.elf: $(OBJ_DIR)/%.o $(LIB_OBJS)
	@mkdir -p $(dir $@)
	@$(LD) $(LDFLAGS) $^ -o $@
	@$(OBJDUMP) -d $@ > $@.asm

$(BIN_DIR)/%.bin: $(ELF_DIR)/%.elf
	@mkdir -p $(dir $@)
	@$(OBJCOPY) -O binary $< $@

clean:
	@rm -rf $(BUILD_DIR)



