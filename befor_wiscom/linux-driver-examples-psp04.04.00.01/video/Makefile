#
# Makefile
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

ROOTDIR = .

include ../Rules.make

CFLAGS = -g -Os \
	-march=armv7-a \
	-fno-strict-aliasing \
	-fno-common \
	-ffixed-r8 \
	-msoft-float \
	-I$(INCLUDE_DIR) \
	-pipe \
	-mno-thumb-interwork \
	-Wall \
	-Wstrict-prototypes \
	-fno-stack-protector \
	 -D__EXPORTED_HEADERS__

LDFLAGS = -L/opt/codesourcery/arm-none-linux-gnueabi/libc/lib


#
# Common sources
#


#
# Platform specific sources
#
SRCS := saFbdevDisplay.c  \
	saFbdevDisplayPan.c  \
	saFbdevMovingDisplay.c  \
	saFbdevScalingDisplay.c  \
	saFbdevHdmiDisplay.c \
	saMmapDisplay.c \
	saUserptrDisplay.c

ifneq ($(PLATFORM),TI811X)
  SRCS += saLoopBackFbdev.c \
	  saLoopBack.c \
	  saLoopBackScale.c
endif

#
# Common libraries
#
LIBS := -lpthread

#
# Platform specific options
#
CFLAGS += -DCONFIG_TI81XX

#
# Get the list of executables
#
APPS :=$(patsubst %.c, %, $(addprefix $(EXE_DIR)/, $(SRCS)))

#
# Default target
#
.PHONY: all

all: $(APPS)


#
# Clean the generated binaries
#
.PHONY: clean

clean:
	@echo ""
	@echo "Cleaning binaries..."
	@echo ""

	@rm -f $(EXE_DIR)/sa*


#
# Build examples
#
$(APPS):
	@echo Creating $@
	$(CROSS_COMPILE)gcc $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@ $(addsuffix .c,${@F})

