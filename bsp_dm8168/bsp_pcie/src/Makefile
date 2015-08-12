SRCS = osa_pcie.c ti81xx_ep.c  ti81xx_rc.c
OBJS = $(SRCS:.c=.o)
OBJS += memcpy_neon.o

TARGET = libbsp4pcie.so
CFLAGS += -I ../inc -I${KERNELDIR}/include
CFLAGS += -O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp

.PHONY: all clean install

all: ${OBJS}
	$(CC) -o $(TARGET) $(OBJS) --shared --fPIC $(CFLAGS)

clean:
	-rm -rf $(TARGET) *.o

install:
	cp $(TARGET) $(ROOT)/usr/lib/

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $<

memcpy_neon.o: memcpy_neon.S
	$(CC) -I. -DFNAME=memcpy_neon -O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -c memcpy_neon.S
