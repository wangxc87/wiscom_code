
CFLAGS = -I $(KERNSRC)

CROSS_COMPILE = arm-arago-linux-gnueabi-

DVRRDK_ROOT=/home/wxc/dsp/DVRRDK_04.01.00.02
KERNSRC = $(DVRRDK_ROOT)/ti_tools/linux_lsp/kernel/linux-dvr-rdk

#root filesystem
ROOTFS_DIR=$(DVRRDK_ROOT)/target/rfs_816x
#EXE INSTALL DIR
EXE_INSTALL_DIR = $(ROOTFS_DIR)/home/root

export EXE_INSTALL_DIR
export ROOTFS_DIR

# Uncomment to turn verbose mode on
ifneq ($(debug),)
CFLAGS += -D$(debug)
endif

ifneq ($(edma),)
CFLAGS += -DEDMA_TRANSMISSION
endif

CFLAGS += -O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp

all: build_all

build_all:lib
	@make -C demo "CC=$(CROSS_COMPILE)" "CFLAGS=$(CFLAGS)" all

lib:
	@echo "make lib.."
	@make -C src "CC=$(CROSS_COMPILE)" "CFLAGS=$(CFLAGS)"

rc:lib
	@make -C demo "CC=$(CROSS_COMPILE)" "CFLAGS=$(CFLAGS)" $@

ep:lib
#	@make -C src "CC=$(CROSS_COMPILE)" "CFLAGS=$(CFLAGS)"
	@make -C demo "CC=$(CROSS_COMPILE)" "CFLAGS=$(CFLAGS)" $@

help:
	@echo "make [MODE=THPT/INTEGRITY/DISPLAY] [debug=DEBUG]"

install:
	@make -C demo install
clean:
	@make -C src clean 
	@make -C demo clean
