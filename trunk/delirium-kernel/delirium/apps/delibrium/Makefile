include ../Makefile.app

CFLAGS+=-I../../include

DELIBRIUM_OBJS=lib.o thread.o serial.o paging.o pool.o
DLIB_OBJS=memherd.o

default: delibrium.o libdelibrium.a

memherd.o:
	gcc $(CFLAGS) dlib/memherd.c -c -o $@

# A .o file with everything if you want the bloat.
#
delibrium.o: $(DELIBRIUM_OBJS) $(DLIB_OBJS)
	$(LD) -r $(LDFLAGS) $^ -o $@

# An archive if you don't
#
libdelibrium.a: $(DELIBRIUM_OBJS) $(DLIB_OBJS)
	rm -f $@
	$(AR) -rc $@ $^ 
	ranlib $@

clean:
	rm -f *.o libdelibrium.a || true 2>/dev/null
