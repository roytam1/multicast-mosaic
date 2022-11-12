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

all: rottext.so


rottext.o: rottext.c
	$(CC) -c $(CFLAGS) $(PLUGINC) $(XINC) -o rottext.o rottext.c

rottext.so: rottext.o 
	$(CC) $(LDFLAGS) -o rottext.so rottext.o 

clean:
	-@rm -f rottext.o rottext.so *.o core 

install: all
	cp rottext.so $(INSTALL_PLUG_DIR)/rottext.so
	chmod 755 $(INSTALL_PLUG_DIR)/rottext.so
