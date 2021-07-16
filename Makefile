NASM = nasm
CC = x86_64-elf-gcc
CC_OPTION = -ffreestanding -mno-red-zone -m64
LD = x86_64-elf-ld

BUILD_DIR = ./build
BUILD_BIN_DIR = $(BUILD_DIR)/bin

vpath %.bin $(BUILD_BIN_DIR)
vpath %.tmp $(BUILD_BIN_DIR)
vpath %.o $(BUILD_BIN_DIR)

.PHONY: all clean
all: buildDir egos.bin

clean:
	rm -rf $(BUILD_DIR)

egos.bin:
	cd ./src/kernel/boot/BootStage1 && $(NASM) boot.s -f bin -o boot.bin
	mv ./src/kernel/boot/BootStage1/boot.bin $(BUILD_BIN_DIR)

	cd ./src/kernel/boot/BootStage2 && $(NASM) entry.s -f elf64 -o entry.o
	mv ./src/kernel/boot/BootStage2/entry.o $(BUILD_BIN_DIR)

	cd ./src/kernel && $(CC) $(CC_OPTION) -c kernel_main.c -o kernel_main.o
	mv ./src/kernel/kernel_main.o $(BUILD_BIN_DIR)

	cd ./src/kernel/drivers && $(CC) $(CC_OPTION) -c vgaTextMode.c -o vgaTextMode.o
	mv ./src/kernel/drivers/vgaTextMode.o $(BUILD_BIN_DIR)

	cd ./src/kernel/drivers && $(CC) $(CC_OPTION) -c portIO.c -o portIO.o
	mv ./src/kernel/drivers/portIO.o $(BUILD_BIN_DIR)

	cp ./src/kernel/linker.ld $(BUILD_BIN_DIR)
	cd $(BUILD_BIN_DIR) && $(LD) -T "linker.ld"

	cd $(BUILD_BIN_DIR) && cat boot.bin kernel.bin > EGOS.bin
	mv $(BUILD_BIN_DIR)/EGOS.bin $(BUILD_DIR)

buildDir:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_BIN_DIR)