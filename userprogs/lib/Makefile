include $(RELATIVE_BASE)/global.mk
include $(RELATIVE_BASE)/tools.mk
include $(RELATIVE_BASE)/funcs.mk

LIBRARY_BUILD_DIR			= $(BUILD_USERPROGS_DIR)/lib

SOURCE_FILES_C				:=	$(shell find ./ -type f -name '*.c' | sort)
OBJ_FILES_C					:=	$(patsubst ./%.c, $(LIBRARY_BUILD_DIR)/%.o, $(SOURCE_FILES_C))

SOURCE_FILES_ASM			:=	$(shell find ./ -type f -name '*.S' | sort)
OBJ_FILES_ASM				:=  $(patsubst ./%.S, $(LIBRARY_BUILD_DIR)/%.o, $(SOURCE_FILES_ASM))

CC_OPTIONS 					= 	-nostdlib			\
								-fno-builtin		\
								-nostdinc			\
								-ffreestanding		\
								-lgcc				\
								-I./include	 		\

.PHONY: all clean

all: buildDir $(OBJ_FILES_C) $(OBJ_FILES_ASM)

buildDir:
	@for i in $(OBJ_FILES_C) $(OBJ_FILES_ASM); do mkdir -p `dirname $$i`; done

$(eval $(call CC_COMPILE, $(SOURCE_FILES_C), $(OBJ_FILES_C), $(CC_OPTIONS)))

$(eval $(call CC_COMPILE, $(SOURCE_FILES_ASM), $(OBJ_FILES_ASM), $(CC_OPTIONS)))