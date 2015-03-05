include /home/wxc/dsp/myproject/Rules.make

# Compiler name (with path, if not in path)
CC=arm-arago-linux-gnueabi-gcc

EXE = disk_test

# Mainly used for include paths
CFLAGS=-I $(KERNEL_DIR)/include -I $(KERNEL_DIR)/arch/arm/include -I $(KERNEL_DIR)/drivers/char

default:
	$(MAKE) $(EXE)

install:
	cp $(EXE) $(EXE_INSTALL_DIR)

clean:
	rm $(EXE)
