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

all: helloworld.so


helloworld.o: helloworld.c
	$(CC) -c $(CFLAGS) $(PLUGINC) $(XINC) -o helloworld.o helloworld.c

helloworld.so: helloworld.o 
	$(CC) $(LDFLAGS) -o helloworld.so helloworld.o 

clean:
	-@rm -f helloworld.o helloworld.so *.o core 

install: all
	cp helloworld.so $(INSTALL_PLUG_DIR)/helloworld.so
	chmod 755 $(INSTALL_PLUG_DIR)/helloworld.so
