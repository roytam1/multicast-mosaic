LIBTARGET = libhtmlw.a

all: $(LIBTARGET)

CFILES = HTMLtext.c HTML.c HTMLlists.c HTMLformat.c HTMLparse.c HTMLtable.c HTMLimages.c HTMLwidgets.c HTML-PSformat.c list.c HTMLmc.c HTMLaprog.c

OBJS = $(CFILES:.c=.o)
$(OBJS): HTML.h HTMLP.h HTMLimages.h HTMLPutil.h HTMLamp.h


$(LIBTARGET): $(OBJS)
	-rm -f $(LIBTARGET)
	ar rv $(LIBTARGET) $(OBJS)
	$(RANLIB) $(LIBTARGET)

HTMLimages.o: images/AnchoredImage.xbm images/DelayedImage.xbm

clean:
	-rm $(LIBTARGET) *.o
	-rm -rf .sb
tags:
	etags -t *.[ch]
