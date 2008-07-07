#include <delirium.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>
#include <i386/interrupts.h>
#include <i386/io.h>

#include "delibrium/delibrium.h"
#include "ipv4.h"
#include "ipv4_icmp.h"

/*
 * IPv4 driver
 */

static volatile soapbox_id_t	datalink_soapbox;

IPv4_Address	ipv4_local_ip;

#define IPv4_DEBUG

/* Discard a packet without notification */
static inline void discard_silently(message_t msg) {
	#ifdef IPv4_DEBUG
	print("[ip] ip_v4_discard_silently: Dropping packet\n");
	#endif

	if (msg.type == gestalt) freepage(msg.m.gestalt.gestalt);
}

void ipv4_datalink_listener(message_t msg) {
	#ifdef IPv4_DEBUG
	print("[ip] ip_inbound_listener: got packet from datalink.\n");
	#endif
	
	discard_silently(msg); /* STUB */

}

void ipv4_userspace_listener(message_t msg) {
	#ifdef IPv4_DEBUG
	print("[ip] ip_inbound_listener: got datagram from userspace.\n");
	#endif

	discard_silently(msg); /* STUB */
}

static inline void send_to_datalink(message_t msg) {
	if (datalink_soapbox == 0) 
		datalink_soapbox = get_soapbox_from_name("/network/ip/link/default");
	if (datalink_soapbox == 0)  {
		print("[ip] send_to_datalink: ERROR: couldn't get default datalink to send IP packet\n");
		discard_silently(msg);
	}
	rant(datalink_soapbox, msg);
}


/* start IPv4 part of the stack */
void ipv4_start() {
	soapbox_id_t	from_userspace_sb;
	soapbox_id_t	from_datalink_sb;
	
	/*
	 * Attempt to get a default link. 
	 * If it doesn't exist, We'll try again later
	 */
	datalink_soapbox = 0;
	datalink_soapbox = get_soapbox_from_name("/network/ip/link/default");

	/* Try to register as the default IPv4 handler */
	if (! (from_userspace_sb = get_new_soapbox("/network/ip/outbound"))) {
		print("ipv4_start: /network/ip/outbound already registered");
		return;
	}
	if (! (from_datalink_sb = get_new_soapbox("/network/ip/inbound"))) {
		print("ipv4_start: /network/ip/inbound already registered");
		return;
	}

	supplicate(from_userspace_sb, ipv4_userspace_listener);
	supplicate(from_datalink_sb, ipv4_datalink_listener);
} 


void ipv4_init(IPv4_Address ip) {
	ipv4_local_ip = ip;
	new_thread(ipv4_start);
}
