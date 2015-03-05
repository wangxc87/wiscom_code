include Rules.make
CMEMLIB=cmem.a470uC
CROSS_COMPILE = $(CSTOOL_PREFIX)
#arm-none-linux-gnueabi-
KERNEL_DIR = $(LINUXKERNEL_INSTALL_DIR)
LINUXLIBS_DIR = $(LINUXLIBS_INSTALL_DIR)
FINAL_DEST=$(EXEC_DIR)/usr/share/ti/linux-driver-examples
CC = $(CROSS_COMPILE)gcc -g -I$(KERNEL_DIR)/include -I$(LINUXLIBS_DIR)/include
INSTALL_DIR=$(FINAL_DEST)/Myproject


EXE:=capture_video_aew
#SOURCES:=$(addsuffix .c,$(EXE))
#SOURCES :=$(wildcard *.c)
SOURCES :=capture_video_ss.c resizer.c preview.c display.c capture.c aew.c key_scan.c

OBJECTS:=$(patsubst %.c,%.o,$(SOURCES))

ALL:$(EXE)


$(EXE): $(OBJECTS) 
	$(CC) -o $(EXE) $(OBJECTS) $(CMEMLIB)

%.O:%.c
	$(CC) -c %< -o %@
install:
	mkdir -p 				$(INSTALL_DIR)
	cp $(EXE)	$(INSTALL_DIR)
clean:
	rm -f *.o *~ $(EXE) 
