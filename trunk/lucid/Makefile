CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)gcc
AS=$(GCC_EXEC_PREFIX)as

CFLAGS=-Wall -g -DARCH_i386 -DARCH_lucid -DENABLE_ASSERTS -Iinclude/ -I. 
LDFLAGS=$(CFLAGS)

DLIBOBJS=delibrium/dlib/memherd.o dlib/map.o dlib/bsearchtree.o delibrium/timer.o
LUCIDOBJS=lucid.o $(DLIBOBJS) pseudo/rant.o pseudo/soapbox.o pseudo/threads.o pseudo/ktimer.o
IPV4OBJS=ip_header.o ipv4.o tcp_test_server.o tcp.o lucidmain.o

default: slip

clean:
	rm -f *.o */*.o */*/*.o slip

slip: $(IPV4OBJS) $(LUCIDOBJS) slip.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ $(LIBS) -o $@

testfaults: lucid.o lucidmain.o
