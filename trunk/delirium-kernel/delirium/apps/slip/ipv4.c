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
#ifdef IPv4_DEBUG
#define IPv4_DEBUG_print(_m)	print(_m)
#else
#define IPv4_DEBUG_print(_m)
#endif

static inline void free_packet_memory(message_t msg) {
	if (msg.type == gestalt) freepage(msg.m.gestalt.gestalt);
}

/* Discard a packet without notification */
static inline void discard_silently(message_t msg) {
	IPv4_DEBUG_print("[ip] ip_v4_discard_silently: Dropping packet\n");
	free_packet_memory(msg);
}


static inline void send_to_datalink(message_t msg) {
	#ifdef IPv4_DEBUG
	print("< "); ipv4_header_dump((struct IPv4_Header *)msg.m.gestalt.gestalt);
	#endif
	if (datalink_soapbox == 0) 
		datalink_soapbox = get_soapbox_from_name("/network/ip/link/default");
	if (datalink_soapbox == 0)  {
		print("[ip] send_to_datalink: ERROR: couldn't get default datalink to send IP packet\n");
		discard_silently(msg);
	}
	rant(datalink_soapbox, msg);
}


void ipv4_checksum_and_send(message_t msg) {
	struct IPv4_Header * iphead = msg.m.gestalt.gestalt;
	size_t headerlen = IPV4_EXTRACT_HEADERLEN(iphead);
	if (headerlen > msg.m.gestalt.length) {
		IPv4_DEBUG_print("[ip] ipv4_checksum_and_send: headerlen bigger than packet. discarding\n");
		discard_silently(msg);
		return;
	}
	iphead->header_checksum = 0;
	iphead->header_checksum = ipv4_checksum(iphead, headerlen);
	send_to_datalink(msg);
}



void ipv4_handle_inbound_icmp(message_t msg) {
	#if 0
	IPv4_DEBUG_print("[ip] ipv4_handle_inbound_icmp: received ICMP packet\n");
	#endif
	
	struct IPv4_Header * iphead = msg.m.gestalt.gestalt;
	size_t ip_header_size;
	ip_header_size = IPV4_EXTRACT_HEADERLEN(iphead);
	struct IPv4_ICMP_Header *icmphead = (struct IPv4_ICMP_Header *)((u_int8_t *)iphead + ip_header_size);

	u_int16_t checksum = ipv4_checksum(icmphead, (msg.m.gestalt.length-ip_header_size));
	if (checksum != 0) {
		IPv4_DEBUG_print("[ip] ipv4_handle_inbound_icmp: Invalid ICMP checksum. discarding\n");
		#ifdef IPv4_DEBUG
		discard_silently(msg); return;
		#endif
	}

	IPv4_Address tmp = iphead->destination;
	switch (icmphead->type) {
		#if 0
			IPv4_DEBUG_print("[ip] ipv4_handle_inbound_icmp: Echo request. Responding...\n");
		#endif
		case (IPV4_ICMP_ECHO):
			iphead->destination = iphead->source;
			iphead->source = tmp;
			icmphead->type = IPV4_ICMP_ECHO_REPLY;
			icmphead->checksum = 0;
			icmphead->checksum = ipv4_checksum(icmphead, (msg.m.gestalt.length-ip_header_size));
			ipv4_checksum_and_send(msg);
			return;
		default:
			IPv4_DEBUG_print("[ip] ipv4_handle_inbound_icmp: unknown ICMP message type. discarding\n");

			discard_silently(msg);
			return;
	}
	discard_silently(msg); // stub
}
void ipv4_handle_inbound_tcp(message_t msg) {
	IPv4_DEBUG_print("[ip] ipv4_handle_inbound_icmp: received TCP packet\n");
	discard_silently(msg); // stub
}
void ipv4_handle_inbound_udp(message_t msg) {
	IPv4_DEBUG_print("[ip] ipv4_handle_inbound_icmp: received UDP packet\n");
	discard_silently(msg); // stub
}

void ipv4_datalink_listener(message_t msg) {
	if (msg.type != gestalt) return;

#if 0
	IPv4_DEBUG_print("[ip] ip_inbound_listener: got packet from datalink.\n"); 
#endif

	if (msg.m.gestalt.length < IPV4_MIN_HEADER_SIZE) {
		IPv4_DEBUG_print("[ip] inbound_listener: truncated header. discarding\n");
		discard_silently(msg);
		return;
	}

	struct IPv4_Header *iphead = msg.m.gestalt.gestalt;

	#ifdef IPv4_DEBUG
	print("> "); ipv4_header_dump(iphead);
	#endif

	if (ipv4_checksum(iphead, IPV4_EXTRACT_HEADERLEN(iphead)) != 0) {
		IPv4_DEBUG_print("[ip] inbound_listener: invalid IP header checksum. discarding\n");
	#ifdef IPv4_DEBUG
		iphead->header_checksum = 0;
		printf("Expected header checksum 0x%4x.\n", ipv4_checksum(iphead, IPV4_EXTRACT_HEADERLEN(iphead)));
	#endif
		discard_silently(msg); return;
	}
	if (iphead->destination != ipv4_local_ip) {
		IPv4_DEBUG_print("[ip] inbound_listener: packet not destined for us. discarding\n");
		discard_silently(msg); return;
	}
	if (ntohs(iphead->total_length) != msg.m.gestalt.length) {
		IPv4_DEBUG_print("[ip] inbound_listener: truncated or too large IP packet (we know nothing of fragments. discarding\n");
		discard_silently(msg); return;
	}
	if (IPV4_EXTRACT_HEADERLEN(iphead) > msg.m.gestalt.length) {
		IPv4_DEBUG_print("[ip] inbound_listener: IP headerlen larger than packet. discarding\n");
		discard_silently(msg); return;
	}
	if ((iphead->version_headerlen >> 4) != 4) {
		IPv4_DEBUG_print("[ip] inbound_listener: Not IP version 4 packet. discarding\n");
		discard_silently(msg); return;
	}


	switch (iphead->protocol) {
		case (IPV4_PROTO_ICMP):
			ipv4_handle_inbound_icmp(msg);
			return;
		case (IPV4_PROTO_TCP):
			ipv4_handle_inbound_tcp(msg);
			return;
		case (IPV4_PROTO_UDP):
			ipv4_handle_inbound_udp(msg);
			return;
		default:
			IPv4_DEBUG_print("[ip] inbound_listener: unknown protocol. discarding\n");
			discard_silently(msg);
			return;
	}
}


void ipv4_userspace_listener(message_t msg) {
	#ifdef IPv4_DEBUG
	print("[ip] ip_inbound_listener: got datagram from userspace.\n");
	#endif

	discard_silently(msg); /* STUB */
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
