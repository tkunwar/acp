#
# Makefile for acp
CC=gcc

#The below line is for debugging
#CFLAGS=-I. -ggdb -Wall -D_FILE_OFFSET_BITS=64
# when linking with libconfig in shared mode
# use this 
# CFLAGS=-Wall -g -lcdk -lncurses -lconfig
# when using libconfig in static linking mode 
# use this
CFLAGS=-Wall -g -lcdk -lncurses
LIBS=libconfig.a

#Uncomment the line below to compile on Mac
#LIBS=-liconv
all:acp
acp: acp.o acp_gui.o acp_common.o acp_config.o
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

%.o: %.c %.h
	$(CC) -c $*.c $(CFLAGS)
docs:
	doxygen acp.doxyfile
clean:
	rm -f *.o *.out acp
	rm -r -f doxygen-output
