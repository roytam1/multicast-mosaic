LIBTARGET = libwww.a

all: $(LIBTARGET)

CFILES = \
CUkerb.c \
HTAABrow.c \
HTAAUtil.c \
HTAccess.c \
HTAlert.c \
HTAnchor.c \
HTAssoc.c \
HTAtom.c \
HTCompressed.c \
HTFile.c \
HTFTP.c \
HTFWriter.c \
HTFormat.c \
HTGopher.c \
HTIcon.c \
HTInit.c \
HTList.c \
HTMIME.c \
HTMosaicHTML.c \
HTML.c \
HTNews.c \
HTParse.c \
HTPlain.c \
HTString.c \
HTTCP.c \
HTTP.c \
HTTelnet.c \
HTWSRC.c \
HTWAIS.c \
HTSort.c \
HTUU.c \
HTMailto.c

OBJS = $(CFILES:.c=.o)

$(LIBTARGET): $(OBJS)
	-rm -f $(LIBTARGET)
	ar rv $(LIBTARGET) $(OBJS)
	$(RANLIB) $(LIBTARGET)

$(OBJS): HTUtils.h
HTFTP.o: HTFTP.h

clean:
	-rm $(LIBTARGET) *.o
	-rm -rf .sb
tags:
	etags -t *.[ch]
