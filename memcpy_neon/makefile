ROOTDIR = ..
include $(ROOTDIR)/Rules.make
##### Change the following for your environment: 
#CODEGEN_PATH_A8  = /opt/CodeSourcery
CODEGEN_PREFIX   = $(CODEGEN_PATH_A8)/bin/arm-arago-linux-gnueabi-
CROSS_COMPILE=		$(CODEGEN_PREFIX)
COMPILE_OPTS =		$(INCLUDES) -I. -O3 -DSOCKLEN_T=socklen_t -DNO_STRSTREAM=1 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
C =			c
C_COMPILER =		$(CROSS_COMPILE)gcc
C_FLAGS =		$(COMPILE_OPTS)

TLIB = lib_memcpy_neon.a
all: memcpy_test

memcpy.o: memcpy.S
	$(C_COMPILER) -I. -DFNAME=memcpy_arm -O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -c memcpy.S

memcpy_neon.o: memcpy_neon.S
	$(C_COMPILER) -I. -DFNAME=memcpy_neon -O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -c memcpy_neon.S

memcpy_test: memcpy_test.c memcpy_neon.o memcpy.o
	$(C_COMPILER)  -o memcpy_test memcpy_test.c memcpy_neon.o memcpy.o $(INC_DIR)
#	$(C_COMPILER) -O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -o memcpy_test memcpy_test.c memcpy_neon.o memcpy.o $(INC_DIR)

lib:memcpy_neon.o
	$(CROSS_COMPILE)ar cr $(TLIB) $<

install:
	cp memcpy_test $(EXE_INSTALL_DIR)

clean:
	-rm -rf *.o $(ALL) core *.core *~ include/*~

