# ##########################################################
#
#  Makefile SunOS5 (Solaris) / SunOS4 pour applications Motif
#  E. Lecolinet - ENST - Avril 1997
#
# ##########################################################

# Automatic update of dependencies (SunOS only)
#
# ##########################################################
# Programs

all: theplug.so theplugsock.so theplugfile.so


theplug.so: myplug.o pluginM.o
	$(CC) $(PLUGINC) $(LDFLAGS) -o myplug.so myplug.o pluginM.o	

theplugsock.so: myplugsock.o pluginM.o
	$(CC) $(PLUGINC) $(LDFLAGS) -o myplugsock.so myplugsock.o pluginM.o

theplugfile.so: myplugfile.o pluginM.o
	$(CC) $(PLUGINC) $(LDFLAGS) -o myplugfile.so myplugfile.o pluginM.o

clean:
	-@rm theplug theplugsock theplugfile *.o core 

