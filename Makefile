LD 							= 	x86_64-elf-ld

BUILD_DIR 					= 	./build

.PHONY: all clean debug

debug: all

all: buildDir EGOS.img
	@echo BUILD COMPLETE

EGOS.img: boot.bin kernel.bin
#Fill to 128KB
	dd if=$(BUILD_DIR)/boot.bin of=$(BUILD_DIR)/boot.bin bs=1 count=1 seek=131071
#Fill to 1MB - 128KB	
	dd if=$(BUILD_DIR)/kernel.bin of=$(BUILD_DIR)/kernel.bin bs=1 count=1 seek=917503
	cd $(BUILD_DIR) && cat boot.bin kernel.bin > EGOS.img
	

buildDir:
	mkdir -p $(BUILD_DIR)

boot.bin:
	cd boot && $(MAKE)
	mv ./boot/build/boot.bin $(BUILD_DIR)

kernel.bin:
	cd kernel && $(MAKE)
	mv ./kernel/build/kernel.bin $(BUILD_DIR)

clean:
	cd boot && $(MAKE) clean
	cd kernel && $(MAKE) clean
	rm -rf $(BUILD_DIR)