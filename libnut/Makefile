#
# Makefile for libnut (NCSA Unix Tools Library)
#

LIB=libnut.a
OBJ=str-tools.o system.o url-utils.o ellipsis.o list.o

all: $(LIB)

$(LIB): $(OBJ)
	-rm -f $(LIB)
	ar rv $(LIB) $(OBJ)
	$(RANLIB) $(LIB)


clean:
	rm -f $(LIB) $(OBJ)
	-rm -rf .sb
