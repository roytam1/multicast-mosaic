PROGRAMS = hello aprog_hello

CFLAGS= -g -I/usr/dt/include -I/usr/openwin/include
LIBS= -L/usr/dt/lib -R/usr/dt/lib -L/usr/openwin/lib -R/usr/openwin/lib -lXm -lXt -lX11

all: $(PROGRAMS)

.c: ; $(CC) $(CFLAGS) $@.c $(LIBS) -o $@

