# ##########################################################
#
#  Makefile SunOS5 (Solaris)
#  ENST - 2000 G.D.
#
# ##########################################################

PLUGINC=-I../../../libhtmlw
LDFLAGS=-G
CFLAGS =-KPIC -v -g
INSTALL_PLUG_DIR=/usr/local/mMosaic/plugins

all: gifview.so

gifview.o: gifview.c
	$(CC) -c $(CFLAGS) $(PLUGINC) $(XINC) -o gifview.o gifview.c

gifview.so: gifview.o 
	$(CC) $(LDFLAGS) -o gifview.so gifview.o 

clean:
	-@rm -f gifview.o gifview.so *.o core 

install: all
	cp gifview.so $(INSTALL_PLUG_DIR)/gifview.so
	chmod 755 $(INSTALL_PLUG_DIR)/gifview.so
