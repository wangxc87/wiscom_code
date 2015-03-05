SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

TARGET = libbsp4dm8168.so
CFLAGS += -I ../inc -I $(INC_DIR)

.PHONY: all clean install

all: ${OBJS}
	$(CC) -o $(TARGET) $(OBJS) --shared --fPIC $(CFLAGS)

clean:
	-rm -rf $(TARGET) *.o

install:
	cp $(TARGET) $(ROOTFS_DIR)/usr/lib/

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $<
