BUILD_DIR 					= 	./build

.PHONY: all clean

all: injectorBuild

injectorBuild:
	@echo BUILDING INJECTOR
	@cd injector && $(MAKE)
	@echo BUILD INJECTOR COMPLETE

clean:
	@cd injector && $(MAKE) clean