#include <delirium.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>
#include <i386/interrupts.h>

#include "delibrium/delibrium.h"
#include "delibrium/serial.h"
#include "slip.h"

#define SLIPDEBUG	1
#define PAGE_SIZE	4096

/*
 * SLIP driver
 *
 * TODO: Move the serial thread stuff out of here
 */

soapbox_id_t	datalink_soapbox;
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
}

void datalink_listener(message_t msg) {
	if (msg.type == gestalt) {
		if (msg.m.gestalt.length > 0)
			slip_send(msg.m.gestalt.gestalt, msg.m.gestalt.length);
		#ifdef SLIPDEBUG
		else
			print("[slip] datalink_listener got empty gestalt message\n");
		#endif
	}
	#ifdef SLIPDEBUG
	else {
		print("[slip] datalink_listener got non gestalt message\n");
	}
	#endif
}

void serial_listener(message_t msg) {
	if (msg.type == gestalt) {
		char *buf = msg.m.gestalt.gestalt;
		#ifdef SLIPDEBUG
		int i;
		print("[slip] Gotframe from serial port\n");
		for (i=0; i<msg.m.gestalt.length; ++i)
			printf("%2x ", buf[i]);
		print("\n");
		#endif
		freepage(msg.m.gestalt.gestalt);
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
			if (slip_inbound_escaped) inchar = SLIP_ESC;
		case SLIP_ESC_END:
			if (slip_inbound_escaped) inchar = SLIP_END;
		default:
			slip_inbound_escaped = 0;
			slip_inbound_frame[slip_inbound_framesize++] = inchar;
	}

	if (slip_inbound_framesize == PAGE_SIZE) 
		sendframe = 1;

	if (sendframe) {
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

void slip_init(u_int16_t base_port, u_int8_t hw_int, size_t speed) {
	serial_base = base_port;
	init_serial(base_port, speed);
	new_thread(slip_start_interface);

}

void dream() {
	print("Starting SLIP driver on port 0x3f8, irq 4 at 9600\n");
	slip_init(0x3f8, 4, 9600);
}
