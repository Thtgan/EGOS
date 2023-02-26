PROGRAMS = test

CC 							= 	x86_64-elf-gcc
CC_OPTIONS 					= 	-nostdlib			\
								-nostdinc			\
								-ffreestanding		\
								-mcmodel=large		\
								-I./lib/include	 	\

LD 							= 	x86_64-elf-ld

BUILD_DIR 					= 	./build

C_FILES 					:= 	$(shell find ./ -type f -name '*.c' | sort)
ASM_FILES					:=	$(shell find ./ -type f -name '*.asm' | sort)

OBJ_FILES_C					:=	$(C_FILES:.c=.o)
OBJ_FILES_ASM				:=  $(ASM_FILES:.asm=.o)
BUILD_OBJ_FILES				:=	$(subst ./, $(BUILD_DIR)/, $(OBJ_FILES_ASM) $(OBJ_FILES_C))
BUILD_LIB_OBJ_FILES			:=	$(filter $(BUILD_DIR)/lib/%, $(BUILD_OBJ_FILES))

.PHONY: all clean

all: buildDir $(PROGRAMS)

buildDir:
	@for i in $(BUILD_OBJ_FILES); do mkdir -p `dirname $$i`; done

$(PROGRAMS): $(OBJ_FILES_C) $(OBJ_FILES_ASM)
	$(foreach i, $(PROGRAMS), @$(LD) $(BUILD_LIB_OBJ_FILES) $(filter $(BUILD_DIR)/$(i)/%, $(BUILD_OBJ_FILES)) -T ./lib/linker.ld -o $(BUILD_DIR)/$(i)/$(i))
	$(foreach i, $(PROGRAMS), @mv $(BUILD_DIR)/$(i)/$(i) ../kernel/files)

$(OBJ_FILES_C):%.o:%.c
	@$(CC) $(CC_OPTIONS) -I./$(firstword $(subst /, ,$<)) -c $< -o $(BUILD_DIR)/$@

$(OBJ_FILES_ASM):%.o:%.asm
	@$(NASM) $< -f elf64 -o $(BUILD_DIR)/$@

clean:
	@rm -rf $(BUILD_DIR)