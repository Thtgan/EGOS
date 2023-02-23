PROGRAMS = test

.PHONY: all clean

all: 
	@for i in $(PROGRAMS); do cd $$i && $(MAKE); done
	@for i in $(PROGRAMS); do mv ./$$i/build/$$i ../kernel/files; done

clean:
	@for i in $(PROGRAMS); do cd $$i && $(MAKE) clean; done
	@for i in $(PROGRAMS); do rm -f ../kernel/files/$$i; done