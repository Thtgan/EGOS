CC 							= 	x86_64-elf-gcc
CC_OPTIONS 					= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\

LD 							= 	x86_64-elf-ld

BUILD_DIR 					= 	./build
BUILD_NAME					=	test
BUILD_PROGRAM_NAME			=	$(BUILD_DIR)/$(BUILD_NAME)
BUILD_BIN_NAME				=	$(BUILD_PROGRAM_NAME).bin

C_FILES 					:= 	$(shell find ./ -type f -name '*.c' | sort)
OBJ_FILES_C					:=	$(C_FILES:.c=.o)
BUILD_OBJ_FILES				:=	$(subst ./, $(BUILD_DIR)/, $(OBJ_FILES_C))

.PHONY: all clean

all: buildDir $(OBJ_FILES_C)
	@$(LD) $(BUILD_OBJ_FILES) -e main -o $(BUILD_BIN_NAME)
	@objcopy -O binary -j .text $(BUILD_BIN_NAME) $(BUILD_PROGRAM_NAME)

buildDir:
	@for i in $(BUILD_OBJ_FILES); do mkdir -p `dirname $$i`; done

$(OBJ_FILES_C):%.o:%.c
	@$(CC) $(CC_OPTIONS) -c $< -o $(BUILD_DIR)/$@

clean:
	@rm -rf $(BUILD_DIR)