include Rules.make

CC=$(CODEGEN_PATH_A8)/bin/arm-none-linux-gnueabi-gcc
AR=$(CODEGEN_PATH_A8)/bin/arm-none-linux-gnueabi-ar
LD=$(CODEGEN_PATH_A8)/bin/arm-none-linux-gnueabi-gcc


INCLUDE=
INCLUDE+=$(KERNELDIR)

EXE_DIR :=$(TARGET_FS)/home/root/

#EXE := demo_serialport
#EXE := serialport
EXE := sony_test
all:
	$(CC) -o $(EXE) $(EXE).c

install:
	cp -f $(EXE) $(EXE_DIR) 

clean:
	rm *.o $(EXE)
