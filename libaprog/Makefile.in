#
# Makefile for libmc (Multicast library)
#

CC=cc
CFLAGS=-g -O
LIB=libaprog.a
#RANLIB=ranlib
OBJ= aprog.o

all: $(LIB) test_aprog

$(LIB): $(OBJ)
	-rm -f $(LIB)
	ar rv $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

aprog.o: aprog.c aprog.h
	$(CC) $(CFLAGS) -c aprog.c -o aprog.o

test_aprog: test_aprog.c aprog.h $(LIB)
	$(CC) $(CFLAGS) test_aprog.c -o test_aprog $(LIB) $(X_LIBS) $(SYS_LIBS)


clean:
	rm -f $(LIB) *.o test_aprog
	rm -rf .sb

