NASM 						= 	nasm
CC 							= 	x86_64-elf-gcc
CC_OPTIONS 					= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\
								-mno-red-zone		\
								-m32				\
								-I.	 				\
								-I./include			\
								-I./include/lib		\
								-I../../include		\
								-Os					\

LD 							= 	x86_64-elf-ld
LD_OPTIONS					=	-m elf_i386 -relocatable

BUILD_DIR 					= 	./build
BUILD_NAME					=	kernelEntry.o
BUILD_BIN_NAME 				=

C_FILES 					:= 	$(shell find ./ -type f -name '*.c' | sort)
ASM_FILES					:=	$(shell find ./ -type f -name '*.asm' | sort)

OBJ_FILES_ASM				:=  $(ASM_FILES:.asm=.o)
OBJ_FILES_C					:=	$(C_FILES:.c=.o)
BUILD_OBJ_FILES				:=	$(subst ./, $(BUILD_DIR)/, $(OBJ_FILES_ASM) $(OBJ_FILES_C))

.PHONY: all clean

all: preBuild build postBuild

preBuild:
override BUILD_OBJECT_NAME = $(BUILD_DIR)/$(BUILD_NAME)

build: buildDir $(BUILD_OBJECT_NAME)

postBuild:

buildDir:
	for i in $(BUILD_OBJ_FILES); do mkdir -p `dirname $$i`; done

$(BUILD_OBJECT_NAME): $(OBJ_FILES_ASM) $(OBJ_FILES_C)
	$(LD) $(BUILD_OBJ_FILES) $(LD_OPTIONS) -o $(BUILD_OBJECT_NAME)

$(OBJ_FILES_ASM):%.o:%.asm
	$(NASM) -i./ $< -f elf32 -o $(BUILD_DIR)/$@

$(OBJ_FILES_C):%.o:%.c
	$(CC) $(CC_OPTIONS) -c $< -o $(BUILD_DIR)/$@

clean:
	rm -rf $(BUILD_DIR)
	