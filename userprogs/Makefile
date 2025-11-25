include $(RELATIVE_BASE)/global.mk
include $(RELATIVE_BASE)/tools.mk

USERPROGS 					= $(filter-out lib, $(patsubst %/,%, $(wildcard */)))

.PHONY: all clean

all: libraries
	@for i in $(USERPROGS); do	\
		$(MAKE) -C $$i RELATIVE_BASE=$$(realpath --relative-to "./$$i" ".")/$(RELATIVE_BASE);	\
	done

libraries:
	@$(MAKE) -C lib RELATIVE_BASE=$(shell realpath --relative-to ./lib .)/$(RELATIVE_BASE)
