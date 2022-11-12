#
# Makefile for libmc (Multicast library)
#

CC=cc
CFLAGS=-g -O
LIB=libmc.a
#RANLIB=ranlib
OBJ= mc_misc.o mc_sockio.o mc_dispatch.o mc_action.o

all: $(LIB)

$(LIB): $(OBJ)
	-rm -f $(LIB)
	ar rv $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

mc_sockio.o: mc_sockio.c mc_sockio.h mc_defs.h mc_action.h ../libnut/mipcf.h
	$(CC) $(CFLAGS) -c mc_sockio.c -o mc_sockio.o

mc_misc.o: mc_misc.c mc_misc.h mc_defs.h mc_action.h
	$(CC) $(CFLAGS) -c mc_misc.c -o mc_misc.o

mc_dispatch.o : ../src/mo-www.h mc_defs.h mc_misc.h mc_sockio.h mc_dispatch.h ../libhtmlw/HTML.h mc_action.h

mc_action.o : mc_sockio.c mc_sockio.h mc_defs.h mc_action.h

clean:
	rm -f $(LIB) *.o core
	rm -rf .sb
