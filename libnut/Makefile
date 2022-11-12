#
# Makefile for libnut (NCSA Unix Tools Library)
#

CC=gcc
CFLAGS=-g -O
LIB=libnut.a
OBJ=str-tools.o system.o url-utils.o ellipsis.o

all: $(LIB)

$(LIB): $(OBJ)
	-rm -f $(LIB)
	ar rv $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

str-tools.o: str-tools.c
	$(CC) $(CFLAGS) -c str-tools.c -o str-tools.o

system.o: system.c system.h
	$(CC) $(CFLAGS) -c system.c -o system.o

url-utils.o: url-utils.c url-utils.h
	$(CC) $(CFLAGS) -c url-utils.c -o url-utils.o

ellipsis.o: ellipsis.c
	$(CC) $(CFLAGS) -c ellipsis.c -o ellipsis.o

clean:
	rm -f $(LIB) $(OBJ)
	-rm -rf .sb
