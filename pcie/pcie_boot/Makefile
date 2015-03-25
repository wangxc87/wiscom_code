include /home/wxc/dsp/myproject/Rules.make

# Compiler name (with path, if not in path)
CC=arm-none-linux-gnueabi-gcc

# Kernel base directory name with path
#KERNEL_DIR=../../linux-omap-2.6

# Mainly used for include paths
CFLAGS=-I $(KERNEL_DIR)/include -I $(KERNEL_DIR)/arch/arm/include -I $(KERNEL_DIR)/drivers/char

default:
	$(MAKE) saBootApp
	$(MAKE) saBootApp_nums

clean:
	rm saBootApp  saBootApp_numszai
