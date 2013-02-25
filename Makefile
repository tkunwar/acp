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
CFLAGS=-Wall -g -lcdk -lncurses -lpthread
LIBS=libconfig.a

#Uncomment the line below to compile on Mac
#LIBS=-liconv
all:acp
acp: acp.o acp_gui.o acp_common.o acp_config.o acp_gmm_utils.o acp_cmm_utils.o
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

%.o: %.c
	$(CC) -c $*.c $(CFLAGS)
docs:
	doxygen acp.doxyfile
run:
	ulimit -c unlimited
	./acp
debug:
	gdb acp core
clean:
	rm -f *.o *.out acp core
	rm -r -f doxygen-output
