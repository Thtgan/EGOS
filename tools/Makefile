BUILD_DIR 					= 	./build

.PHONY: all clean

all: buildDir injectorBuild

buildDir:
	@mkdir -p $(BUILD_DIR)

injectorBuild:
	@echo BUILDING INJECTOR
	@cd injector && $(MAKE)
	@mv ./injector/build/injector $(BUILD_DIR)
	@echo BUILD INJECTOR COMPLETE

clean:
	@cd injector && $(MAKE) clean
	@rm -rf $(BUILD_DIR)