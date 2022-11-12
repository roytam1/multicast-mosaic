#
# Toplevel Makefile for mMosaic.
#
#This Release 3.5.5 compile on:
#	- a Linux debian 2.1.8.1 sparc with Lesstif
#	- Solaris 2.5.1 sparc with Motif
#	- NetBSD 1.4 sparc
#	- FreeBSD3.1 ,FreeBSD5.0
#	- Mandrake6.1
#	- NetWinder (StrongArm based machine)
#	- SGI Irix 6.5 (MIPSPro)

MCVER=3.6.0

##
## -------------------------- CUSTOMIZABLE OPTIONS ----------------------------
##

##
## Your compiler (ANSI required)
##

#CC = gcc
#CC = cc
CC = CC

##
## Linker options
##

# For building a shared lib (not recommanded)
#ldflags = -G
#ldflags = -shared
#ldflags = -Bshared
# For building a static lib
#ldflags = -static
#ldflags = -Bstatic

##
## Prerelease flags
##

# GCC : Linux Intel optimised (does not work well)
#prereleaseflags = -O3 -fomit-frame-pointer -funroll-loops -finline-functions -m486 -malign-double
# GCC : Linux, FreeBSD, NetBSD compiler flags (recommended)
# prereleaseflags = -Wall -g
# Qnx
#prereleaseflags = -Oeax
# SGI Mipspro
#prereleaseflags = -g -n32 -Xcpluscomm -woff 1009,1014,1048,1110,1116,1185,1188,1204,1230,1233
# Sun Workshop C Compiler
#prereleaseflags = -v -g -xstrconst
# Sun Workshop C++ Compiler
prereleaseflags = +w -g

# For building a shared lib add this	(don't use: experimental)
#prereleaseflags = $prereleaseflags -fPIC

##
## Ranlib
##

# Linux, NetBSD, FreeBSD, BSDI, OSF1, SunOS4this 
#RANLIB = ranlib
# SVR4, Irix, AIX, Qnx, Solaris 2.x
RANLIB = /bin/true

##
## System configuration flags
##

# NeXT
#sysconfigflags = -DNEXT -DBSD
# HP/UX
#sysconfigflags = -Aa -D_HPUX_SOURCE
# SCO ODT 3.0
#sysconfigflags = -DSCO -DSVR4
# Motorola SVR4, Esix 4.0.4
#sysconfigflags = -DSVR4
# Linux
#sysconfigflags = -DLINUX -Dlinux -DSVR4
# OSF1
#sysconfigflags = -std1 -DDECOSF1
# Qnx
#sysconfigflags = -DQNX
# FreeBSD
#sysconfigflags = -DFreeBSD
# NetBSD
#sysconfigflags = -DNETBSD
# Irix
#sysconfigflags = -DSVR4 -DIRIX
# Solaris2.5 & 2.5.1
sysconfigflags = -DSOLARIS -DSVR4

# Use Motif 1.2.x
sysconfigflags += -DMOTIF1_2

# -DHTMLTRACE for verbose about html tag
#sysconfigflags += -DHTMLTRACE

##
## System libraries
##

# SunOS
#syslibs = -lPW -lsun -lmalloc
# SunOS without DNS
#syslibs = -lresolv
#syslibs = -lresolv -l44bsd
# AIX 3.2
#syslibs = -lPW -lbsd
# Ultrix, HP-UX, BSD/386, FreeBSD, Linux
#syslibs =
# SCO ODT
#syslibs = -lPW -lsocket -lmalloc
# Dell SVR4
#syslibs = -lnsl -lsocket -lc -lucb
# BSDi
#syslibs = -lipc
# QNX 4.XX
#syslibs = -lsocket
# Irix
#syslibs = -lPW
# Solaris 2.x, Motorola SVR4
syslibs = -lsocket -lnsl

##
## X11 includes
##

# Common
#xinc	= -I/usr/include/Motif1.2 -I/usr/local/X11R5/include
#xinc	= -I/usr/include/X11
#xinc	= -I/usr/X11/include
# Linux, FreeBSD
#xinc	= -I/usr/X11R6/include
# Irix
#xinc	= -I/usr/include
# Solaris 2.x
xinc	= -I/usr/openwin/include -I/usr/dt/include


##
## X11 libraries
##

# Common
#xlibs	= -L/usr/lib/X11 -lXm -lXmu -lXt -lX11
# HP-UX
#xlibs	= -L/usr/lib/Motif1.2 -lXm -L/usr/lib/X11R5 -L/usr/lib/X11R4 -lXmu -lXt -lX11
# NeXT
#xlibs	= -L/usr/lib/X11 -lXm -lXmu -lXt -lX11
# Dell SVR4
#xlibs	= -L/usr/X5/lib -lXm -lXmu -lXt -lXext -lX11
# Motorola SVR4
#xlibs	= -lXm -lXmu -lXt -lXext -lX11 -lm
# QNX
#xlibs	= -L/usr/X11/lib -lXm_s -lXt_s -lX11_s -lXqnx_s -lXt -lXmu -lXext
# Linux, FreeBSD, NetBSD with Lesstif
#xlibs	= -L/usr/X11R6/lib -lXm -lXmu -lXt -lXext -lX11 -lm 
# Irix
#xlibs	= -L/usr/lib32 -lXm -lXmu -lXt -lXext -lX11 -lm 
# Solaris 2.x
xlibs	= -L/usr/openwin/lib -R/usr/openwin/lib -L/usr/dt/lib -R/usr/dt/lib
xlibs	+= -lXm -lXmu -lXt -lXext -lX11 -lm

##
## PNG support (PNGLIB 0.99d and ZLIB 1.0.9)
##

# Linux
#pnginc  = /usr/include
#pnglibs = /usr/lib/libpng.a /usr/lib/libz.a
# Irix 
#pnginc   = /usr/local/include
#pnglibs  = /usr/local/lib/libpng.a /usr/lib32/libz.a
# FreeBSD
#pnginc  = /usr/local/include
#pnglibs = /usr/local/lib/libpng.a /usr/lib/libz.a
# Solaris
pnginc  = /usr/local/include
pnglibs = /usr/local/lib/libpng.a /usr/local/lib/libz.a

# To disable PNG support comment this.

pngflags =  -I$(pnginc) -DHAVE_PNG

##
## JPEG support (jpeg-6a)
##

# Linux
#jpeginc  = /usr/include
#jpeglibs = /usr/lib/libjpeg.a
# Irix
#jpeginc   = /usr/include
#jpeglibs  = /usr/lib32/libjpeg.so
# Solaris FreeBSD
jpeginc  = /usr/local/include
jpeglibs = /usr/local/lib/libjpeg.a

# To disable JPEG support (not recommended) comment this.

jpegflags = -I$(jpeginc) -DHAVE_JPEG

##
## KERBEROS support
##

# If you want Mosaic to support Kerberos authentication, set the 
# following flags appropriately.  You can support Kerberos V4 and/or V5,
# although it's most likely that your realm supports one or the other.
# To enable DES-encryption of HTTP messages via Kerberos key exchange, 
# define the KRB-ENCRYPT flag.

#krb4dir   = /usr/athena
#krb4dir   = /xdev/mosaic/libkrb4/solaris-24
#krb4libs  = $(krb4dir)/lib/libkrb.a $(krb4dir)/lib/libdes.a
#krb4flags = -DKRB4 -I$(krb4dir)/include

#krb5dir   = /krb5
#krb5dir   = /xdev/mosaic/libkrb5/solaris-24
#krb5libs  = $(krb5dir)/lib/libkrb5.a $(krb5dir)/lib/libcrypto.a $(krb5dir)/util/et/libcom_err.a
#krb5flags = -DKRB5 -I$(krb5dir)/include -I$(krb5dir)/include/krb5

# Do not comment out!
krbflags  = $(krb4flags) $(krb5flags)
krblibs   = $(krb4libs) $(krb5libs) 

##
## MULTICAST support
## 

mcdir = $(PWD)/libmc
mcflag = -I$(mcdir) -DMULTICAST
mclib = $(mcdir)/libmc.a

##
## APROG support (obsolete)
## 

#adir = $(PWD)/libaprog
#aflag = -I$(adir) -DAPROG
#alib = $(adir)/libaprog.a

##
## Customization flags
## 

# -DMONO_DEFAULT			Mosaic comes up in monochrome default
# -DHOME_PAGE_DEFAULT=\\\"url\\\"	Define Mosaic home page
# -DDOCS_DIRECTORY_DEFAULT=\\\"url\\\"  Define Mosaic documentation directory
# -DIPV6				IPV6 native kernel (don't mix with MULTICAST)
# -DNEWS				News support (obsolete) 
# -DAPPLET				Applet support (don't use!)
# -DLOOSE_PACKET			Testing multicast packet loss
# -DDEBUG_MULTICAST			Debug MULTICAST
# -DDEBUG_FRAME				Be verbose on frames
# -DOBJECT				Experimental: implement plugin! (not yet)
# -DNDEBUG				don't use assert.
#
# Other things you can define are spelled out in src/mosaic.h

# Debugging
#customflags = -DLOOSE_PACKET -DDEBUG_MULTICAST -DDEBUG_FRAME -DOBJECT
#
# Common

customflags = -DOBJECT

##
## ---------------------- END OF CUSTOMIZABLE OPTIONS -------------------------
##

CFLAGS = $(sysconfigflags) $(prereleaseflags) $(mcflag) $(aflag) $(customflags)

all: libhtmlw libnut $(mclib) $(alib) src
	@echo \*\*\* Welcome to mMosaic.

libhtmlw::
	@echo --- Building libhtmlw
	(cd libhtmlw; make CC=$(CC) RANLIB=$(RANLIB) CFLAGS="$(CFLAGS) $(xinc)" )

libnut::
	@echo --- Building libnut
	(cd libnut; make CC=$(CC) RANLIB=$(RANLIB) CFLAGS="$(CFLAGS)")

$(mclib) ::
	@echo --- Building libmc
	(cd libmc; make CC=$(CC) RANLIB=$(RANLIB) CFLAGS="$(CFLAGS) $(xinc)")

$(alib) ::
	@echo --- Building libaprog
	(cd libaprog; make CC=$(CC) RANLIB=$(RANLIB) CFLAGS="$(CFLAGS) $(xinc)" X_LIBS="$(xlibs)" SYS_LIBS="$(sockslibs) $(syslibs)")
 

src::
	@echo --- Building src
	(cd src; make CC=$(CC) RANLIB=$(RANLIB) LDFLAGS="$(ldflags)" CFLAGS="$(CFLAGS) $(xinc) $(jpegflags) $(pngflags) $(krbflags) -I.." X_LIBS="$(xlibs)" SYS_LIBS="$(sockslibs) $(syslibs)" JPEG_LIBS="$(jpeglibs)" PNG_LIBS="$(pnglibs)" KRB_LIBS="$(krblibs)" LIBNUT_DIR=../libnut LIBMC=$(mclib) MOSAIC="mMosaic")


install: all
	- strip src/mMosaic
	- cp src/mMosaic /usr/local/bin/mMosaic	
	- chmod 755 /usr/local/bin/mMosaic
	- mkdir /usr/local/mMosaic
	- mkdir /usr/local/mMosaic/plugins
	- cp plugins/* /usr/local/mMosaic/plugins
	- chmod 755 /usr/local/mMosaic/plugins/*

clean:
	cd libhtmlw; $(MAKE) clean
	cd libnut; $(MAKE) clean
	cd libmc; $(MAKE) clean
	cd src; $(MAKE) clean MOSAIC="Mosaic"
	rm -f *.core core
	@echo "Done cleaning..."

archive:
	$(MAKE) clean
	(cd ..; tar cf /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar ./mMosaic-src-$(MCVER) )
	gzip -9 /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar
	chmod 444 /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar.gz
	- rm /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-src.tar.gz
	(cd /tsi/archive/ftp/pub/multicast/mMosaic; ln -s mMosaic-$(MCVER).tar.gz mMosaic-src.tar.gz)

archive-bin :
	- strip src/mMosaic
	- cp src/mMosaic /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER)-bin-solaris-25
	- gzip -9 /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER)-bin-solaris-25
	chmod 444 /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER)-bin-solaris-25.gz
	- rm /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-solaris25-bin.gz
	(cd /tsi/archive/ftp/pub/multicast/mMosaic; ln -s mMosaic-$(MCVER)-bin-solaris-25.gz mMosaic-solaris25-bin.gz)

snap :
	$(MAKE) clean
	(cd ..; tar cf /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-snap.tar ./mMosaic-src-$(MCVER) )
	gzip -9 /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-snap.tar
	chmod 444 /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-snap.tar.gz

