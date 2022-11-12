##
## Toplevel Makefile for all Makefiles
##

##
## Scott Powers
##

MCVER=3.2.0

all: dev_$(DEV_ARCH)

list: dev_
help: dev_
dev_::
	@echo "You must specify one of the following or set the environment variable"
	@echo "'DEV_ARCH' to one of the following:"
	@echo "  alpha -- DEC Alpha AXP running OSF/1"
	@echo "  osf1 -- DEC Alpha AXP running OSF/1"
	@echo "  bsdi -- x86 running BSDI BSD/OS 2.1"
	@echo "  dec -- DEC 2100 running Ultrix 4.3"
	@echo "  ultrix -- DEC 2100 running Ultrix 4.4"
	@echo "  hp -- HP 9000/735 running HP-UX A.09.01"
	@echo "  ibm -- IBM RS6000 running AIX 4.4 BSD"
	@echo "  indy -- SGI Indy running IRIX 5.3"
	@echo "  linux -- x86 running Linux 1.2.13 DYNAMIC"
	@echo "  linux-static -- x86 running Linux 1.2.13 ALL STATIC"
	@echo "  linux-static-motif -- x86 running Linux 1.2.13 STATIC MOTIF"
	@echo "  QNX  running QNX4.23A"
	@echo "  sgi -- SGI Iris running IRIS 4.0.2"
	@echo "  solaris-23 -- SPARCstation 20 running Solaris 2.3"
	@echo "  solaris-24 -- SPARCstation 20 running Solaris 2.4"
	@echo "  solaris-25 -- SPARCstation 20 running Solaris 2.5"
	@echo "  solaris-25-ipv6 -- SPARCstation 20 running Solaris 2.5 IPV6 kernel"
	@echo "  solaris-24-x86 -- x86 running Solaris 2.4 for x86"
	@echo "  sun -- SPARCserver 690MP running SunOS 4.1.3"
	@echo "  sun-lresolv -- SPARCserver 690MP running SunOS 4.1.3"
	@echo " "
	@echo "If your OS is not listed, you will need to copy one of the"
	@echo "  the Makefiles.OS in the 'makefiles' directory, edit it for"
	@echo "  your system, edit this Makefile to add your system, compile,"
	@echo "  and send in your changes to: mMosaic-dev@sig.enst.fr"

dev_alpha: alpha
alpha:
	$(MAKE) -f makefiles/Makefile.alpha

dev_osf1: osf1
osf1:
	$(MAKE) -f makefiles/Makefile.osf1

dev_bsdi: bsdi
bsdi:
	$(MAKE) -f makefiles/Makefile.bsdi

dev_dec: dec
dec:
	$(MAKE) -f makefiles/Makefile.dec

dev_ultrix: ultrix
ultrix:
	$(MAKE) -f makefiles/Makefile.ultrix

dev_hp: hp
hp:
	$(MAKE) -f makefiles/Makefile.hp

dev_ibm: ibm
ibm:
	$(MAKE) -f makefiles/Makefile.ibm

dev_indy: indy
indy:
	$(MAKE) -f makefiles/Makefile.indy

dev_linux: linux
linux:
	$(MAKE) -f makefiles/Makefile.linux

dev_linux_static: linux_static
linux_static:
	$(MAKE) -f makefiles/Makefile.linux staticd

dev_linux_static_motif: linux_static_motif
linux_static_motif:
	$(MAKE) -f makefiles/Makefile.linux static_motifd

dev_qnx: qnx
qnx:
	$(MAKE) -f makefiles/Makefile.qnx

dev_sgi: sgi
sgi:
	$(MAKE) -f makefiles/Makefile.sgi

dev_solaris-23: solaris-23
solaris-23:
	$(MAKE) -f makefiles/Makefile.solaris-23

dev_solaris-24: solaris-24
solaris-24:
	$(MAKE) -f makefiles/Makefile.solaris-24

dev_solaris-25: solaris-25
solaris-25:
	$(MAKE) -f makefiles/Makefile.solaris-25

dev_solaris-25-ipv6: solaris-25-ipv6
solaris-25-ipv6:
	$(MAKE) -f makefiles/Makefile.solaris-25-ipv6

dev_solaris-24-x86: solaris-24-x86
solaris-24-x86:
	$(MAKE) -f makefiles/Makefile.solaris-24-x86

dev_sun: sun
sun:
	$(MAKE) -f makefiles/Makefile.sun

dev_sun-lresolv: sun-lresolv
sun-lresolv:
	$(MAKE) -f makefiles/Makefile.sun-lresolv

clean:
	cd libXmx; $(MAKE) clean
	cd libhtmlw; $(MAKE) clean
	cd libnut; $(MAKE) clean
	cd libwww2; $(MAKE) clean
	cd libmc; $(MAKE) clean
	cd src; $(MAKE) clean MOSAIC="Mosaic"
	cd libaprog; $(MAKE) clean
	rm -f core
	@echo "Done cleaning..."

archive:
	$(MAKE) clean
	(cd ..; tar cf /sig/ultimate1/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar ./mMosaic-src-$(MCVER) )
	gzip -9 /sig/ultimate1/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar
	chmod 444 /sig/ultimate1/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER).tar.gz
	- rm /sig/ultimate1/ftp/pub/multicast/mMosaic/mMosaic-src.tar.gz
	(cd /sig/ultimate1/ftp/pub/multicast/mMosaic; ln -s mMosaic-$(MCVER).tar.gz mMosaic-src.tar.gz)

archive-bin :
	- cp src/mMosaic /sig/ultimate1/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER)-bin-solaris-25
	- gzip -9 /sig/ultimate1/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER)-bin-solaris-25
	chmod 444 /sig/ultimate1/ftp/pub/multicast/mMosaic/mMosaic-$(MCVER)-bin-solaris-25.gz
	- rm /sig/ultimate1/ftp/pub/multicast/mMosaic/mMosaic-solaris25-bin.gz
	(cd /sig/ultimate1/ftp/pub/multicast/mMosaic; ln -s mMosaic-$(MCVER)-bin-solaris-25.gz mMosaic-solaris25-bin.gz)
