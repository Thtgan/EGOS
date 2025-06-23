include $(RELATIVE_BASE)/global.mk
include $(RELATIVE_BASE)/tools.mk
include $(RELATIVE_BASE)/funcs.mk

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
								-MMD				\

BUILD_KERNEL_MAP_NAME		:=	kernelMap.txt
BUILD_KERNEL_MAP			:=	$(BUILD_KERNEL_DIR)/$(BUILD_KERNEL_MAP_NAME)

SOURCE_FILES_C				:=	$(abspath $(shell find ./ -type f -name '*.c' | sort))
OBJ_FILES_C					:=	$(abspath $(patsubst $(PWD)/%.c, $(BUILD_KERNEL_DIR)/%.o, $(SOURCE_FILES_C)))

SOURCE_FILES_ASM			:=	$(abspath $(shell find ./ -type f -name '*.S' | sort))
OBJ_FILES_ASM				:=	$(abspath $(patsubst $(PWD)/%.S, $(BUILD_KERNEL_DIR)/%.o, $(SOURCE_FILES_ASM)))

DEP_FILES 					:=	$(patsubst %.o, %.d, $(OBJ_FILES_C) $(OBJ_FILES_ASM))

PROCESSED_LINKER_SCRIPT		:=	$(BUILD_KERNEL_DIR)/linker.ld

.PHONY: all clean

all: preBuild build postBuild

preBuild: buildDir

build: $(BUILD_KERNEL)

postBuild:

buildDir:
	@for i in $(OBJ_FILES_C) $(OBJ_FILES_ASM); do mkdir -p `dirname $$i`; done

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_FILES)
endif

$(BUILD_KERNEL): $(OBJ_FILES_C) $(OBJ_FILES_ASM) $(PROCESSED_LINKER_SCRIPT)
	@$(LD) $(OBJ_FILES_C) $(OBJ_FILES_ASM) -T $(PROCESSED_LINKER_SCRIPT) -o $@ -nmagic
	@echo "LD -> $@"
	@$(OBJDUMP) -d -S $@ > $(BUILD_KERNEL_MAP)
	@echo "OBJDUMP -> $(BUILD_KERNEL_MAP)"
	@$(OBJCOPY) --only-keep-debug $@ $@.dbg
	@echo "OBJCOPY -> $@.dbg"
	@$(OBJCOPY) --strip-debug $@

$(eval $(call CC_COMPILE, $(SOURCE_FILES_C), $(OBJ_FILES_C), $(CC_OPTIONS)))

$(eval $(call CC_COMPILE, $(SOURCE_FILES_ASM), $(OBJ_FILES_ASM), $(CC_OPTIONS)))

$(PROCESSED_LINKER_SCRIPT): linker.lds.s
	@$(CC) -E -P -x c -I../include $< > $(PROCESSED_LINKER_SCRIPT)