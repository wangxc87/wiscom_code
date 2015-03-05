CROSS_COMPILE = /opt/arm-2009q1-203/bin/arm-none-linux-gnueabi-
DVSDK_INSTALL_DIR=/home/davicwxc/dm365/dvsdk_dm365_4_02_00_06
LINUXKERNEL_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/psp/linux-2.6.32.17-psp03.01.01.39
KERNEL_DIR = $(LINUXKERNEL_INSTALL_DIR)

EXEC_DIR=/home/davicwxc/dm365/dvsdk_dm365_4_02_00_06/filesystem/dm365rootfs
DEST_DIR = $(EXEC_DIR)/home/root/module

.PHONY:all clean install

export CROSS_COMPILE


SOURCE := second.c hello.c
OBJ := $(addsuffix .o,$(basename $(SOURCE)))
obj_module := $(addsuffix .ko,$(basename $(SOURCE)))


obj-m := $(OBJ)
#module-objs :=$(obj_module).o
PWD := $(shell pwd)

all:build_module

build_module :
	$(MAKE) -C  $(KERNEL_DIR) M=$(PWD) modules
	@echo "make $(obj_module).."
	@echo 

echovar:
	@echo "source $(SOURCE).."
	@echo "make obj_module $(obj_module).."
	@echo "make OBJ $(OBJ).."	
	@echo 

install:
	mkdir -p $(DEST_DIR)
	cp $(obj_module) $(DEST_DIR)
#	cp module_install.shell $(DEST_DIR)

clean:
	rm -f  *.o  *.ko *.order
