include /home/wxc/dsp/myproject/Rules.make

# Compiler name (with path, if not in path)
CC=arm-arago-linux-gnueabi-gcc

EXE = com_test

# Mainly used for include paths
CFLAGS=-I $(KERNEL_DIR)/include -I $(KERNEL_DIR)/arch/arm/include -I $(KERNEL_DIR)/drivers/char

default:
	$(CC) $(EXE).c -o $(EXE) $(CFLAGS)
	gcc $(EXE).c -o $(EXE)_pc
install:
	cp $(EXE) $(EXE_INSTALL_DIR)
	sudo cp $(EXE)_pc /usr/local/bin/
clean:
	rm $(EXE)
