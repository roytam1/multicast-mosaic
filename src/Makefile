All: $(MOSAIC)

LIBHTMLW_DIR = ../libhtmlw

PROGRAM_LIBS = $(LIBWWW_DIR)/libwww.a $(LIBHTMLW_DIR)/libhtmlw.a $(LIBNUT_DIR)/libnut.a $(LIBMC_DIR)/libmc.a
LIBS = $(PROGRAM_LIBS) $(X_LIBS) $(DTM_LIBS) $(HDF_LIBS) $(WAIS_LIBS) $(JPEG_LIBS) $(PNG_LIBS) $(KRB_LIBS) $(MATH_LIB) $(SYS_LIBS)

CFILES = main.c gui.c  gui-dialogs.c gui-menubar.c gui-documents.c gui-news.c\
  newsrc.c\
  gui-extras.c mo-www.c hotlist.c history.c\
  globalhist.c img.c\
  picread.c xpmhash.c xpmread.c gifread.c pixmaps.c\
  child.c mailto.c\
  accept.c cciServer.c cciBindings.c cciBindings2.c support.c bla.c\
  md5.c proxy-dialogs.c gui-popup.c gui-ftp.c\
  get_url_data.c readJPEG.c readPNG.c cache.c Xmx.c

OBJS = main.o gui.o gui-dialogs.o gui-menubar.o gui-documents.o gui-news.o\
  newsrc.o\
  gui-extras.o mo-www.o hotlist.o history.o\
  globalhist.o img.o\
  picread.o xpmhash.o xpmread.o gifread.o pixmaps.o\
  child.o mailto.o\
  accept.o cciServer.o cciBindings.o cciBindings2.o support.o bla.o\
  md5.o proxy-dialogs.o gui-popup.o gui-ftp.o\
  get_url_data.o readJPEG.o readPNG.o cache.o Xmx.o

globalhist.o: globalhist.c ../libhtmlw/HTML.h
	$(CC) -c $(CFLAGS) $(AUX_CFLAGS) globalhist.c

$(MOSAIC): $(OBJS) $(HDFOBJS) $(PROGRAM_LIBS) $(DTM_LIBS)
	$(PURIFY) $(CC) $(LDFLAGS) -o $(MOSAIC) $(OBJS) $(HDFOBJS) $(LIBS) 
	mv $(MOSAIC) mMosaic



HFILES = mosaic.h xresources.h ../libmc/mc_defs.h
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

wipe:
	-rm -f Mosaic Mosaic-p Mosaic-q $(OBJS) core
clean:
	-rm -f Mosaic Mosaic-p Mosaic-q *.o core mMosaic
	-rm -rf .sb
tags:
	etags -t *.[ch]
