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

extern const char * tcp_state_string[];

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

#define  _TCP_Q_LINK_EXP	5
struct _tcp_q_link {
        struct _tcp_q_link *next;
        u_int32_t first_seq; /* seq # of the first byte */
        message_t usermsg;
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
#define INITIAL_RTO_MS				2500

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

	u_int32_t	congestion_window;	/* cwnd */
	u_int32_t	slowstart_threshold;	/* ssthresh */
	u_int32_t	retransmit_timeout;	/* RTO  - in ms */
	u_int32_t	srtt;
	u_int32_t	rtt_variance;

	/* from the segment used to generate the last window value */
	u_int32_t	last_win_seg_seq;	/* SND.WL1 */
	u_int32_t	last_win_seg_ack;	/* SND_WL2 */

	/* Give each timer instance an ID number, with the most
	 * recent saved here. Any timer id other than this one 
	 * will be ignored by the RTO handler. This saves us having
	 * to modify a single timeout. This is pretty ugly though.
	 * It uses too many timers.
	 */
	u_int32_t	current_rto_id;	

	u_int16_t	next_ip_id;		/* IP identification field */
} __packme tcp_transmitter_state_t;


#define _TCP_STATE_EXPONENT	7
typedef struct {
	tcp_connection_t	endpoints;
	soapbox_id_t		soapbox_from_application;
	soapbox_id_t		soapbox_to_application;
	tcp_connection_state_t	current_state;
	tcp_receiver_state_t	rx;
	tcp_transmitter_state_t	tx;
	tcp_queue_t		rxwindowdata;
	tcp_queue_t		txwindowdata;
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


/* Signals sent from the TCP to the application
 */
typedef enum {
	/* noop: Ignore */
	server_noop,	

	/* listener_has_new_established:
	 *
	 * LISTEN state connection has a new connection in the
	 * ESTABLISHED state. 
	 *
	 * Message reply_to is set to the soapbox that will be
	 * used for the new connection. Any signals about this
	 * connection will have the same reply_to set.
	 */
	listener_has_new_established,

	/* remote_reset:
	 *
	 * The remote has reset this connection.
	 * Connection is immediately forced closed.
	 */
	remote_reset,

	/* remote_finished_sending:
	 *
	 * The remote has finished sending data.
	 */
	remote_finished_sending,

	/* connection_closed:
	 *
	 * This connection is closed. Send no further data.
	 *
	 * (Note: This is not guaranteed to be sent when the connection
	 *  is closed, and is more an error response)
	 */
	connection_closed,

	/* source_quench:
	 *
	 * ICMP_SOURCE_QUENCH received.
	 */
	source_quench,

	/* zero_length_window:
	 *
	 * The remote connection currently has a 0 sized window
	 * and you are sending data. This is only an advisory, but
	 * the application should realise that we have to buffer
	 * messages in memory at the moment
	 */
	zero_length_window

} tcp_stack_signals_t;

/* Signals sent from the application to the TCP
 */
typedef enum {
	app_noop,		/* Ignore */

	/* Please force close this connection and don't send the
	 * application any more data. The application is likely to
	 * not attempt receive any more rants to this soapbox
	 */
	force_close,

	/* finished sending
	 *
	 * I have finished sending data for this connection.
	 */
	local_finish_sending,
} tcp_app_signals_t;
#endif
