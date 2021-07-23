NASM 						= 	nasm
CC 							= 	x86_64-elf-gcc
CC_OPTION 					= 	-ffreestanding	\
								-mno-red-zone	\
								-m64			\
								-I.	 			\
								-I..			\

CC_OPTION_INTERRUPT_HANDLER = 	-mgeneral-regs-only
LD 							= 	x86_64-elf-ld

BUILD_DIR 					= 	./build

C_FILES 					:= 	$(shell find ./ -type f -name '*.c' | sort)
C_INTERRUPT_FILES			:=	keyboard.c
ASM_FILES					:=	./entry/entry.asm

OBJ_FILES_ASM				:=  $(ASM_FILES:.asm=.o)
OBJ_FILES_C					:=	$(C_FILES:.c=.o)
BUILD_OBJ_FILES				:=	$(addprefix $(BUILD_DIR)/, $(subst ./,, $(OBJ_FILES_ASM) $(OBJ_FILES_C)))


.PHONY: all clean

all:
	$(MAKE) buildDir
	$(MAKE) kernel.bin

clean:
	rm -rf $(BUILD_DIR)

buildDir:
	for i in $(BUILD_OBJ_FILES); do mkdir -p `dirname $$i`; done

kernel.bin:
	$(MAKE) $(OBJ_FILES_ASM)
	$(MAKE) $(OBJ_FILES_C)
	$(LD) $(BUILD_OBJ_FILES) -T linker.ld -o $(BUILD_DIR)/kernel.bin

$(OBJ_FILES_ASM):%.o:%.asm
	$(NASM) -i./ $< -f elf64 -o ./build/$@

$(OBJ_FILES_C):%.o:%.c
	@if test $(findstring $(notdir $<), $(C_INTERRUPT_FILES)); then				\
		$(CC) $(CC_OPTION) $(CC_OPTION_INTERRUPT_HANDLER) -c $< -o ./build/$@;	\
	else																		\
		$(CC) $(CC_OPTION) -c $< -o ./build/$@;									\
	fi;
