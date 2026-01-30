ifeq ($(wildcard $(CUTECORE)/os/src/main.c),)
$(error CUTECORE=$(CUTECORE) is not a cuteCore repo)
endif

-include $(CUTECORE)/include/config/auto.conf
-include $(CUTECORE)/include/config/auto.conf.cmd

remove_quote = $(patsubst "%",%,$(1))

GUEST_ISA ?= $(call remove_quote,$(CONFIG_ISA))
COMPILER ?= $(call remove_quote,$(CONFIG_COMPILER))


FILELIST_MK = $(shell find -L ./os/src -name "filelist.mk")
include $(FILELIST_MK)


include $(CUTECORE)/os/scripts/config.mk


include $(CUTECORE)/os/scripts/native.mk

all: kernel
