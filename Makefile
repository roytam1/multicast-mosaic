# Toplevel Makefile for mMosaic.
#This Release 3.4.2 compile on:
#	- a Linux debian 2.1.8.1 sparc with Lesstif
#	- Solaris 2.5.1 sparc with Motif
#	- NetBSD 1.4 sparc
#	- FreeBSD3.1

MCVER=3.4.2

# -------------------------- CUSTOMIZABLE OPTIONS ----------------------------

#### your compiler (ANSI required). And options
#### Gnu C, C++, local C.
#CC = gcc
#CC = cc

CC = CC

#### Linux Intel optimised (does not work well)
#prereleaseflags = -O3 -fomit-frame-pointer -funroll-loops -finline-functions -m486 -malign-double
#### Standard gcc Linux FreeBSD NetBSD compiler flags (recommanded )
#prereleaseflags = -Wall -g -fwritable-strings  -O0
#### Qnx
#prereleaseflags = -Oeax
#### Sun Workshop C Compiler
#prereleaseflags = -xsb -v -g
#### Sun Workshop C++ Compiler

prereleaseflags = +w -g

#### On NetBSD, FreeBSD, BSDI, OSF1, IBM , SunOS4this should be ranlib.
#RANLIB = ranlib
#### ranlib for SVR4, indy , linux Qnx, Solaris2.x
RANLIB = /bin/true


#### Random system configuration flags.
#### --> *** For Motif 1.2 ON ANY PLATFORM, do -DMOTIF1_2 *** <--
#### For NeXT, do -DNEXT
#### For HP/UX, do -Aa -D_HPUX_SOURCE
#### For SCO ODT 3.0, do -DSCO -DSVR4
#### For Motorola SVR4 , Esix 4.0.4, do -DSVR4
#### For Linux 
#sysconfigflags = -DLINUX -Dlinux -DSVR4
#### For OSF1 -std1 -DDECOSF1
#### For Qnx  -DQNX
#### for FreeBSD
#sysconfigflags = -DFreeBSD -DMOTIF1_2
#### for NetBSD
#sysconfigflags = -DNETBSD -DMOTIF1_2
#### For Solaris2.5 & 2.5.1, do
sysconfigflags = -DSOLARIS -DSVR4 -DMOTIF1_2 -DSOLARIS25

#### Add some debug option for developper
#### -DHTMLTRACE for verbose about html tag
#sysconfigflags += -DHTMLTRACE

#### System libraries SunOS.
# syslibs = -lPW -lsun -lmalloc
#### For Sun's with no DNS:
# syslibs = -lresolv
# syslibs = -lresolv -l44bsd
#### For AIX 3.2
# syslibs = -lPW -lbsd
#### For most other Motif platforms:
# syslibs = -lPW
#### For Sun's and Ultrix and HP and BSD/386:
# syslibs =
#### For SCO ODT:
# syslibs = -lPW -lsocket -lmalloc
#### For Dell SVR4:
# syslibs = -lnsl -lsocket -lc -lucb
#### For BDSI
# syslibs = -lipc
#### For QNX 4.XX
#syslibs = -lsocket
#### For Solaris (2.x) and Motorola SVR4  .
syslibs = -lsocket -lnsl

#### X include file locations -- if your platform puts the X include
#### files in a strange place, set this variable appropriately.  Else
#### don't worry about it.
# xinc = -I/usr/include/Motif1.2 -I/usr/local/X11R5/include
# xinc = -I/usr/include/X11
# xinc = -I/usr/X11/include
#### Linux Debian
#xinc = -I/usr/X11R6/include
#FreeBSD
#xinc = -I/usr/X11R6/include -I$(HOME)/fpkg/include
#### Solaris 2.x (Patched X11R5 and Motif libs)

xinc = -I/usr/openwin/include -I/usr/dt/include

#### X library locations.
#### For nearly everyone:
# xlibs = -lXm -lXmu -lXt -lX11
#### For HP-UX 9.01: The X11R5 libraries are here on our systems
# xlibs = -L/usr/lib/Motif1.2 -lXm -L/usr/lib/X11R5 -L/usr/lib/X11R4 -lXmu -lXt -lX11
#### For NeXT:
# xlibs = -L/usr/lib/X11 -lXm -lXmu -lXt -lX11
#### For Dell SVR4:
# xlibs = -L/usr/X5/lib -lXm -lXmu -lXt -lXext -lX11
#### For Motorola SVR4:
# xlibs = -lXm -lXmu -lXt -lXext -lX11 -lm
#### QNX
#xlibs =  -L/usr/X11/lib -lXm_s -lXt_s -lX11_s -lXqnx_s  -lXt -lXmu -lXext
#### Linux Debian, FreeBSD, NetBSD with Lesstif
#xlibs = -L/usr/X11R6/lib -lXm -lXmu -lXt -lXext -lX11 -lm 
#### For Solaris (2.x) (Use static to go from machine to machine)
xlibs = -L/usr/openwin/lib -R/usr/openwin/lib -L/usr/dt/lib -R/usr/dt/lib
xlibs += -lXm -lXmu -lXt -lXext -lX11 -lm

#### PNG SUPPORT
#### For inline PNG support, the following should be defined:
#### The libraries currently used are PNGLIB 0.99d and ZLIB 1.0.9

zlibdir = /usr/local/lib
pngdir = /usr/local
pnglibdir = $(pngdir)/lib
pngincludedir = /$(pngdir)/include
pnglibs = $(pnglibdir)/libpng.a $(zlibdir)/libz.a
pngflags =  -I$(pngincludedir) -DHAVE_PNG

#### JPEG SUPPORT
#### For inline JPEG support, the following should be defined:
#### The library used is Independent JPEG Group (IJG's) jpeg-6a.  

jpegdir = /usr/local
jpeglibs = $(jpegdir)/lib/libjpeg.a
jpegflags = -I$(jpegdir)/include -DHAVE_JPEG

#### KERBEROS SUPPORT
####
#### If you want Mosaic to support Kerberos authentication, set the 
#### following flags appropriately.  You can support Kerberos V4 and/or V5,
#### although it's most likely that your realm supports one or the other.
#### To enable DES-encryption of HTTP messages via Kerberos key exchange, 
#### define the KRB-ENCRYPT flag.

##krb4dir   = /usr/athena
#krb4dir   = /xdev/mosaic/libkrb4/solaris-24
#krb4libs  = $(krb4dir)/lib/libkrb.a $(krb4dir)/lib/libdes.a
#krb4flags = -DKRB4 -I$(krb4dir)/include

##krb5dir   = /krb5
#krb5dir   = /xdev/mosaic/libkrb5/solaris-24
#krb5libs  = $(krb5dir)/lib/libkrb5.a $(krb5dir)/lib/libcrypto.a $(krb5dir)/util/et/libcom_err.a
#krb5flags = -DKRB5 -I$(krb5dir)/include -I$(krb5dir)/include/krb5

#Do not comment out.
krbflags  = $(krb4flags) $(krb5flags)
krblibs   = $(krb4libs) $(krb5libs) 

#### MULTICAST support
#mcdir = $(PWD)/libmc
#mcflag = -I$(mcdir) -DMULTICAST
#mclib = $(mcdir)/libmc.a

#### APROG support
#adir = $(PWD)/libaprog
#aflag = -I$(adir) -DAPROG
#alib = $(adir)/libaprog.a

#### Customization flags:
#### . If you want Mosaic to come up with monochrome colors by default,
####   use -DMONO_DEFAULT
#### . If you want to define the default Mosaic home page, set
####   -DHOME_PAGE_DEFAULT=\\\"url\\\"
#### . If you want to define the default Mosaic documentation directory
####   (should be a URL), set -DDOCS_DIRECTORY_DEFAULT=\\\"url\\\"
#### . Other things you can define are spelled out in src/mosaic.h.
####
#### . If you run on IPV6 native kernel set -DIPV6
#### . For NEWS support -DNEWS
#### . For APPLET support -DAPPLET

customflags =


# ---------------------- END OF CUSTOMIZABLE OPTIONS -------------------------


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


clean:
	cd libhtmlw; $(MAKE) clean
	cd libnut; $(MAKE) clean
	cd libmc; $(MAKE) clean
	cd src; $(MAKE) clean MOSAIC="Mosaic"
	cd libaprog; $(MAKE) clean
	rm -f core
	@echo "Done cleaning..."

archive:
	$(MAKE) clean
	(cd ..; tar cf /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar ./mMosaic-src-$(MCVER) )
	gzip -9 /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar
	chmod 444 /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar.gz
	- rm /tsi/archive/ftp/pub/multicast/mMosaic/mMosaic-src.tar.gz
	(cd /tsi/archive/ftp/pub/multicast/mMosaic; ln -s mMosaic-$(MCVER).tar.gz mMosaic-src.tar.gz)

archive-bin :
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

