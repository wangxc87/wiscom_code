#
# Makefile
#
# Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
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

include ../Rules.make

#Modify this to point the location where alsa include files are located
ALSA_INC = /user/alsa-lib-1.0.25/lib/include

#Modify this to point the location where alsa shared lib are located
ALSA_LIB = /user/alsa-lib-1.0.25/lib/lib

LIB_INC += $(ALSA_INC)

CFLAGS += -I$(INCLUDE_DIR) -I$(LIB_INC) -Wall
LFLAGS = -L$(LIB_DIR) -L$(ALSA_LIB)

objects := $(patsubst %.c,%.o,$(wildcard *.c))
executables := $(patsubst %.c,%,$(wildcard *.c))

LIBS = -lasound -lpthread -lm -ldl -lrt

all: $(.DELETE_ON_ERROR) $(objects)
$(objects): %.o: %.c
	mkdir -p ./bin
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $< -o $@
	$(CROSS_COMPILE)gcc $(CFLAGS) $< -o $(subst .o, ,$(EXE_DIR)/$@) $(LFLAGS) $(LIBS)

clean:
	rm -f *.o rm -f ./bin/* rm -rf ./bin		      

distclean: clean
