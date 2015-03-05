#
# Rules.make
#
# Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
#
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#    Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the
#    distribution.
#
#    Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#This is the path to the Linux Kernel Directory. Change this path
#to the one where Linux kernel is kept.
#KERNEL_DIR = /user/a0875517/git-work/omap-kernel
KERNEL_DIR = $(shell pwd)/../../linux-04.04.00.01

#This is root directory where Source is available. Change this path
#to the one where source files are kept
INSTALL_DIR = ./

#this is the path to the executable where all the executables are kept
EXE_DIR = ./bin

#This is path to the Include files of Library
#LIB_INC = $(INSTALL_DIR)/User_Library

#This is the path the include folder of the kernel directory. The include
#folder should contain all the necessary header files
INCLUDE_DIR = -I$(KERNEL_DIR)/include -I$(KERNEL_DIR)/arch/arm/include/ -I$(KERNEL_DIR)/arch/arm/plat-omap/include/

#This is the prefix applied to the gcc when compiling applications and library
COMPILER_PREFIX = arm-none-linux-gnueabi
CROSS_COMPILE = $(COMPILER_PREFIX)-

#This is the toolchain base folder
#TOOLCHAIN_DIR = /opt/codesourcery/2009q1-203
TOOLCHAIN_DIR = /opt/arm-2009q1

#add cflags
CFLAGS += -march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
#CFLAGS += -g -Os -Wall -fno-common -ffixed-r8 -msoft-float
CFLAGS += -I$(TOOLCHAIN_DIR)/$(COMPILER_PREFIX)/libc/usr/include -I$(KERNEL_DIR)

#Libc path
LDFLAGS="-isystem $(TOOLCHAIN_DIR)/$(COMPILER_PREFIX)/libc/lib"

#This is the path to the Directory containing Library
LIB_DIR = $(TOOLCHAIN_DIR)/$(COMPILER_PREFIX)/libc/lib

CC     := $(CROSS_COMPILE)gcc
AR     := $(CROSS_COMPILE)ar
LD     := $(CROSS_COMPILE)ld
NM     := $(CROSS_COMPILE)nm
RANLIB := $(CROSS_COMPILE)ranlib
STRIP  := $(CROSS_COMPILE)strip
