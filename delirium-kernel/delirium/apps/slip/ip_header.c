/*
 * Very minimalist implementation of IPv4
 *
 * Copyright (c)2005-2008 David Basden <davidb-delirium@rcpt.to>
 * 
 * References:
 *
 *   RFC 792:  Internet Protocol: DARPA Internet Program Protocol Specification
 *   RFC 1071: Computing the Internet checksum
 *
 * Although these RFCs were consulted, this should not imply complete or 
 * correct implementation of those RFCs. Any use of this code is at your
 * own risk. All warranties, implicit or otherwise are hereby disclaimed.
 * This source code makes NO CLAIMS TO BE FIT FOR ANY PURPOSE WHATSOEVER.
 */

#include <delirium.h>
#include "delibrium/delibrium.h"

#if 0
typedef 	unsigned char	u_int8_t;
typedef 	unsigned short	u_int16_t;
typedef 	unsigned int	u_int32_t;
#endif

#include "ipv4.h"
#include "ipv4_icmp.h"


inline u_int16_t calc_ipv4_checksum(u_int16_t *h, int shortcount) {
	u_int32_t csum = 0;

	while (shortcount > 0)
		csum += h[--shortcount] & 0xffff;
	
	return htons(~csum);
}

inline u_int16_t ipv4_checksum(void *buf, size_t len) {
	u_int32_t csum = 0;
	u_int16_t * ptr = buf;

	while (len > 1) {
		csum += *ptr;
		len -= 2;
		++ptr;
	}
	if (len) 
		csum += *((u_int8_t *)ptr);

	while (csum >> 16)
		csum = (csum & 0xffff) + (csum >> 16);
	
	return (~csum) & 0xffff;
}

inline void ipv4_genHeader(struct IPv4_Header *h, u_int16_t packetLength,
				u_int8_t ttl, u_int8_t protocol, u_int16_t identity,
				IPv4_Address source, IPv4_Address destination,
				u_int8_t flags) {
	h->version_headerlen = 5 | (4<<4);
	h->type_of_service = 0;
	h->total_length = htons(20+packetLength);
	h->identification = htons(identity);
	h->flags_fragoff = flags & 0xff;
	h->ttl = ttl;
	h->protocol = protocol;
	h->header_checksum = 0;
	h->source = source;
	h->destination = destination;
	h->header_checksum = calc_ipv4_checksum((u_int16_t *)h, 10);
	h->header_checksum = htons(h->header_checksum);
}

#define IPV4_PRINTADDR(_a)	printf("%d.%d.%d.%d",(_a) & 0xff,(_a)>>8 & 0xff, \
				(_a)>>16 & 0xff, (_a)>>24 & 0xff)

#ifdef __FUTURE__
void ifdump(struct IPv4_Interface *interface) {
	printf("IP Address:\t"); IPV4_PRINTADDR(interface->ip);
	printf("\nIP Netmask:\t"); IPV4_PRINTADDR(interface->netmask);
	printf("\nIP Gateway:\t"); IPV4_PRINTADDR(interface->gateway);
	printf("\n");
}
#endif /* __FUTURE__ */

void ipv4_header_dump(struct IPv4_Header *h) {
	IPV4_PRINTADDR(h->source);	
	printf(" -> ");
	IPV4_PRINTADDR(h->destination);
	printf(", ttl %d, proto %d, id %d, csum 0x%x, flag 0x%x\n", h->ttl, h->protocol, 
		ntohs(h->identification), ntohs(h->header_checksum), 
		h->flags_fragoff & 0xff);
}

#if 0
int main() {
	struct IPv4_Interface testInt;
	struct IPv4_Header testHead;

	testInt.ip = IPV4_OCTET_TO_ADDR(10,0,0,1);
	testInt.netmask = IPV4_OCTET_TO_ADDR(255,255,255,255);
	testInt.gateway = IPV4_OCTET_TO_ADDR(10,0,0,2);
	testInt.state = IPV4_INTERFACE_UP | IPV4_INTERFACE_POINTTOPOINT;

	ifdump(&testInt);

	genHeader(&testHead, 0, 128, IPV4_PROTO_ICMP, 12345,
			IPV4_OCTET_TO_ADDR(10,0,0,1), 
			IPV4_OCTET_TO_ADDR(10,0,0,2),
			0);
	ipv4_header_dump(&testHead);

	return 0;
}
#endif
