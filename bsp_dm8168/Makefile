include Rules.make

SUB_DIRS = src demo bsp_pcie
SUB_DIRS += hw_test 

.PHONY: all clean install $(SUB_DIRS) $(DEMO_DIRS)

all: $(SUB_DIRS)

$(SUB_DIRS):
	make -C $@

clean:
	for dir in $(SUB_DIRS); do $(MAKE) -C $$dir clean; done

install:
	mkdir -p $(EXE_INSTALL_DIR)
	for dir in $(SUB_DIRS); do $(MAKE) -C $$dir install; done
