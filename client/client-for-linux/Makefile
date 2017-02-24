DEBUG = 0

BIN_NAME = tf
#CC = /opt/nova72461_arm_gcc-4.9.2/gcc-linaro-arm-linux-gnueabihf-4.9-2014.08_linux/bin/arm-linux-gnueabihf-gcc
CC = gcc

ifeq ($(DEBUG), 1)
LDFLAGS = -lpthread -g -rdynamic -DDEBUG
else
LDFLAGS = -lpthread
endif

SRC += tcp_push.c
SRC += tcp_download.c
SRC += ft-client.c

all:
	$(CC) $(SRC) -o $(BIN_NAME) $(LDFLAGS) -Wall

clean:
	rm $(BIN_NAME)
