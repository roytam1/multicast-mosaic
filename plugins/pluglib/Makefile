#########################################
## Makefile de la librairie PLUGIN     ##
#########################################

LIBTARGET	= libmMplug.a

all: $(LIBTARGET)

CFILES		= private.c public_struct1.c

OBJS = $(CFILES:.c=.o)
$(OBJS): private.h public_struct.h com_struct.h

$(LIBTARGET): $(OBJS)
	-rm -f $(LIBTARGET)
	ar rv $(LIBTARGET) $(OBJS)
	$(RANLIB) $(LIBTARGET)

clean:
	-rm -f $(LIBTARGET) *.o *.c~ *.h~ makefile~ core
	-rm -rf .sb
