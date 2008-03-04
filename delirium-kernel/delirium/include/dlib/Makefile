CFLAGS=-Wall -g 

all: test_cqueue test_bsearchtree test_map test_memherd test_queue test_bitvec

test_bsearchtree: bsearchtree.o

test_map: map.o bsearchtree.o

test_memherd: memherd.o

test_queue: memherd.o

clean:
	rm *.o 2>/dev/null || true
	rm test_cqueue test_bsearchtree test_map test_memherd test_queue || true
	rm test_bitvec || true

test: all
	./test_cqueue
	./test_bsearchtree
	./test_map
	./test_memherd
	./test_queue
	./test_bitvec

# DO NOT DELETE

bsearchtree.o: btree.h bsearchtree.h
map.o: map.h bsearchtree.h btree.h
memherd.o: bitvec.h memherd.h
test_bsearchtree.o: bsearchtree.h btree.h
test_cqueue.o: cqueue.h
test_map.o: map.h bsearchtree.h btree.h
test_memherd.o:  memherd.h bitvec.h
test_queue.o: queue.h
