#
# Makefile for acp
CC=gcc

#The below line is for debugging
#CFLAGS=-I. -ggdb -Wall -D_FILE_OFFSET_BITS=64
CFLAGS=-Wall -g -lcdk -lncurses -lconfig

LIBS=

#Uncomment the line below to compile on Mac
#LIBS=-liconv
all:acp
acp: acp.o acp_gui.o acp_common.o acp_config.o
	$(CC) $(LIBS) -o $@ $^ $(CFLAGS)

%.o: %.c %.h
	$(CC) -c $*.c $(CFLAGS)

clean:
	rm -f *.o *.out acp
