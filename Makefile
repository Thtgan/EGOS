RELATIVE_BASE				:=	.

include $(RELATIVE_BASE)/global.mk
include $(RELATIVE_BASE)/tools.mk

.PHONY: all clean updateBootloader updateKernel menuconfig

-include .config

all: preBuild build postBuild

preBuild: buildDir ./include/kit/config.h

build: $(BUILD_TARGET)

postBuild:

# TODO: Use -j make this procedure fails
$(BUILD_TARGET): bootBuild kernelBuild userPrograms prepareFiles
	@$(DD) if=/dev/zero bs=1M count=$(TARGET_SIZE_MB) > $(BUILD_TARGET)

	@$(GUESTFISH) -a $(BUILD_TARGET) -- run : \
	part-init /dev/sda mbr : \
	part-add /dev/sda p 2048 -1 : \
	debug sh "mkfs.vfat -F 32 /dev/sda1" : \
	mount /dev/sda1 / : copy-in $(FILES_DIR)/* /

	@$(PY) ./tools/bootInstall/install.py -b $(shell realpath $(BUILD_BOOTLOADER)) -i $(shell realpath $(BUILD_TARGET))

buildDir:
	@$(MKDIR) -p $(BUILD_BASE_DIR)
	@$(MKDIR) -p $(FILES_DIR)/dev
	@$(MKDIR) -p $(FILES_DIR)/mnt

bootBuild:
	@$(MAKE) -C ./boot RELATIVE_BASE=$(shell realpath --relative-to ./boot .)/$(RELATIVE_BASE)

kernelBuild:
	@$(MAKE) -C ./kernel RELATIVE_BASE=$(shell realpath --relative-to ./kernel .)/$(RELATIVE_BASE)

userPrograms:
	@$(MAKE) -C ./userprogs RELATIVE_BASE=$(shell realpath --relative-to ./userprogs .)/$(RELATIVE_BASE)

prepareFiles:
	@$(MKDIR) -p $(FILES_DIR)/boot
	@$(CP) -rf $(BUILD_KERNEL) $(FILES_DIR)/boot
	@$(MKDIR) -p $(FILES_DIR)/bin
	@for i in $(filter-out lib, $(notdir $(patsubst %/,%, $(wildcard $(BUILD_USERPROGS_DIR)/*/)))); do	\
		userprog="$(BUILD_USERPROGS_DIR)/$$i/$$i";	\
		if [ -f "$$userprog" ]; then	\
			cp "$$userprog"	"$(FILES_DIR)/bin";	\
			echo "Found $$userprog";	\
		else	\
			echo "$$userprog not found";\
		fi;	\
	done;

clean:
	@$(RM) -rf $(BUILD_BASE_DIR)
	@$(RM) -rf $(FILES_DIR)/boot/*
	@$(RM) -rf $(FILES_DIR)/bin/*

updateBootloader: bootBuild
	@$(PY) ./tools/bootInstall/install.py -b $(shell realpath $(BUILD_BOOTLOADER)) -i $(shell realpath $(BUILD_TARGET))

updateKernel: kernelBuild

menuconfig:
	KCONFIG_CONFIG=.config menuconfig Kconfig
	KCONFIG_CONFIG=.config genconfig --header-path ./include/kit/config.h Kconfig

./include/kit/config.h:
	@if ! [ -f .config ]; then \
		echo "Please run 'make menuconfig' first."; \
		exit 1; \
	fi