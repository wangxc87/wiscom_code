INC=./
TLIB=lib_ti81xx.a
TLIB_MEMCPY=lib_memcpy_neon.a

SRC = $(wildcard *.c)
OBJS = $(SRC:%.c= %.o)
INCLUDE_FILE = $(wildcard ./*.h)

INTCFLAGS=-I$(INC) 


all: $(TLIB) $(TLIB_MEMCPY)

#.c.o:
#	$(CC)gcc -c $<  $(INTCFLAGS) $(CFLAGS) 
%.o:%.c
	$(CC)gcc -c $(INTCFLAGS) $(CFLAGS) $< -o $@ 

$(OBJS): $(INCLUDE_FILE)

$(TLIB): $(OBJS)
	$(CC)ar cr $(TLIB) $(OBJS)

$(TLIB_MEMCPY):memcpy_neon.o
	$(CC)ar cr $(TLIB_MEMCPY) $<

memcpy_neon.o: memcpy_neon.S
	$(CC)gcc -I. -DFNAME=memcpy_neon -O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -c memcpy_neon.S

clean:
	-rm -f *.o *.a

