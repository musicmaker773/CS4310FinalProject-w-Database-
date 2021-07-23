#Write the Makefile for this directory. You should make sure the dependencies are in the right order.
#Here is a link for reference: https://opensource.com/article/18/8/what-how-makefile

CC=clang
CFLAGS= -g
COMMON_OBJS = common.o
SERVER_OBJS = myqueue.o

all: $(COMMON_OBJS) $(SERVER_OBJS) tcps

common.o: common.h common.c

myqueue.o: myqueue.h myqueue.c

tcps: tcpserver.c $(COMMON_OBJS) $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o tcps tcpserver.c -l sqlite3 $(COMMON_OBJS) $(SERVER_OBJS)

clean:
	rm -rf *.dSYM tcps