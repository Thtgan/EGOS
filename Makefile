include build.config

BUILD_DIR 					= 	./build
BUILD_RES					=	1

.PHONY: all clean

all: buildDir toolsBuild OSbuild userPrograms injectFiles
	@echo BUILD COMPLETE

OSbuild: boot.bin kernel.bin
	@echo GENETATING EGOS
ifeq ($(FORMAT), IMG)
	@dd if=$(BUILD_DIR)/boot.bin of=$(BUILD_DIR)/boot.bin bs=1 count=1 seek=131071 > /dev/null
	@cd $(BUILD_DIR) && cat boot.bin kernel.bin > EGOS.img
#Fill to 16MB, 1MB for kernel program, others for any operation, like file system
	@dd if=$(BUILD_DIR)/EGOS.img of=$(BUILD_DIR)/EGOS.img bs=1 count=1 seek=16777215 > /dev/null
#Yeah, I tested out, part of the image file might be unavailable for there is limitation to have 63 sectors on one track
else
	$(eval override BUILD_RES = 0)
	@echo ERROR: UNKNOW FORMAT $(FORMAT)
endif

	@if [ "$(BUILD_RES)" -eq "1" ] ; then echo EGOS BUILD COMPLETE ; fi

buildDir:
	@mkdir -p $(BUILD_DIR)

toolsBuild:
	@echo BUILDING TOOLS
	@cd tools && $(MAKE) > /dev/null
	@echo BUILD TOOLS COMPLETE

boot.bin:
	@echo BUILDING BOOT
	@cd boot && $(MAKE) > /dev/null
	@echo BOOT BUILD COMPLETE
	@mv ./boot/build/boot.bin $(BUILD_DIR)

kernel.bin:
	@echo BUILDING KERNEL
	@cd kernel && $(MAKE) > /dev/null
	@echo KERNEL BUILD COMPLETE
	@mv ./kernel/build/kernel.bin $(BUILD_DIR)

userPrograms:
	@echo BUILDING USER PROGRAMS
	@cd userprogs && $(MAKE) > /dev/null
	@echo USER PROGRAMS BUILD COMPLETE

injectFiles:
	@if [ "$(BUILD_RES)" -eq "1" ] ; then echo INJECTING FILES ; ./fileInject.sh ./kernel/files ./tools/injector/build/injector ./build/EGOS.img 0x800 > /dev/null ; fi

clean:
	@cd boot && $(MAKE) clean > /dev/null
	@cd kernel && $(MAKE) clean > /dev/null
	@cd tools && $(MAKE) clean > /dev/null
	@cd userprogs && $(MAKE) clean > /dev/null
	@rm -rf $(BUILD_DIR)