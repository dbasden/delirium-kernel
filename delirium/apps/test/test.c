/*
 * Test suite for Delirium
 */

#include <delirium.h>
#include "delibrium/delibrium.h"
#include <assert.h>
#include <stdarg.h>
#include <cpu.h>
#include "ipc.h"
#include "soapbox.h"
#include "rant.h"

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
	if (renounce(msg.destination) != msg.destination) printf("FAILED");
}
void test_consumer() {
	soapbox_id_t c_soapbox;
	soapbox_id_t ret;

	c_soapbox = get_new_soapbox("tests/producer-consumer");
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

	printf("Testing interrupt handler\n");
	asm volatile ( "INTO" );

	printf("Testing producer/consumer: ");
	test_producer();

	printf("Testing threads");
	new_thread(test_task_a);
	new_thread(test_task_b);
	new_thread(test_task_c);

	printf("Tests over\n");
}
