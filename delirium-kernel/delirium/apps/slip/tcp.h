#ifndef __TCP_H
#define __TCP_H

#include <delirium.h>
#include <soapbox.h>
#include <ipc.h>
#include "ipv4.h"
#include "delibrium/dlib/queue.h"

#define __packme	__attribute__ ((packed))

/* Network byte order */
struct TCP_Header {
	u_int16_t	source_port;
	u_int16_t	destination_port;
	u_int32_t	sequence_num;
	u_int32_t	ack_num;
	u_int8_t	header_len; /* Low nibble: reserved */
	u_int8_t	flags; /* High bits: reserved */
	u_int16_t	window;
	u_int16_t	checksum;
	u_int16_t	urgent_pointer;
	/* options can be added, but must be 0 padded to 32 bytes */
} __packme;

#define TCP_MIN_HEADER_SIZE 20
#define TCP_EXTRACT_HEADERLEN(_tcpheader_p) (( ((struct TCP_Header *)(_tcpheader_p))->header_len & 0xf0 ) >> 2)

#define TCP_DEFAULT_MSS		536
#define TCP_PREFERRED_MSS	1400

#define TCP_OPTION_END		0
#define TCP_OPTION_NOOP		1
#define TCP_OPTION_MSS		2

#define TCP_FLAG_NONE	0
#define TCP_FLAG_FIN	0x01
#define TCP_FLAG_SYN	0x02
#define TCP_FLAG_RST	0x04
#define TCP_FLAG_PSH	0x08
#define TCP_FLAG_ACK	0x10
#define TCP_FLAG_URG	0x20

typedef enum {
	listen, syn_sent, syn_received, established, fin_wait_1, fin_wait_2,
	close_wait, closing, last_ack, time_wait,  /* from RFC 793 */
	closed, allocated  /* internal states for connection table */
} tcp_connection_state_t;

/* Really just a note to self */
typedef enum {
	open, send, receive, close, abort, status
} tcp_event_t;

#define TCPIP_PACKET_SIZE	4096

#define TCP_DEFAULT_TTL		60

/* Hardcoded maximum queued packets */
#define MAX_TX_WINDOW_PACKETS	128
#define MAX_RX_WINDOW_PACKETS	128

typedef u_int8_t tcpip_packet_t[TCPIP_PACKET_SIZE];

typedef struct {
	IPv4_Address	local_addr;
	IPv4_Address	remote_addr;
	u_int16_t	local_port;
	u_int16_t	remote_port;
} __packme tcp_connection_t;

/* Use macros in queue.h for a linked queue for the windows
 *
 * TODO: put some macros in queue.h to make this more transparent
 */
struct _tcp_q_link {
        struct _tcp_q_link *next;
        tcpip_packet_t *item;
} __packme;
typedef struct _tcp_q_link tcp_q_link_t;

struct tcp_queue {
	tcp_q_link_t *head;
	tcp_q_link_t *tail;
} __packme;
typedef struct tcp_queue tcp_queue_t;

#if 0
#define TCP_DEFAULT_PREFERRED_WINDOW_SIZE	65535
#endif
/* a 64k window would take 4.5 seconds to xfer across a slip link. 
 */
#define TCP_DEFAULT_PREFERRED_WINDOW_SIZE	2048

typedef struct {
	u_int32_t	initial_seq; 	/* IRS */
	u_int32_t	seq_expected; 	/* RCV.NXT */
	u_int32_t	window_size;	/* RCV.WND */
	u_int16_t	urgent_ptr;	/* RCV.UP */
	u_int32_t	preferred_window_size;
} __packme tcp_receiver_state_t;

typedef struct {
	u_int32_t	initial_seq; 		/* ISS */
	u_int32_t	oldest_unack_seq;	/* SND.UNA */
	u_int32_t	seq_next;		/* SND.NXT */
	u_int32_t	window_size;		/* SND.WND */
	u_int16_t	urgent_ptr;		/* SND.UP */

	/* from the segment used to generate the last window value */
	u_int32_t	last_win_seg_seq;	/* SND.WL1 */
	u_int32_t	last_win_seg_ack;	/* SND_WL2 */

	u_int16_t	next_ip_id;		/* IP identification field */
} __packme tcp_transmitter_state_t;

typedef struct {
	tcp_connection_t	endpoints;
	soapbox_id_t		soapbox_from_application;
	soapbox_id_t		soapbox_to_application;
	tcp_connection_state_t	current_state;
	tcp_receiver_state_t	rx;
	tcp_transmitter_state_t	tx;
	tcp_queue_t		rxwindow;
	tcp_queue_t		txwindow;
	u_int32_t		mss;
} __packme tcp_state_t;

/* Packet metadata for tcp.h internal use */
typedef struct {
	struct IPv4_Header *iphead;
	struct TCP_Header *tcphead;
	void *tcpoptions;
	void *tcpdata;
	u_int32_t packetlen;
	u_int32_t ipheaderlen;
	u_int32_t tcpheaderlen;
	u_int32_t tcpdatalen;
} tcp_packetinfo_t;

/* Setup TCP state etc. */
void tcp_init();

/* Hooks for IPv4 */
void handle_inbound_tcp(message_t msg);

/* Hook to open a connection in the LISTEN state
 *
 * Should be actually called through IPC so other apps can use it, but this will do for testing
 */
tcp_state_t * tcp_create_new_listener(u_int16_t port, soapbox_id_t sb_from_application, soapbox_id_t sb_to_application);


#endif
