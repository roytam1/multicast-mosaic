#
# Makefile for libnut (NCSA Unix Tools Library)
#

CFLAGS=-g -O
LIB=libnut.a
OBJ=str-tools.o system.o url-utils.o ellipsis.o list.o

all: $(LIB)

$(LIB): $(OBJ)
	-rm -f $(LIB)
	ar rv $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

str-tools.o: str-tools.c
	$(CC) $(CFLAGS) -c str-tools.c

system.o: system.c system.h
	$(CC) $(CFLAGS) -c system.c

url-utils.o: url-utils.c url-utils.h
	$(CC) $(CFLAGS) -c url-utils.c

ellipsis.o: ellipsis.c
	$(CC) $(CFLAGS) -c ellipsis.c

clean:
	rm -f $(LIB) $(OBJ)
	-rm -rf .sb
