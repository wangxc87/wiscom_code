#include ./arm-config.mk
include ./Rules.make

SUB_DIRS = src demo hw_test

.PHONY: all clean install $(SUB_DIRS) $(DEMO_DIRS)

all: $(SUB_DIRS)

$(SUB_DIRS):
	make -C $@

clean:
	for dir in $(SUB_DIRS); do $(MAKE) -C $$dir clean; done

install:
	for dir in $(SUB_DIRS); do $(MAKE) -C $$dir install; done
