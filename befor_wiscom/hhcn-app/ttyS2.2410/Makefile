#CROSS = /LinuxPPC/CDK/bin/powerpc-linux-
CROSS = arm-none-linux-gnueabi-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
LDFLAGS += --static 

EXEC = ttytest
OBJS = ttytest.o rs232.o

CFLAGS += -DPARANOID_TTYS -DUSE_TERMIO -DKLUDGELINEMODE -D_GNU_SOURCE

all: $(EXEC)

#$(EXEC): $(OBJS)
$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS$(LDLIBS-$(@)))
install:
	cp $(EXEC) ~/dsp/DVRRDK_02.80.00.10/target/rfs_814x/home/root/app-test/


clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

#$(OBJS): rs232.h
