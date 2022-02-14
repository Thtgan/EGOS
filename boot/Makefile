NASM 						= 	nasm
CC 							= 	x86_64-elf-gcc
CC_OPTIONS 					= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\
								-mno-red-zone		\
								-m16				\
								-I.	 				\
								-I./include			\
								-I./include/lib		\
								-I../include		\
								-Os					\
								-march=i386			\

LD 							= 	x86_64-elf-ld
LD_OPTIONS					=	-melf_i386

BUILD_DIR 					= 	./build
BUILD_NAME					=	boot.bin
BUILD_BIN_NAME 				=

C_FILES 					:= 	$(filter-out $(wildcard ./kernelEntry/*.c), $(shell find ./ -type f -name '*.c' | sort))
ASM_FILES					:=	$(filter-out $(wildcard ./kernelEntry/*.asm), $(shell find ./ -type f -name '*.asm' | sort))

OBJ_FILES_C					:=	$(C_FILES:.c=.o)
OBJ_FILES_ASM				:=  $(ASM_FILES:.asm=.o)
ENTRY_OBJ_FILE				=	kernelEntry.o

BUILD_OBJ_FILES				:=	$(subst ./, $(BUILD_DIR)/, $(OBJ_FILES_ASM) $(OBJ_FILES_C) ./$(ENTRY_OBJ_FILE))

.PHONY: all clean

all: preBuild build postBuild

preBuild:
override BUILD_OBJECT_NAME = $(BUILD_DIR)/$(BUILD_NAME)

build: buildDir $(BUILD_OBJECT_NAME)

postBuild:

buildDir:
	for i in $(BUILD_OBJ_FILES); do mkdir -p `dirname $$i`; done

$(BUILD_OBJECT_NAME): $(OBJ_FILES_ASM) $(OBJ_FILES_C) $(ENTRY_OBJ_FILE)
	$(LD) $(BUILD_OBJ_FILES) $(LD_OPTIONS) -T linker.ld -o $(BUILD_OBJECT_NAME)

$(OBJ_FILES_ASM):%.o:%.asm
	$(NASM) -i./ $< -f elf32 -o $(BUILD_DIR)/$@

$(OBJ_FILES_C):%.o:%.c
	$(CC) $(CC_OPTIONS) -c $< -o $(BUILD_DIR)/$@

$(ENTRY_OBJ_FILE):
	cd kernelEntry && $(MAKE)
	mv ./kernelEntry/build/$(ENTRY_OBJ_FILE) $(BUILD_DIR)/$(ENTRY_OBJ_FILE)

clean:
	rm -rf $(BUILD_DIR)
	cd kernelEntry && $(MAKE) clean