LIBTARGET = libhtmlw.a

all: $(LIBTARGET)

CFILES = HTMLcolors.c HTMLparse.c HTML.c HTMLformat.c HTMLtext.c HTMLlists.c HTMLtable.c \
	 HTMLimages.c HTMLwidgets.c HTML-PSformat.c list.c \
	 HTMLaprog.c HTMLapplet.c HTMLform.c HTMLframe.c HTMLfonts.c

OBJS = $(CFILES:.c=.o)
$(OBJS): HTML.h HTMLP.h HTMLPutil.h HTMLparse.h HTMLmiscdefs.h


$(LIBTARGET): $(OBJS)
	-rm -f $(LIBTARGET)
	ar rv $(LIBTARGET) $(OBJS)
	$(RANLIB) $(LIBTARGET)

clean:
	-rm -f $(LIBTARGET) *.o
	-rm -rf .sb
tags:
	etags -t *.[ch]
