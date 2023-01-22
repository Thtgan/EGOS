include ../build.config

NASM 						= 	nasm
CC 							= 	x86_64-elf-gcc
CC_OPTIONS 					= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\
								-mno-red-zone		\
								-m64				\
								-I.	 				\
								-I./include			\
								-I./include/lib		\
								-I./include/lib/std	\
								-I../include		\
								-Os					\
								-mcmodel=large		\

LD 							= 	x86_64-elf-ld

BUILD_DIR 					= 	./build
BUILD_NAME					=	kernel.bin
BUILD_BIN_NAME				=	$(BUILD_DIR)/$(BUILD_NAME)

C_FILES 					:= 	$(shell find ./ -type f -name '*.c' | sort)
ASM_FILES					:=	$(shell find ./ -type f -name '*.asm' | sort)

OBJ_FILES_C					:=	$(C_FILES:.c=.o)
OBJ_FILES_ASM				:=  $(ASM_FILES:.asm=.o)
BUILD_OBJ_FILES				:=	$(subst ./, $(BUILD_DIR)/, $(OBJ_FILES_ASM) $(OBJ_FILES_C))

.PHONY: all clean

all: preBuild build postBuild

preBuild:
ifeq ($(TEST), TRUE)
	$(eval CC_OPTIONS += -D DEBUG)
endif

build: buildDir $(BUILD_BIN_NAME)

postBuild:

buildDir:
	@for i in $(BUILD_OBJ_FILES); do mkdir -p `dirname $$i`; done

$(BUILD_BIN_NAME): $(OBJ_FILES_C)
	@$(LD) $(BUILD_OBJ_FILES) -T linker.ld -o $(BUILD_BIN_NAME)

$(OBJ_FILES_C):%.o:%.c
	@$(CC) $(CC_OPTIONS) -c $< -o $(BUILD_DIR)/$@

$(OBJ_FILES_ASM):%.o:%.c
	@$(NASM) $< -f elf64 -o $(BUILD_DIR)/$@

clean:
	@rm -rf $(BUILD_DIR)