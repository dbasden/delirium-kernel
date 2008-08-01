#include <delirium.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>
#include <i386/interrupts.h>
#include <i386/io.h>
#include <assert.h>

#include "delibrium/delibrium.h"
#include "delibrium/serial.h"
#include "slip.h"

#define USE_TUN
#undef	USE_SLIP

#undef SLIPDEBUG

#define PAGE_SIZE	4096

/*
 * SLIP driver
 *
 * TODO: Move the serial thread stuff out of here
 */

soapbox_id_t	datalink_soapbox;
soapbox_id_t	ip_layer_soapbox;
u_int16_t	serial_base;

inline void slip_send(u_int8_t * packet, size_t len) {

	/* As per RFC 1055, send an end packet to flush any line noise */
	blocking_send_serial(serial_base, SLIP_END);

	for (;len;len--, packet++) {
		switch (*packet) {
			case SLIP_ESC:
				blocking_send_serial(serial_base, SLIP_ESC);
				blocking_send_serial(serial_base, SLIP_ESC_ESC);
				break;
			case SLIP_END:
				blocking_send_serial(serial_base, SLIP_ESC);
				blocking_send_serial(serial_base, SLIP_ESC_END);
				break;
			default:
				blocking_send_serial(serial_base, *packet);
		}
	}

	/* Send end of packet */
	blocking_send_serial(serial_base, SLIP_END);
}

void datalink_listener(message_t msg) {
	if (msg.type == gestalt) {
		if (msg.m.gestalt.length > 0)
			slip_send(msg.m.gestalt.gestalt, msg.m.gestalt.length);
		#ifdef SLIPDEBUG
		else
			print("[slip] datalink_listener got empty gestalt message\n");
		#endif
		/* We need to release the memory for the packet */
		assert (msg.m.gestalt.length <= PAGE_SIZE);
		freepage(msg.m.gestalt.gestalt);
	}
	#ifdef SLIPDEBUG
	else {
		print("[slip] datalink_listener got non gestalt message\n");
	}
	#endif
}

extern int _TUN_read_frame(void *buf, int len);
extern int _TUN_write_frame(void *buf, int len);
extern int _TUN_is_data_waiting();

void tun_todatalink_listener(message_t msg) {
	if (msg.type == gestalt ) {

		if (msg.m.gestalt.length > 0) {
			int ret;
			ret = _TUN_write_frame(msg.m.gestalt.gestalt, msg.m.gestalt.length);
			if (ret < msg.m.gestalt.length)
				printf("%s: warning! short write from _TUN_write_frame (only sent %u of %u bytes\n", __func__, ret, msg.m.gestalt.length);
		}

		assert (msg.m.gestalt.length <= PAGE_SIZE);
		assert (msg.m.gestalt.length > 0);
		freepage(msg.m.gestalt.gestalt);
	}
	else 
		printf("%s got non gestalt message\n", __func__);
}

void tun_interrupt_handler() {
	size_t ret;
	message_t msg;

	if (! _TUN_is_data_waiting())
		return;

	msg.type = gestalt;
	msg.m.gestalt.gestalt = getpage();

	ret = _TUN_read_frame(msg.m.gestalt.gestalt, PAGE_SIZE);
	if (ret <= 0) {
		printf("%s: error on _TUN_read_frame (return val %d)\n", __func__, ret);
		freepage(msg.m.gestalt.gestalt); return;
	}
		
#if 0
	printf("%s: read %u bytes from wire\n", __func__, ret);
#endif

	msg.m.gestalt.length = ret;
	rant(ip_layer_soapbox, msg);
}

void serial_listener(message_t msg) {
	if (msg.type == gestalt) {
		#ifdef SLIPDEBUG
		char *buf = msg.m.gestalt.gestalt;
		int i;
		print("[slip] Gotframe from serial port\n");
		for (i=0; i<msg.m.gestalt.length; ++i)
			printf("%2x ", buf[i]);
		print("\n");
		#endif

		#ifdef HAIRPIN_SLIP_TEST
		rant(datalink_soapbox, msg);
		return;
		#endif

		rant(ip_layer_soapbox, msg);
	} 
}

char volatile *slip_inbound_frame;
volatile size_t slip_inbound_framesize;
volatile int slip_inbound_escaped;
soapbox_id_t serial_inbound_sb;

void slip_on_receive_interrupt() {
	u_int8_t inchar;
	int sendframe = 0;
	message_t msg;

	if (! _SERIAL_is_Data_Waiting(serial_base)) return;
	inchar = _SERIAL_Readb(serial_base);

	#if 0
	// DONT USE PRINTF IN AN EVENT HANDLER!
	// It can't release a semaphore in vgaprint
	printf(">%2x>", inchar);
	#endif
	switch (inchar) {
		case SLIP_END:
			sendframe = 1;
			break;
		case SLIP_ESC:
			slip_inbound_escaped = 1;
			break;
		case SLIP_ESC_ESC:
			if (slip_inbound_escaped) {
				inchar = SLIP_ESC;
				slip_inbound_escaped = 0;
			}
		case SLIP_ESC_END:
			if (slip_inbound_escaped) {
				inchar = SLIP_END;
				slip_inbound_escaped = 0;
			}
		default:
			slip_inbound_escaped = 0;
			slip_inbound_frame[slip_inbound_framesize++] = inchar;
	}

	if (slip_inbound_framesize == PAGE_SIZE) 
		sendframe = 1;

	if (sendframe && slip_inbound_framesize != 0) {
		msg.type = gestalt;
		msg.m.gestalt.length = slip_inbound_framesize;
		msg.m.gestalt.gestalt = (void *)slip_inbound_frame;
		rant(serial_inbound_sb, msg);

		slip_inbound_framesize = 0;
		slip_inbound_frame = getpage();
	}
}

void slip_setup_inth(u_int8_t hw_int) {
	// This be serial thread.
	slip_inbound_escaped = 0;
	slip_inbound_framesize = 0;
	slip_inbound_frame = getpage();

	// TODO: stop hardcoding the serial port number
	if (!(serial_inbound_sb = get_new_soapbox("/hardware/serial/0"))) {
		print("slip_serial_rx_thread: /hardware/serial/0 already registered");
		return;
	}
	add_c_interrupt_handler(hw_int, slip_on_receive_interrupt);
}


void slip_start_interface() {
	soapbox_id_t default_sb;
	soapbox_id_t serial_input_sb;

	slip_setup_inth(4);

	if (! (serial_input_sb = get_soapbox_from_name("/hardware/serial/0"))) {
		print("slip_start_interface: couldn't find soapbox for /hardware/serial/0");
		return;
	}
	

	if (! (ip_layer_soapbox = get_soapbox_from_name("/network/ip/inbound"))) {
		print("slip_start_interface: couldn't find soapbox for /network/ip/inbound");
		return;
	}

	if (!(datalink_soapbox = get_new_soapbox("/network/ip/link/slip"))) {
		print("slip_start_interface: /network/interface/slip/ip already registered");
		return;
	}
	

	if (supplicate(datalink_soapbox, datalink_listener) == 0) {
		print("slip_start_interface: error registering ipv4_listener");
	}
	if (supplicate(serial_input_sb, serial_listener) == 0) {
		print("slip_start_interface: error registering serial_listener");
	}
	
	// Attempt to register as the default_sb IP provider, but only if there
	// isn't one registered yet; This way if we have a more sophisticated
	// network stack, we don't interfere with it, but lightweight
	// implementations still get the benefit of a default_sb route without
	// the router
	default_sb = get_new_soapbox("/network/ip/link/default");
	if (default_sb)
		supplicate(default_sb, datalink_listener);
}

void slip_init(u_int16_t base_port, u_int8_t hw_int,size_t speed) {
	serial_base = base_port;
	init_serial(base_port, speed);
	new_thread(slip_start_interface);

}

#define TUN_VIRTUAL_HWINT	9

void tun_init() {
	soapbox_id_t tun_sb;
	soapbox_id_t default_sb;

	if (! (ip_layer_soapbox = get_soapbox_from_name("/network/ip/inbound"))) {
		print("couldn't find soapbox for /network/ip/inbound. This is bad!. Aborting.\n");
		return;
	}

	tun_sb = get_new_soapbox("/network/ip/link/tun");
	if (tun_sb == 0) {
		printf("Unable to register /network/ip/link/tun! Bailing\n");
		return;
	}

	default_sb = get_new_soapbox("/network/ip/link/default");
	if (default_sb == 0)  {
		printf("Unable to get default ip link soapbox! Continuing..n");
	}

	if (supplicate(tun_sb, tun_todatalink_listener) == 1)
		printf("%s: error registering /network/ip/link/tun to datalink listener", __func__);

	if (default_sb && supplicate(default_sb, tun_todatalink_listener) == 0)
		printf("%s: error registering /network/ip/link/default to datalink listener", __func__);
		
	add_c_interrupt_handler(TUN_VIRTUAL_HWINT, tun_interrupt_handler);

	/* Hook the tun handler */
	extern void init_tun();
	init_tun();
}

#include "ipv4.h"

void dream() {
	print("Starting IPv4 driver with address 192.168.11.2\n");
	ipv4_init( IPV4_OCTET_TO_ADDR(192,168,11,2) );
#ifdef USE_TUN
	print("Starting TUN driver\n");
	tun_init();
#endif
#ifdef USE_SLIP
	print("Starting SLIP driver on port 0x3f8, irq 4 at 115200\n");
	slip_init(0x3f8, 4, 115200);
#endif
}
