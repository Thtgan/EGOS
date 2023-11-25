include $(RELATIVE_BASE)/global.mk
include $(RELATIVE_BASE)/tools.mk

USERPROGS 					= test
USERPROGS_BUILD 			= $(foreach i, $(USERPROGS), $(BUILD_USERPROGS_BIN_DIR)/$(i))

CC_OPTIONS 					= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\
								-mcmodel=large		\
								-I./lib/include	 	\

SOURCE_FILES_C				:=	$(shell find ./ -type f -name '*.c' | sort)
OBJ_FILES_C					:=	$(patsubst ./%.c, $(BUILD_USERPROGS_DIR)/%.o, $(SOURCE_FILES_C))

SOURCE_FILES_ASM			:=	$(shell find ./ -type f -name '*.S' | sort)
OBJ_FILES_ASM				:=  $(patsubst ./%.S, $(BUILD_USERPROGS_DIR)/%.o, $(SOURCE_FILES_ASM))

OBJ_FILES_LIB				:=	$(filter $(BUILD_USERPROGS_DIR)/lib/%, $(OBJ_FILES_C) $(OBJ_FILES_ASM))

.PHONY: all clean

all: buildDir $(USERPROGS_BUILD)

buildDir:
	@for i in $(OBJ_FILES_C) $(OBJ_FILES_ASM); do mkdir -p `dirname $$i`; done
	@$(MKDIR) -p $(BUILD_USERPROGS_BIN_DIR)

$(USERPROGS_BUILD):$(OBJ_FILES_C) $(OBJ_FILES_ASM)
	@$(LD) $(OBJ_FILES_LIB) $(filter $(BUILD_USERPROGS_DIR)/$(subst $(BUILD_USERPROGS_BIN_DIR)/,,$@)/%, $(OBJ_FILES_C) $(OBJ_FILES_ASM)) -o $@
	@echo "LD -> $@"

$(OBJ_FILES_C):$(BUILD_USERPROGS_DIR)/%.o:%.c
	@$(CC) $(CC_OPTIONS) -I./$(firstword $(subst /, ,$<)) -c $< -o $@
	@echo "CC -> $@"

$(OBJ_FILES_ASM):$(BUILD_USERPROGS_DIR)/%.o:%.S
	@$(CC) $(CC_OPTIONS) -I./$(firstword $(subst /, ,$<)) -c $< -o $@
	@echo "CC -> $@"
