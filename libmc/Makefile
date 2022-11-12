#
# Makefile for libmc (Multicast library)
#

CC=cc
CFLAGS=-g -O
LIB=libmc.a
#RANLIB=ranlib
OBJ= mc_main.o mc_session.o mc_obj.o mc_io.o mc_misc.o mc_gui.o

all: $(LIB)

$(LIB): $(OBJ)
	-rm -f $(LIB)
	ar rv $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

mc_io.o: mc_io.c
	$(CC) $(CFLAGS) -c mc_io.c -o mc_io.o

mc_misc.o: mc_misc.c mc_misc.h mc_defs.h
	$(CC) $(CFLAGS) -c mc_misc.c -o mc_misc.o



clean:
	rm -f $(LIB) *.o core
	rm -rf .sb
