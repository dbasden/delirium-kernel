#ifndef __IPV4_H
#define __IPV4_H

#include <delirium.h>
/* Network byte order */
#define __packme	__attribute__ ((packed))

struct ipv4_header {
	u_int8_t	version_headerlen __packme; /* Version in low nibble */
	u_int8_t	type_of_service	__packme;
	u_int16_t	total_length	__packme;
	u_int16_t	identification	__packme;
	u_int16_t	flags_fragoff	__packme; /* Low nibble: flags. */
	u_int8_t	ttl		__packme;		
	u_int8_t	protocol	__packme;
	u_int16_t	header_checksum	__packme;
	u_int32_t	source		__packme;
	u_int32_t	destination	__packme;
	u_int8_t	option_type	__packme;
	u_int8_t	option_len	__packme; /* Optional */
	u_int8_t	option_data	__packme; /* Optional */
	u_int8_t	padding		__packme;
} __packme;

#endif
