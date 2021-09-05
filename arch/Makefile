BUILD_DIR = ./build

.PHONY: all clean

all: buildDir boot.bin

buildDir:
	mkdir -p $(BUILD_DIR)

boot.bin:
	cd boot && $(MAKE)
	mv ./boot/build/boot.bin $(BUILD_DIR)

clean:
	cd boot && $(MAKE) clean
	rm -rf $(BUILD_DIR)