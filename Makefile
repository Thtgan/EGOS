BUILD_DIR = ./build

.PHONY: all clean

all: buildDir EGOS.bin
	@echo BUILD SUCCESS

EGOS.bin: boot.bin kernel.bin
#Fill to 64KB
	dd if=$(BUILD_DIR)/boot.bin of=$(BUILD_DIR)/boot.bin bs=1 count=1 seek=65535	
#Fill to 1MB - 64KB	
	dd if=$(BUILD_DIR)/kernel.bin of=$(BUILD_DIR)/kernel.bin bs=1 count=1 seek=983039
	cd $(BUILD_DIR) && cat boot.bin kernel.bin > EGOS.bin

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