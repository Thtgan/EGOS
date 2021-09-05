CC 							= 	x86_64-elf-gcc
CC_OPTIONS 					= 	-ffreestanding	\
								-mno-red-zone	\
								-m64			\
								-I.	 			\
								-I..			\
								-I./include		\
								-I../include	\
								-Os				\

LD 							= 	x86_64-elf-ld

BUILD_DIR 					= 	./build

C_FILES 					:= 	$(shell find ./ -type f -name '*.c' | sort)

OBJ_FILES_C					:=	$(C_FILES:.c=.o)
BUILD_OBJ_FILES				:=	$(subst ./, $(BUILD_DIR)/, $(OBJ_FILES_ASM) $(OBJ_FILES_C))

.PHONY: all clean

all: buildDir kernel.bin

buildDir:
	for i in $(BUILD_OBJ_FILES); do mkdir -p `dirname $$i`; done

kernel.bin: $(OBJ_FILES_C)
	$(LD) $(BUILD_OBJ_FILES) -T linker.ld -o $(BUILD_DIR)/$@

$(OBJ_FILES_C):%.o:%.c
	$(CC) $(CC_OPTIONS) -c $< -o $(BUILD_DIR)/$@

clean:
	rm -rf $(BUILD_DIR)