NASM = nasm
BOOT_SRC = Boot.s Entry.s
BOOT_BIN = $(BOOT_SRC:.s=.bin)
BUILD_DIR = ./build

vpath %.s ./kernel/boot/BootStage1
vpath %.s ./kernel/boot/BootStage2

.PHONY: all clean
all: buildDir egos.bin

clean:
	rm -rf $(BUILD_DIR)

egos.bin: $(BOOT_BIN)
	cd $(BUILD_DIR) && cat $(BOOT_BIN) > EGOS.bin

buildDir:
	mkdir -p $(BUILD_DIR)

$(BOOT_BIN): %.bin: %.s
	cd $(dir $<) && $(NASM) $(notdir $<) -f bin -o $(notdir $@)
	mv $(dir $<)$@ $(BUILD_DIR)
