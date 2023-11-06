include $(RELATIVE_BASE)/global.mk
include $(RELATIVE_BASE)/tools.mk

CC_OPTIONS 					:= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\
								-mno-red-zone		\
								-mno-mmx			\
								-mno-sse			\
								-mno-sse2			\
								-m32				\
								-I./include			\
								-I./include/lib		\
								-I../include		\
								-O2					\
								-g					\

BUILD_BOOTLOADER_MAP_NAME	:=	bootMap.txt
BUILD_BOOTLOADER_MAP		:=	$(BUILD_BOORLOADER_DIR)/$(BUILD_BOOTLOADER_MAP_NAME)

SOURCE_FILES_C				:=	$(shell find ./ -type f -name '*.c' | sort)
OBJ_FILES_C					:=	$(patsubst ./%.c, $(BUILD_BOORLOADER_DIR)/%.o, $(SOURCE_FILES_C))

SOURCE_FILES_ASM			:=	$(shell find ./ -type f -name '*.S' | sort)
OBJ_FILES_ASM				:=  $(patsubst ./%.S, $(BUILD_BOORLOADER_DIR)/%.o, $(SOURCE_FILES_ASM))

PROCESSED_LINKER_SCRIPT		:=	$(BUILD_BOORLOADER_DIR)/linker.ld
BUILD_TARGET_BLOCK_SIZE		:= 	512

.PHONY: all test

all: preBuild build postBuild

preBuild: buildDir

build: $(BUILD_BOOTLOADER)

postBuild:

buildDir:
	@for i in $(OBJ_FILES_C) $(OBJ_FILES_ASM); do mkdir -p `dirname $$i`; done
	
$(BUILD_BOOTLOADER): $(OBJ_FILES_C) $(OBJ_FILES_ASM) $(PROCESSED_LINKER_SCRIPT)
	@$(LD) $(OBJ_FILES_C) $(OBJ_FILES_ASM) -T $(PROCESSED_LINKER_SCRIPT) -m elf_i386 -o $@
	@echo "LD -> $@"
	@$(OBJDUMP) -d -S $@ > $(BUILD_BOOTLOADER_MAP)
	@echo "OBJDUMP -> $(BUILD_BOOTLOADER_MAP)"
	@$(OBJCOPY) --only-keep-debug $@ $@.dbg
	@echo "OBJCOPY -> $@.dbg"
	@$(OBJCOPY) -O binary $@ $@
	@$(DD) if=/dev/null of=$@ bs=1 count=1 seek=$$(expr $$(expr $$(stat -L -c %s $@) + $(BUILD_TARGET_BLOCK_SIZE) - 1) / $(BUILD_TARGET_BLOCK_SIZE) \* $(BUILD_TARGET_BLOCK_SIZE))

$(OBJ_FILES_C):$(BUILD_BOORLOADER_DIR)/%.o:%.c
	@$(CC) $(CC_OPTIONS) -c ./$< -o $@
	@echo "CC -> $@"

$(OBJ_FILES_ASM):$(BUILD_BOORLOADER_DIR)/%.o:%.S
	@$(CC) $(CC_OPTIONS) -c ./$< -o $@
	@echo "CC -> $@"

$(PROCESSED_LINKER_SCRIPT): linker.lds.s
	@$(CC) -E -P -x c -I../include $< > $(PROCESSED_LINKER_SCRIPT)