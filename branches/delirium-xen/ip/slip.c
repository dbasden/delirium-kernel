#include <delirium.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>

#include "delibrium/delibrium.h"
#include "slip.h"

soapbox_id_t	ipv4_soapbox = 0;
u_int16_t	serial_base;

void ipv4_listener(message_t msg) {
}

void serial_listener(message_t msg) {
}

void slip_start_interface() {
	soapbox_id_t default_sb;

	
	if (!(ipv4_soapbox = get_new_soapbox("/network/interface/slip/ip"))) {
		print("slip_start_interface: interface already registered");
		return;
	}
	

	if (supplicate(ipv4_soapbox, ipv4_listener)) {
		print("slip_start_interface: error registering interface");
	}
	
	// Attempt to register as the default_sb IP provider, but only if there
	// isn't one registered yet; This way if we have a more sophisticated
	// network stack, we don't interfere with it, but lightweight
	// implementations still get the benefit of a default_sb route without
	// the router
	default_sb = ipv4_soapbox = get_new_soapbox("/network/ip/route/default_sb");
	if (default_sb)
		supplicate(default_sb, ipv4_listener);
}

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

void slip_init(u_int16_t base_port, size_t speed) {
	serial_base = base_port;
	init_serial(base_port, speed);
	new_thread(slip_start_interface);
}
