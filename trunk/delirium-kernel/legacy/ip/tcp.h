#ifndef __TCP_H
#define __TCP_H

#include <delirium.h>

#define __packme	__attribute__ ((packed))

/* Network byte order */
struct TCP_Header {
	u_int16_t	source_port		__packme;
	u_int16_t	destination_port	__packme;
	u_int32_t	sequence_num		__packme;
	u_int32_t	ack_num			__packme;
	u_int8_t	data_offset		__packme; /* Low nibble: reserved */
	u_int8_t	flags			__packme; /* High bits: reserved */
	u_int16_t	window			__packme;
	u_int16_t	checksum		__packme;
	u_int16_t	urgent_pointer		__packme;
	/* options can be added, but must be 0 padded to 32 bytes */
} __packme;

#define TCP_OPTION_END		0
#define TCP_OPTION_NOOP		1
#define TCP_OPTION_MSS		2

typedef enum {
	listen, syn-sent, syn_received, established, fin_wait_1, fin_wait_2,
	close_wait, closing, last_ack, time_wait, closed
} tcp_state_t;

typedef enum {
	open, send, receive, close, abort, status
} tcp_event_t;

#endif
