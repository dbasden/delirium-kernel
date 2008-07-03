#include <delirium.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>

#include "delibrium/delibrium.h"
#include "slip.h"

#define SLIPDEBUG
#define PAGE_SIZE	4096

/*
 * SLIP driver
 *
 * TODO: Move the serial thread stuff out of here
 */

soapbox_id_t	datalink_soapbox;
u_int16_t	serial_base;

inline void slip_send(char * packet, size_t len) {

	/* As per RFC 1055, send an end packet to flush any line noise */
	send_serial(serial_base, SLIP_END);

	for (;len;len--, packet++) {
		switch (*packet) {
			case SLIP_ESC:
				send_serial(serial_base, SLIP_ESC);
				send_serial(serial_base, SLIP_ESC_ESC);
				break;
			case SLIP_END:
				send_serial(serial_base, SLIP_ESC);
				send_serial(serial_base, SLIP_ESC_END);
				break;
			default:
				send_serial(serial_base, *packet);
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

void slip_serial_rx_thread() {
	// This be serial thread.
	soapbox_id_t serial_sb;
	char *frame;
	size_t framesize;
	int escaped = 0;
	int sendframe = 0;

	frame = getpage();
	framesize = 0;
	
	// TODO: stop hardcoding the serial port number
	if (!(serial_sb = get_new_soapbox("/hardware/serial/0"))) {
		print("slip_serial_rx_thread: /hardware/serial/0 already registered");
		return;
	}

	for (;;) {
		char inchar;
		message_t msg;

		inchar = read_serial(serial_base);

		#ifdef SLIP_DEBUG
		printf("[slip] slip_serial_rx_thread: read 0x%2x\n", inchar);
		#endif
		
		switch (inchar) {
			case SLIP_END:
				sendframe = 1;
				break;
			case SLIP_ESC:
				escaped = 1;
				break;
			case SLIP_ESC_ESC:
				if (escaped) inchar = SLIP_ESC;
			case SLIP_ESC_END:
				if (escaped) inchar = SLIP_END;
			default:
				escaped = false;
				frame[framesize++] = inchar;
		}

		if (framesize == PAGE_SIZE) 
			sendframe = true;

		if (sendframe) {
			msg.type = gestalt;
			msg.m.gestalt.length = framesize ;
			msg.m.gestalt.gestalt = frame;
			rant(serial_sb, msg);

			sendframe = false;
			framesize = 0;
			frame = getpage();
		}
	}
}

void slip_start_interface() {
	soapbox_id_t default_sb;
	soapbox_id_t serial_input_sb;

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

void slip_init(u_int16_t base_port, size_t speed) {
	serial_base = base_port;
	init_serial(base_port, speed);
	new_thread(slip_serial_rx_thread);
	new_thread(slip_start_interface);
}
