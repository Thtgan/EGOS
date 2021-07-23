NASM = nasm

BUILD_DIR = ./build

.PHONY: all clean
all: 
	$(MAKE) buildDir
	$(MAKE) egos.bin

clean:
	cd kernel && $(MAKE) clean
	rm -rf $(BUILD_DIR)

egos.bin:
	cd boot && $(NASM) boot.asm -f bin -o boot.bin
	mv boot/boot.bin $(BUILD_DIR)

	cd kernel && $(MAKE)
	mv kernel/build/kernel.bin $(BUILD_DIR)

	cd $(BUILD_DIR) && cat boot.bin kernel.bin >> EGOS.bin

buildDir:
	mkdir -p $(BUILD_DIR)