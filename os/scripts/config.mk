Q			 := @
KCONFIG_PATH := $(CUTECORE)/os/tools/kconfig
FIXDEP_PATH  := $(CUTECORE)/os/tools/fixdep
Kconfig      := $(CUTECORE)/os/Kconfig
silent       := -s


CONF   := $(KCONFIG_PATH)/build/conf
MCONF  := $(KCONFIG_PATH)/build/mconf
FIXDEP := $(FIXDEP_PATH)/build/fixdep

$(CONF):
	$(Q)$(MAKE) $(silent) -C $(KCONFIG_PATH) NAME=conf

$(MCONF):
	$(Q)$(MAKE) $(silent) -C $(KCONFIG_PATH) NAME=mconf

$(FIXDEP):
	$(Q)$(MAKE) $(silent) -C $(FIXDEP_PATH)

menuconfig: $(MCONF) $(CONF) $(FIXDEP)
	$(Q)$(MCONF) $(Kconfig)
	$(Q)$(CONF) $(silent) --syncconfig $(Kconfig)


help:
	@echo 		' menuconfig        - Updata current config utilising a menu based program'
	@echo 		' kernel            - Build cuteCore kernel default' 


.PHONY: help

define call_fixdep
	@$(FIXDEP) $(1) $(2) unused > $(1).tmp
	@mv $(1).tmp $(1)
endef
