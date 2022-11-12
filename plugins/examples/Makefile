# ##########################################################
#
#  Makefile SunOS5 (Solaris)
#  ENST - 2000 G.D.
#
# ##########################################################

PLUGINC=-I$(PWD)/../../libhtmlw
LDFLAGS=-G
CFLAGS =-KPIC -v -g
#CFLAGS = -fpic
#LDLAGS = -shared
INSTALL_PLUG_DIR=/usr/local/mMosaic/plugins

all: rottext mtvp gifview helloworld
	@echo Making plugins done.

mtvp rottext gifview helloworld::
	@echo --- Building rottext.so
	(cd $@; make CC=$(CC) CFLAGS="$(CFLAGS)" PLUGINC="$(PLUGINC)" LDFLAGS="$(LDFLAGS)" INSTALL_PLUG_DIR="$(INSTALL_PLUG_DIR)")


clean:
	(cd rottext; make clean)
	(cd mtvp; make clean)
	(cd gifview; make clean)
	(cd helloworld; make clean)

install:
	-@ mkdir /usr/local/mMosaic/plugins
	chmod 755 /usr/local/mMosaic/plugins
	(cd rottext; make install)
	(cd mtvp; make install)
	(cd gifview; make install)
	(cd helloworld; make install)
