# Arg1: Source file paths list
# Arg2: Object file paths list
# Arg3: CC compile options
define CC_COMPILE
    $(eval INDICES := $(shell seq 1 $(words $(1))))
    $(foreach i,$(INDICES),\
        $(eval src=$(word $(i),$(1)))\
        $(eval obj=$(word $(i),$(2)))\
        $(eval \
		$(obj):$(src);\
			@$(CC) $(3) -c $(src) -o $(obj);\
			echo "CC -> $(obj)")\
    )
endef