All: $(MOSAIC)

LIBHTMLW_DIR = ../libhtmlw

PROGRAM_LIBS = $(LIBHTMLW_DIR)/libhtmlw.a $(LIBNUT_DIR)/libnut.a $(LIBMC)
LIBS = $(PROGRAM_LIBS) $(JPEG_LIBS) $(PNG_LIBS) $(X_LIBS) $(KRB_LIBS) $(PLUGLIB) $(MATH_LIB) $(SYS_LIBS)

CFILES = main.c gui.c  gui-dialogs.c gui-menubar.c gui-documents.c gui-news.c\
  newsrc.c gui-extras.c hotlist.c navigate.c globalhist.c img.c picread.c\
  xpmhash.c xpmread.c gifread.c pixmaps.c child.c mailto.c md5.c URLParse.c \
  telnet-proto.c compress.c proxy-dialogs.c gui-popup.c readJPEG.c readPNG.c \
  cache.c Xmx.c paf.c converter.c www-con.c file-proto.c http-proto.c MIME.c \
  ftp-proto.c util.c ho.c

OBJS = main.o gui.o gui-dialogs.o gui-menubar.o gui-documents.o gui-news.o\
  newsrc.o gui-extras.o hotlist.o navigate.o globalhist.o img.o picread.o\
  xpmhash.o xpmread.o gifread.o pixmaps.o child.o mailto.o md5.o URLParse.o\
  telnet-proto.o compress.o proxy-dialogs.o gui-popup.o readJPEG.o readPNG.o \
  cache.o Xmx.o paf.o converter.o www-con.o file-proto.o http-proto.o MIME.o\
  ftp-proto.o util.o ho.o

globalhist.o: globalhist.c ../libhtmlw/HTML.h
	$(CC) -c $(CFLAGS) $(AUX_CFLAGS) globalhist.c

$(MOSAIC): $(OBJS) $(PROGRAM_LIBS)
	$(CC) $(LDFLAGS) -o $(MOSAIC) $(OBJS) $(LIBS)

# XmIm.o


HFILES = mosaic.h xresources.h
$(OBJS): $(HFILES)
hotlist.o : hotlist.h
gui.o: toolbar.h xresources.h ../libhtmlw/HTML.h bitmaps/*.xbm
img.o: ../libhtmlw/HTML.h
gui-documents.o: toolbar.h
gui-popup.o: gui-popup.h
pixmaps.o: pixmaps/*.xpm
readJPEG.o readPNG.o picread.o: readJPEG.h readPNG.h picread.h
picread.o: readJPEG.h readPNG.h picread.h gifread.h xpmread.h
Xmx.o : Xmx.h

#$(PIXMAPS)

clean:
	-rm -f *.o core mMosaic
	-rm -rf .sb
tags:
	etags -t *.[ch]
