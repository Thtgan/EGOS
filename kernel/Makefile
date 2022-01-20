CC 							= 	x86_64-elf-gcc
CC_OPTIONS 					= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\
								-mno-red-zone		\
								-m32				\
								-I.	 				\
								-I./include			\
								-I./include/lib		\
								-I./include/lib/std	\
								-I../include		\
								-Os					\

LD 							= 	x86_64-elf-ld

BUILD_DIR 					= 	./build
BUILD_NAME					=	kernel
BUILD_BIN_NAME				=

C_FILES 					:= 	$(shell find ./ -type f -name '*.c' | sort)

OBJ_FILES_C					:=	$(C_FILES:.c=.o)
BUILD_OBJ_FILES				:=	$(subst ./, $(BUILD_DIR)/, $(OBJ_FILES_ASM) $(OBJ_FILES_C))

.PHONY: all clean

all: preBuild build postBuild

preBuild:
override BUILD_BIN_NAME = $(BUILD_DIR)/$(BUILD_NAME).bin

build: buildDir $(BUILD_BIN_NAME)

postBuild:

buildDir:
	for i in $(BUILD_OBJ_FILES); do mkdir -p `dirname $$i`; done

$(BUILD_BIN_NAME): $(OBJ_FILES_C)
	$(LD) -m elf_i386 $(BUILD_OBJ_FILES) -T linker.ld -o $(BUILD_BIN_NAME)

$(OBJ_FILES_C):%.o:%.c
	$(CC) $(CC_OPTIONS) -c $< -o $(BUILD_DIR)/$@

clean:
	rm -rf $(BUILD_DIR)