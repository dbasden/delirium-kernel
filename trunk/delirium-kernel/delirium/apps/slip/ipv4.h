#ifndef __IPV4_H
#define __IPV4_H

/* ipv4 headers for DeLiRiuM
 *
 * Some assumptions are made that the arch is using little-endian. Not many.
 */
#include <delirium.h>

#define IPV4_PROTO_ICMP		1
#define IPV4_PROTO_TCP		6
#define IPV4_PROTO_UDP		17

#define __packme	__attribute__ ((packed))

typedef size_t IPv4_Address;

/* Network byte order */
struct IPv4_Header {
	u_int8_t	version_headerlen; /* Version in high nibble */
	u_int8_t	type_of_service;
	u_int16_t	total_length;
	u_int16_t	identification;
	u_int16_t	flags_fragoff; /* flags are top 3 bits (network byte order) */
	u_int8_t	ttl;		
	u_int8_t	protocol;
	u_int16_t	header_checksum;
	IPv4_Address	source;
	IPv4_Address	destination;
	u_int8_t	option_type;
	u_int8_t	option_len; /* Optional */
	u_int8_t	option_data; /* Optional */
	u_int8_t	padding;
} __packme;

#define IPV4_MIN_HEADER_SIZE	20

#define IPV4_OCTET_TO_ADDR(_a,_b,_c,_d) (((_d) <<24 ) | ((_c)<<16) |((_b)<<8) | (_a))
#define htons(_i)	( ((_i) & 0xff00) >>8 | ((_i) & 0xff) << 8)
#define htonl(_i)	( ( ((_i) & 0xff000000) >> 24) | ( ((_i) & 0x00ff0000) >> 8) | \
			  ( ((_i) & 0x00000f00) << 8)  | ( ((_i) & 0x000000ff) << 24) )
#define ntohs(_i)	htons(_i)
#define ntohl(_i)	htonl(_i)


#define IPV4_PRINTADDR(_a)	printf("%d.%d.%d.%d",(_a) & 0xff,(_a)>>8 & 0xff, \
				(_a)>>16 & 0xff, (_a)>>24 & 0xff)

#define IPV4_EXTRACT_HEADERLEN(_ipheader_p) ((((struct IPv4_Header *)(_ipheader_p))->version_headerlen & 0xf) * 4)
#define IPV4_REMOVE_WORD_FROM_CHECKSUM(_checksum, _word) htons(~ ( (~ntohs(_checksum)) - ((_word) & 0xffff) ))

/* ip_header.c */
inline u_int16_t ipv4_checksum(void *buf, size_t len);

inline void ipv4_genHeader(struct IPv4_Header *h, u_int16_t packetLength,
				u_int8_t ttl, u_int8_t protocol, u_int16_t identity,
				IPv4_Address source, IPv4_Address destination,
				u_int8_t flags);

void ipv4_header_dump(struct IPv4_Header *h);

/* ipv4.c */
void ipv4_init(IPv4_Address ip);

#endif

