#dvrrdk root
DVRRDK_ROOT=/home/wxc/dsp/DVRRDK_04.01.00.02

#CROSS COMPILE
CODEGEN_PATH_A8 = $(DVRRDK_ROOT)/ti_tools/cgt_a8/arago/linux-devkit

CROSS_COMPILE = arm-arago-linux-gnueabi
CC=$(CODEGEN_PATH_A8)/bin/$(CROSS_COMPILE)-gcc
AR=$(CODEGEN_PATH_A8)/bin/$(CROSS_COMPILE)-ar
LD=$(CODEGEN_PATH_A8)/bin/$(CROSS_COMPILE)-ld

#kernel path
KERNEL_DIR = $(DVRRDK_ROOT)/ti_tools/linux_lsp/kernel/linux-dvr-rdk

#devkit path
DVRRDK_DEVKIT = $(DVRRDK_ROOT)/ti_tools/cgt_a8/arago/linux-devkit

#root filesystem
ROOTFS_DIR=$(DVRRDK_ROOT)/target/rfs_816x

#library install dir
LIBRARY_INSTALL_DIR =$(ROOTFS_DIR)/usr/lib
#EXE INSTALL DIR
EXE_INSTALL_DIR = $(ROOTFS_DIR)/home/root

#Libc path
#LDFLAGS="-isystem $(TOOLCHAIN_DIR)/$(COMPILER_PREFIX)/libc/lib"
#LDFLAGS="-isystem $(TOOLCHAIN_DIR)/$(COMPILER_PREFIX)/lib"

#This is the path to the Directory containing Library
LIB_DIR = -I$(DVRRDK_DEVKIT)/${CROSS_COMPILE}/lib

#include path
export INC_DIR = -I$(DVRRDK_DEVKIT)/${CROSS_COMPILE}/usr/include -I$(KERNEL_DIR)/include -I$(KERNEL_DIR)/arch/arm/include/ -I$(KERNEL_DIR)/arch/arm/plat-omap/include/

export CC
export LD
export AR
export EXE_INSTALL_DIR
export LIBRARY_INSTALL_DIR
export ROOTFS_DIR
#INC_DIR += LIB_DIR
