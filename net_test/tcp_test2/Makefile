dvr_rdk_BASE     :=/home/wxc/dsp/DVRRDK_04.01.00.02
TARGET_FS     := $(dvr_rdk_BASE)/target/rfs_816x
EXE_DIR :=$(TARGET_FS)/home/root/

COMPILER_PREFIX = arm-arago-linux-gnueabi
CROSS_COMPILE = $(COMPILER_PREFIX)-


CC=arm-arago-linux-gnueabi-gcc

server_src = server_tcp2.c
server_obj = $(server_src: %.c = %.o)
SERVER_EXE = $(basename $(server_src))

client_src = client_tcp2.c
client_obj = $(client_src: %.c = %.o)
CLIENT_EXE = $(basename $(client_src))

EXE = $(CLIENT_EXE) $(SERVER_EXE)

.PHONY:all clean install

ALL:$(EXE)

$(EXE): $(client_src) $(server_src)
	$(CC) $(client_src) -o $(CLIENT_EXE)
#	gcc $(client_src) -o $(CLIENT_EXE)
	gcc $(server_src) -o $(SERVER_EXE)

%.o:%.c
	$(CC) -c $< -o $@

install:
	cp -f $(CLIENT_EXE) $(EXE_DIR) 

clean:
	rm *.o $(EXE)
