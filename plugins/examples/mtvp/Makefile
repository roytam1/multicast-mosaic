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

all: mtvp.so


mtvp.o: mtvp.c
	$(CC) -c $(CFLAGS) $(PLUGINC) $(XINC) -o mtvp.o mtvp.c

mtvp.so: mtvp.o 
	$(CC) $(LDFLAGS) -o mtvp.so mtvp.o 

clean:
	-@rm -f mtvp.o mtvp.so *.o core 

install: all
	cp mtvp.so $(INSTALL_PLUG_DIR)/mtvp.so
	chmod 755 $(INSTALL_PLUG_DIR)/mtvp.so
