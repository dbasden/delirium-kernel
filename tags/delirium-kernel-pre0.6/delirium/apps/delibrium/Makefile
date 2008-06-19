include ../Makefile.app

CFLAGS+=-I../../include

default: delibrium.o

delibrium.o: lib.o thread.o serial.o paging.o
	$(LD) -r $(LDFLAGS) $^ -o $@

clean:
	rm *.o || true 2>/dev/null
