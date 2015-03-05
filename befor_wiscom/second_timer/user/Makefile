CROSS_COMPILE = /opt/arm-2009q1-203/bin/arm-none-linux-gnueabi-

DVSDK_INSTALL_DIR=/home/davicwxc/dm365/dvsdk_dm365_4_02_00_06
LINUXKERNEL_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/psp/linux-2.6.32.17-psp03.01.01.39
KERNEL_DIR = $(LINUXKERNEL_INSTALL_DIR)

EXEC_DIR=/home/davicwxc/dm365/dvsdk_dm365_4_02_00_06/filesystem/dm365rootfs
DEST_DIR = $(EXEC_DIR)/home/root/

CC = $(CROSS_COMPILE)gcc  -I$(KERNEL_DIR)

source = $(wildcard *.c)
obj = $(source: %.c = %.o)

EXE = $(basename $(source))

.PHONY:all clean install

ALL:$(EXE)

$(EXE): $(obj)
	$(CC) $(obj) -o $(EXE)

%.o:%.c
	$(CC) -c $< -o $@

install:
	cp $(EXE) $(DEST_DIR)

clean:
	rm -f *.o $(EXE)
