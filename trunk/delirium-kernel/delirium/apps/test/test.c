/*
 * Test suite for Delirium
 */

#include <delirium.h>
#include "delibrium/delibrium.h"
#include <assert.h>
#include <stdarg.h>
#include <cpu.h>
#include <ktimer.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>
#include "delibrium/timer.h"


void test_stdarg(int count, ...) {
	int i;
	va_list argp;
	int n;
	va_start(argp, count);

	for (i=0; i<count; i++) {
		n = va_arg(argp, int);
		Assert(i == n);
	}

	va_end(argp);
}

void test_task_a() { int i = 12; for (;i;i--) { print("A"); yield(); } }
void test_task_b() { int i = 23; for (;i;i--) print("B"); }
void test_task_c() { int i = 45; for (;i;i--) { print("C"); yield(); } }


void rx_consumer(message_t msg) {
	printf("[rx_consumer: got message from %u ", msg.sender);
	switch (msg.type) {
		case signal: printf("(signal %x)", msg.m.signal);
			break;
		case linear:
		case gestalt:
		default: printf("Got unknown type %x]\n", msg.type);
	}
	printf("(disassociating consumer from %x)", msg.destination);
	if (renounce(msg.destination) != msg.destination) printf("FAILED RENOUNCE!");
}
void test_consumer() {
	soapbox_id_t c_soapbox;
	soapbox_id_t ret;

	c_soapbox = get_new_soapbox("tests/producer-consumer");
	if (c_soapbox == 0) {
		printf("%s: FAIL! couldn't create soapbox tests/producer-consumer!\n", __func__);
		return;
	}
	printf("[consumer: got soapbox id %x]", c_soapbox);
	ret = supplicate(c_soapbox, rx_consumer);
	Assert(ret);
}

void test_producer() {
	soapbox_id_t p_soapbox = 0;
	message_t outmsg;

	outmsg.type = signal;
	outmsg.m.signal = 0xc0c1f00f;

	new_thread(test_consumer);
	while (!(p_soapbox = get_soapbox_from_name("tests/producer-consumer")))
		yield();
	printf("[producer: got soapbox id %x]", p_soapbox);
	printf("[producer: sending signal %x]", outmsg.m.signal);
	rant(p_soapbox, outmsg);
	printf("[producer: sending signal %x]", outmsg.m.signal);
	rant(p_soapbox, outmsg);
}

void test_timer_rx(message_t msg) {
	u_int64_t tsc = rdtsc();
	Assert(msg.type == signal);	
	printf("%s: timer tick every %u us. TSC is 0x%8x%8x. lower word: %u\n", __func__, (u_int32_t)msg.m.signal, (u_int32_t)(tsc >> 32),(u_int32_t)(tsc & 0xffffffff),(u_int32_t)(tsc & 0xffffffff));
}

void dream() {

	Assert(sizeof(u_int8_t) == 1);
	Assert(sizeof(u_int16_t) == 2);
	Assert(sizeof(u_int32_t) == 4);
	Assert(sizeof(u_int64_t) == 8);
	Assert(sizeof(int8_t) == 1);
	Assert(sizeof(int16_t) == 2);
	Assert(sizeof(int32_t) == 4);
	Assert(sizeof(int64_t) == 8);


	Assert(atoi("") == 0);
	Assert(atoi("42") == 42);
	Assert(atoi("2") == 2);
	Assert(atoi("-2") == -2);
	Assert(atoi("    	-2") == -2);
	Assert(atoi("-0") == 0);
	Assert(atoi("-1fishy") == -1);
	Assert(atoi("-1fishy543") == -1);
	Assert(atoi("fishy543") == 0);
	Assert(atoi("4234 543") == 4234);
	Assert(atoi("32453453") == 32453453);

	printf("Testing printf\n");
	printf("0123456789ABCDEF     ");
	printf("%%d, 42 = %d\t", 42);
	printf("%%u, 42 = %u      ", 42);
	printf("%%03u, 42 = %03u\n", 42);
	printf("%%x, 42 = %x       ", 42);
	printf("%%c, 42 = %c\t", 42);
	printf("%%b, 42 = %b\n", 42);

	printf("Testing stdarg\n");
	test_stdarg(5, 0, 1, 2, 3, 4, 5);

	printf("Testing cmpxchg");
	volatile int v;
	int a;
	v = 10;
	a = cmpxchg(v, 20, 10);
	printf("%d, %d\n", a,v);
	a = cmpxchg(v, 50, 10);
	printf("%d, %d\n", a,v);
	a = cmpxchg(v, 50, 20);
	printf("%d, %d\n", a,v);

	printf("Testing timers\n");
	soapbox_id_t timersb = get_new_anon_soapbox();
	supplicate(timersb, test_timer_rx);
	add_timer(timersb, 1000, 10, 1000); /* Every 1ms */
	add_timer(timersb, 1000000, 10, 1); /* Every 1s */
#if 0

	printf("Testing interrupt handler\n");
	asm volatile ( "INTO" );

	printf("Testing producer/consumer: ");
	test_producer();

	printf("Testing threads");
	new_thread(test_task_a);
	new_thread(test_task_b);
	new_thread(test_task_c);

	printf("Tests over\n");
#endif
}
