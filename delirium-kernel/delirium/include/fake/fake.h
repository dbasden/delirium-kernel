#ifndef __FAKE_H
#define __FAKE_H

#define __write_proto	size_t write(int, void *, size_t);
#define _fake_wr(ch) (write(1, (ch), 1))
#define _fake_writestr(xxx) {  int i;  \
		_fake_wr("{"); for (i=0; (xxx)[i] != '\0'; i++) \
		_fake_wr((xxx)+i); _fake_wr("}"); }

void cmain(unsigned long, void *);

#endif
