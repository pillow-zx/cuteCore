WORK_DIR = $(shell pwd)
BUILD_DIR = $(WORK_DIR)/build

INC_PATH := $(WORK_DIR)/include $(INC_PATH)
OBJ_DIR  = $(BUILD_DIR)/obj-$(NAME)$(SO)
BINARY   = $(BUILD_DIR)/$(NAME)$(SO)


LD := gcc
INCLUDES = $(addprefix -I,$(INC_PATH))
CFLAGS := -O2 -MMD -Wall -Werror $(INCLUDES) $(CFLAGS)
LDFLAGS := -O2 $(LDFLAGS)


C_OBJS = $(filter %.c, $(SRCS))
C_OBJS := $(C_OBJS:%.c=$(OBJ_DIR)/%.o)
S_OBJS = $(filter %.S, $(SRCS))
S_OBJS := $(S_OBJS:%.S=$(OBJ_DIR)/%.o)
OBJS = $(C_OBJS) $(S_OBJS)

$(OBJ_DIR)/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	$(call call_fixdep, $(@:.o=.d), $@)


$(OBJ_DIR)/%.o: %.S
	@echo + AS $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@


-include $(OBJS:.o=.d)

.PHONY: all clean

all: $(BINARY)

$(BINARY):: $(OBJS) $(ARCHIVES)
	@echo + LD $@
	@$(LD) -o $@ $(OBJS) $(ARCHIVES) $(LIBS)


clean:
	-rm -rf $(BUILD_DIR)
