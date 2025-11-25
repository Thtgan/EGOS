include $(RELATIVE_BASE)/global.mk
include $(RELATIVE_BASE)/tools.mk
include $(RELATIVE_BASE)/funcs.mk

USERPROG_NAME 				= test
BUILD_USERPROG_DIR			= $(BUILD_USERPROGS_DIR)/$(USERPROG_NAME)
BUILD_USERPROG_TARGET		= $(BUILD_USERPROG_DIR)/$(USERPROG_NAME)

SOURCE_FILES_C				:=	$(shell find ./ -type f -name '*.c' | sort)
OBJ_FILES_C					:=	$(patsubst ./%.c, $(BUILD_USERPROG_DIR)/%.o, $(SOURCE_FILES_C))

SOURCE_FILES_ASM			:=	$(shell find ./ -type f -name '*.S' | sort)
OBJ_FILES_ASM				:=  $(patsubst ./%.S, $(BUILD_USERPROG_DIR)/%.o, $(SOURCE_FILES_ASM))

OBJ_FILES_LIB				:=	$(shell find $(BUILD_USERPROGS_DIR)/lib/ -type f -name '*.o' | sort)

CC_OPTIONS 					= 	-nostdlib			\
								-fno-builtin		\
								-nostdinc			\
								-ffreestanding		\
								-lgcc				\
								-I../lib/include	\
								-I.					\

.PHONY: all clean

all: buildDir $(BUILD_USERPROG_TARGET)

buildDir:
	@for i in $(OBJ_FILES_C) $(OBJ_FILES_ASM); do mkdir -p `dirname $$i`; done

$(BUILD_USERPROG_TARGET): $(OBJ_FILES_C) $(OBJ_FILES_ASM)
	@$(LD) $(OBJ_FILES_LIB) $(OBJ_FILES_C) $(OBJ_FILES_ASM) -o $@
	@echo "LD -> $@"

$(eval $(call CC_COMPILE, $(SOURCE_FILES_C), $(OBJ_FILES_C), $(CC_OPTIONS)))

$(eval $(call CC_COMPILE, $(SOURCE_FILES_ASM), $(OBJ_FILES_ASM), $(CC_OPTIONS)))