#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PAGE_SIZE	4096

void new_thread(void *entry(void)) {
	fprintf(stderr,"%s: called with entry 0x%8x. Calling...\n", __func__, (uint32_t) entry);
	entry();
	fprintf(stderr, "%s: finished fake new_thread with entry 0x%8x\n", __func__, (uint32_t) entry);
}

void *getpage() {
	return malloc(PAGE_SIZE);
}
void *kgetpage() { return getpage(); }

void freepage(void *ptr) {
	free(ptr);
}

void kprint(char * cs) {
	printf(cs);
}

void yield() {
	fprintf(stderr, "%s stub called\n", __func__);
}

/* pool_alloc and pool_free might be defined in pool.c, but kpool_alloc and kpool_free
 * are assumed to working in kernel space 
 */
void * kpool_alloc(int exp) { return malloc(1 << exp); }
void kpool_free(int exp, void * ptr) { free(ptr); }
void * pool_alloc(int exp) { return malloc(1 << exp); }
void pool_free(int exp, void * ptr) { free(ptr); }

void setup_pools() {
	fprintf(stderr, "%s stub called\n", __func__);
}

typedef void(*serial_callback_t)(void);
serial_callback_t serial_callback;

typedef void(*tun_callback_t)(void);
tun_callback_t tun_callback;

void add_c_interrupt_handler(u_int32_t hwirq, void *handler(void)) {
	fprintf(stderr, "%s: called: 0x%8x to IRQ %d\n", __func__, (uint32_t) handler, hwirq);
	if (hwirq == 4) {
		fprintf(stderr, "%s: hooking virtual IRQ 4 to serial pty\n", __func__);
		serial_callback = (serial_callback_t)handler;
	}
	else if (hwirq == 9) {
		fprintf(stderr, "%s: hooking virtual IRQ 9 to tun interface\n", __func__);
		tun_callback = (tun_callback_t)handler;
	}
}

inline void lucid_spin_wait_semaphore(uint32_t *s) {
	if (*s == 0)  {
		fprintf(stderr, "%s: Error! Semaphore already taken!\n", __func__);
		exit(EXIT_FAILURE);
	}
	++(*s);
}

/* Serial emulation using a PTY */

#define SERIAL_EMU_PTY	"/tmp/socat1"


/* FILE *serial_fp; */

int serial_fd;

void init_serial() {
	serial_fd = open(SERIAL_EMU_PTY, O_RDWR);
	if (serial_fd == -1) {
		perror(SERIAL_EMU_PTY);
		exit(EXIT_FAILURE);
	}
}

void _SERIAL_Sendb(int baseport, int byte) {
	if ( write(serial_fd, &byte, 1) != 1) {
		perror("write(serial_fd)");
		exit(EXIT_FAILURE);
	}
}

int _SERIAL_is_Data_Waiting(int baseport) {
	fd_set fds;
	struct timeval tval;
	int ret;

	if (serial_callback == NULL)
		return 0;

	tval.tv_sec = 0;
	tval.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(serial_fd, &fds);

	ret = select(serial_fd + 1, NULL, &fds, NULL, &tval);

	if (ret == 1) return 1;
	return 0;
}

int _SERIAL_is_Spare_Outbuf(int baseport) {
	return 1;
}

int _SERIAL_Readb(int baseport) {
	char data;
	if (read(serial_fd, &data, 1) != 1) {
		perror("read(serial_fd)");
		exit(EXIT_FAILURE);
	}
	return data;
}

inline void blocking_send_serial(u_int16_t baseport, char ch) {
	_SERIAL_Sendb(baseport, ch);
}

#define SERIAL_SELECT_BLOCK_TIME 10000

/* return 1 iff the timeout didn't kick in */
int do_serial_select() {
	fd_set fds;
	struct timeval tval;
	int ret;

	if (serial_callback == NULL)
		return -1;

	tval.tv_sec = 0;
	tval.tv_usec = SERIAL_SELECT_BLOCK_TIME;
	FD_ZERO(&fds);
	FD_SET(serial_fd, &fds);

	ret = select(serial_fd + 1, &fds, NULL, NULL, &tval);
	if (ret == -1) { perror("select()"); return 0; }
	if (ret == 0) { /* timeout */ return 0; }
	serial_callback();

	return 1;
}

#define TUNTAP
#ifdef TUNTAP

#define TUN_DEVICE	"/tmp/socat.tun"

int tun_fd = -1;
char zeros[1500];

void init_tun() {
	tun_fd = open(TUN_DEVICE, O_RDWR | O_SYNC);
	if (tun_fd == -1) {
		perror(TUN_DEVICE);
		exit(EXIT_FAILURE);
	}
}

int _TUN_read_frame(void *buf, int buflen) {
	if (tun_fd == -1){ perror(__func__); exit(EXIT_FAILURE); }
	return read(tun_fd, buf, buflen);
}
int _TUN_write_frame(void *buf, unsigned long buflen) {
	if (tun_fd == -1){ perror(__func__); exit(EXIT_FAILURE); }
	int ret;
	ret = write(tun_fd, buf, buflen);
	if (ret) fsync(tun_fd);
	return ret;
}

int _TUN_is_data_waiting() {
	fd_set fds;
	struct timeval tval;

	if (tun_callback == NULL) return 0;
	tval.tv_sec = 0; tval.tv_usec = 0;
	FD_ZERO(&fds); FD_SET(tun_fd, &fds);
	return select(tun_fd + 1, NULL, &fds, NULL, &tval) == 1;
}

/* return 1 iff the timeout didn't kick in */
int do_tun_select() {
	fd_set fds;
	struct timeval tval;
	int ret;

	if (tun_callback == NULL) return -1;

	tval.tv_sec = 0;
	tval.tv_usec = SERIAL_SELECT_BLOCK_TIME;
	FD_ZERO(&fds);
	FD_SET(tun_fd, &fds);

	ret = select(tun_fd + 1, &fds, NULL, NULL, &tval);
	if (ret == -1) { perror("select()"); return 0; }
	if (ret == 0) { /* timeout */ return 0; }
	tun_callback();

	return 1;
}
/*endif TUNTAP*/
#endif
