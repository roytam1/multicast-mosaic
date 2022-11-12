# Toplevel Makefile for mMosaic.

MCVER=3.3.3

# -------------------------- CUSTOMIZABLE OPTIONS ----------------------------

#### your compiler (ANSI required).
#CC = gcc
CC = CC
#CC = cc

# On BSDI, OSF1, IBM , SunOS4this should be ranlib.
# RANLIB = ranlib
# ranlib for SVR4, indy , linux Qnx
RANLIB = /bin/true

#### Linux
#prereleaseflags = -O3 -fomit-frame-pointer -funroll-loops -finline-functions -m486 -malign-double
#### Qnx
#prereleaseflags = -Oeax
#
#prereleaseflags = -O
#prereleaseflags = -xsb -v -g
#prereleaseflags = -DMDEBUG -v -g
prereleaseflags = +w -g
#prereleaseflags =  -v -g


#### Linker Flags -- Primarily for linking static on linux-elf.
#ldflags = -static
#### linux static
#ldflags = -s
#### QNX
#ldflags = -N256k
ldflags =

#### Random system configuration flags.
#### --> *** For Motif 1.2 ON ANY PLATFORM, do -DMOTIF1_2 *** <--
#### For IBM AIX 3.2, do -D_BSD
#### For NeXT, do -DNEXT
#### For HP/UX, do -Aa -D_HPUX_SOURCE -DMOTIF1_2
#### For Dell SVR4, do -DSVR4
#### For SunOS4 -DSUNOS
#### For Solaris, do -DSVR4 -DSOLARIS
#### For Esix 4.0.4 and Solaris x86 2.1, do -DSVR4
#### For Convex whatever, do -DCONVEX
#### For SCO ODT 3.0, do -DSCO -DSVR4 -DMOTIF1_2
#### For Motorola SVR4, do -DSVR4 -DMOTOROLA -DMOTIF1_2
#### For Linux -Dlinux  -DMOTIF1_2 -DSOLARIS25
#### For OSF1 -DMOTIF1_2 -std1 -DDECOSF1
#### For Qnx -DMOTIF1_2 -DQNX
sysconfigflags = -DSOLARIS -DSVR4 -DMOTIF1_2 -DSOLARIS25

#### System libraries SunOS.
# syslibs = -lPW -lsun -lmalloc
#### For AIX 3.2
# syslibs = -lPW -lbsd
#### For most other Motif platforms:
# syslibs = -lPW
#### For Sun's and Ultrix and HP and BSD/386:
# syslibs =
#### For Sun's with no DNS:
# syslibs = -lresolv
# syslibs = -lresolv -l44bsd
#### For SCO ODT:
# syslibs = -lPW -lsocket -lmalloc
#### For Dell SVR4:
# syslibs = -lnsl -lsocket -lc -lucb
#### For BDSI
# syslibs = -lipc
#### For QNX 4.XX
#syslibs = -lsocket
#### For Solaris (2.x) and Motorola SVR4  .
syslibs = -lnsl -lsocket -lgen

#### X include file locations -- if your platform puts the X include
#### files in a strange place, set this variable appropriately.  Else
#### don't worry about it.
#### HP X11R4 version:
# xinc = -I/usr/include/Motif1.1 -I/usr/include/X11R4
#### HP X11R5 version:
# xinc = -I/usr/include/Motif1.2 -I/usr/local/X11R5/include
#### NeXT version:
# xinc = -I/usr/include/X11
#### BSD/386
# xinc = -I/usr/X11/include
#### Solaris 2.x (Patched X11R5 and Motif libs)
xinc = -I/usr/openwin/include -I/usr/dt/include
#xinc = -I/usr/openwin/include -I$(HOME)/mbone-dev/mMosaic/lesstif-0.77/include

#### X library locations.
# xlibs = -lXm_s -lXmu -lXt_s -lX11_s
#### For Sun's (at least running stock X/Motif as installed on our machines):
# xlibs = /usr/lib/libXm.a /usr/lib/libXmu.a /usr/lib/libXt.a /usr/lib/libXext.a /usr/lib/libX11.a -lm
#### For HP-UX 8.00:
# xlibs = -L/usr/lib/Motif1.1 -lXm -L/usr/lib/X11R4 -lXmu -lXt -lX11
#### For HP-UX 9.01: The X11R5 libraries are here on our systems
# xlibs = -L/usr/lib/Motif1.2 -lXm -L/usr/lib/X11R5 -L/usr/lib/X11R4 -lXmu -lXt -lX11
#### For NeXT:
# xlibs = -L/usr/lib/X11 -lXm -lXmu -lXt -lX11
#### For Dell SVR4:
# xlibs = -L/usr/X5/lib -lXm -lXmu -lXt -lXext -lX11
#### For Solaris (2.x) (Use static to go from machine to machine)
xlibs = -R/usr/openwin/lib -L/usr/openwin/lib 
xlibs += -L/usr/dt/lib -R/usr/dt/lib -lXm
#xlibs += -L$(HOME)/mbone-dev/mMosaic/lesstif-0.77/libXm -lXm
xlibs += -lXmu -lXt
xlibs += -lXext -lX11
xlibs += -lm
#### For SCO ODT 3.0 (I'm told that -lXtXm_s is *not* a typo :-):
# xlibs = -lXtXm_s -lXmu -lX11_s
#### For nearly everyone else:
# xlibs = -lXm -lXmu -lXt -lX11
#### For BSD/386:
# xlibs = -L/usr/X11/lib -lXm -lXmu -lXt -lX11
#### X11R6 Linux
# xlibs = -L/usr/X11/lib -lXm -lXmu -lXt -lXext -lX11 -lSM -lICE -lXpm -lm
#### For Motorola SVR4:
# xlibs = -lXm -lXmu -lXt -lXext -lX11 -lm
#### QNX
#xlibs =  -L/usr/X11/lib -lXm_s -lXt_s -lX11_s -lXqnx_s  -lXt -lXmu -lXext

#### PNG SUPPORT
#### For inline PNG support, the following should be defined:
#### The libraries currently used are PNGLIB 0.99d and ZLIB 1.0.9

pngdir = /usr/local
pnglibdir = $(pngdir)/lib
pngincludedir = $(pngdir)/include
pnglibs = $(pnglibdir)/libpng.a $(pnglibdir)/libz.a -lm
pngflags =  -I$(pngincludedir) -DHAVE_PNG

#pngdir = $(HOME)/mbone-dev/mMosaic/libpng-0.89c
#pnglibdir = $(pngdir)
#pngincludedir = $(pngdir)
#pnglibs = $(pnglibdir)/libpng.a $(pnglibdir)/../zlib-1.0.4/libz.a -lm
#pngflags =  -I$(pngincludedir) -I$(pngincludedir)/../zlib-1.0.4 -DHAVE_PNG

#### JPEG SUPPORT
#### For inline JPEG support, the following should be defined:
#### The library used is Independent JPEG Group (IJG's) jpeg-6a.  

jpegdir = /usr/local
jpeglibs = $(jpegdir)/lib/libjpeg.a
jpegflags = -I$(jpegdir)/include -DHAVE_JPEG

#jpegdir = $(HOME)/mbone-dev/mMosaic/jpeg-6a
#jpeglibs = $(jpegdir)/libjpeg.a
#jpegflags = -I$(jpegdir) -DHAVE_JPEG

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
mcdir = $(PWD)/libmc
mcflag = -I$(mcdir) -DMULTICAST
mclib = $(mcdir)/libmc.a

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
	(cd libhtmlw; make CC=$(CC) RANLIB=$(RANLIB) CFLAGS="$(CFLAGS) $(xinc) -DMOTIF")

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

