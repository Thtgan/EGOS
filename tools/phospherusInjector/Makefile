.PHONY: all clean

CC 							= 	gcc
CC_OPTIONS 					= 	-I./include			\
								-O2					\

BUILD_DIR 					= 	./build
BUILD_NAME					=	phospherusInjector
BUILD_BIN_NAME				=

C_FILES 					:= 	$(shell find ./ -type f -name '*.c' | sort)

all: preBuild build postBuild

preBuild:
override BUILD_BIN_NAME = $(BUILD_DIR)/$(BUILD_NAME)

build: buildDir $(BUILD_BIN_NAME)

postBuild:

buildDir:
	mkdir -p $(BUILD_DIR)

$(BUILD_BIN_NAME):
	$(CC) $(C_FILES) -g -o $(BUILD_DIR)/$(BUILD_NAME) $(CC_OPTIONS)

clean:
	rm -rf $(BUILD_DIR)