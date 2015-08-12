
SUB_DIRS = src demo

.PHONY: all clean install $(SUB_DIRS)

all: $(SUB_DIRS)

$(SUB_DIRS):
	make -C $@

clean:
	for dir in $(SUB_DIRS); do $(MAKE) -C $$dir clean; done

install:
	for dir in $(SUB_DIRS); do $(MAKE) -C $$dir install; done
