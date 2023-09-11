include $(RELATIVE_BASE)/global.mk
include $(RELATIVE_BASE)/tools.mk

CC_OPTIONS 					= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\
								-mno-red-zone		\
								-mno-mmx			\
								-mno-sse			\
								-mno-sse2			\
								-m64				\
								-I.	 				\
								-I./include			\
								-I./include/lib		\
								-I../include		\
								-O2					\
								-mcmodel=large		\
								-g					\

BUILD_KERNEL_MAP_NAME		:=	kernelMap.txt
BUILD_KERNEL_MAP			:=	$(BUILD_KERNEL_DIR)/$(BUILD_KERNEL_MAP_NAME)

SOURCE_FILES_C				:=	$(shell find ./ -type f -name '*.c' | sort)
OBJ_FILES_C					:=	$(patsubst ./%.c, $(BUILD_KERNEL_DIR)/%.o, $(SOURCE_FILES_C))

SOURCE_FILES_ASM			:=	$(shell find ./ -type f -name '*.S' | sort)
OBJ_FILES_ASM				:=  $(patsubst ./%.S, $(BUILD_KERNEL_DIR)/%.o, $(SOURCE_FILES_ASM))

.PHONY: all clean

all: preBuild build postBuild

preBuild: buildDir

build: $(BUILD_KERNEL)

postBuild:

buildDir:
	@for i in $(OBJ_FILES_C) $(OBJ_FILES_ASM); do mkdir -p `dirname $$i`; done

$(BUILD_KERNEL): $(OBJ_FILES_C) $(OBJ_FILES_ASM)
	@$(LD) $(OBJ_FILES_C) $(OBJ_FILES_ASM) -T linker.ld -o $@ -nmagic
	@echo "LD -> $@"
	@$(OBJDUMP) -d -S $@ > $(BUILD_KERNEL_MAP)
	@echo "OBJDUMP -> $(BUILD_KERNEL_MAP)"
	@$(OBJCOPY) --only-keep-debug $@ $@.dbg
	@echo "OBJCOPY -> $@.dbg"
	@$(OBJCOPY) --strip-debug $@

$(OBJ_FILES_C):$(BUILD_KERNEL_DIR)/%.o:%.c
	@$(CC) $(CC_OPTIONS) -c ./$< -o $@
	@echo "CC -> $@"

$(OBJ_FILES_ASM):$(BUILD_KERNEL_DIR)/%.o:%.S
	@$(CC) $(CC_OPTIONS) -c ./$< -o $@
	@echo "CC -> $@"