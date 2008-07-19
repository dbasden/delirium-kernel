#ifndef __IPV4_H
#define __IPV4_H

#include <delirium.h>

#define IPV4_PROTO_ICMP		1
#define IPV4_PROTO_TCP		6
#define IPV4_PROTO_UDP		17

/* Network byte order */
#define __packme	__attribute__ ((packed))

typedef size_t IPv4_Address;

struct IPv4_Header {
	u_int8_t	version_headerlen __packme; /* Version in low nibble */
	u_int8_t	type_of_service	__packme;
	u_int16_t	total_length	__packme;
	u_int16_t	identification	__packme;
	u_int16_t	flags_fragoff	__packme; /* Low nibble: flags. */
	u_int8_t	ttl		__packme;		
	u_int8_t	protocol	__packme;
	u_int16_t	header_checksum	__packme;
	IPv4_Address	source		__packme;
	IPv4_Address	destination	__packme;
	u_int8_t	option_type	__packme;
	u_int8_t	option_len	__packme; /* Optional */
	u_int8_t	option_data	__packme; /* Optional */
	u_int8_t	padding		__packme;
} __packme;

#ifdef __FUTURE__
#define IPV4_INTERFACE_UP		1
#define IPV4_INTERFACE_POINTTOPOINT	2

struct IPv4_Interface {
	IPv4_Address	ip;
	IPv4_Address	netmask;
	IPv4_Address	gateway;
	size_t		state;
};
#endif /* __FUTURE__ */

#define IPV4_OCTET_TO_ADDR(_a,_b,_c,_d) (((_d) <<24 ) | ((_c)<<16) |((_b)<<8) | (_a))
#define htons(_i)	( ((_i) & 0xff00) >>8 | ((_i) & 0xff) << 8)
#define htonl(_i)	( ( ((_i) & 0xff000000) >> 24) | ( ((_i) & 0x00ff0000) >> 8) | \
			  ( ((_i) & 0x00000f00) << 8)  | ( ((_i) & 0x000000ff) << 24) )
#define ntohs(_i)	htons(_i)
#define ntohl(_i)	htonl(_i)

#endif

