
#include <assert.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>
#include <cpu.h>
#include <ktimer.h>

#include "delibrium/delibrium.h"
#include "delibrium/pool.h"
#include "delibrium/timer.h"

#include "tcp.h"


#undef TCPDEBUG
#ifdef TCPDEBUG
#define TCPDEBUG_print(_s)	{print("[tcp] "); print( (char *) __func__ ); print(": " _s);}
#else
#define TCPDEBUG_print(_s)
#endif

#define TCP_CALC_CHECKSUM	1
#define TCP_DONT_CALC_CHECKSUM	0

/* Obvious problem */
#define _min(_a,_b)	(((_a) < (_b)) ? (_a) : (_b))


#define TCP_MAX_CONNECTIONS	1024

static tcp_state_t * connection_table[TCP_MAX_CONNECTIONS];

const char * tcp_state_string[] = {
        "listen", "syn_sent", "syn_received", "established", "fin_wait_1", "fin_wait_2",
        "close_wait", "closing", "last_ack", "time_wait",
        "closed", "allocated" };



static tcp_state_t * alloc_new_tcp_state() {
	tcp_state_t * tcp;
	tcp = pool_alloc(_TCP_STATE_EXPONENT);
	tcp->txwindowdata.head = NULL;
	tcp->txwindowdata.tail = NULL;
	tcp->current_state = closed;

	return tcp;
}

/* send a signal rant to an application
 */
void signal_application(tcp_state_t *tcpc, tcp_stack_signals_t s) {
	message_t sigmsg;
	sigmsg.type = signal;
	sigmsg.reply_to = tcpc->soapbox_from_application;
	sigmsg.m.signal = s;
	rant(tcpc->soapbox_to_application, sigmsg);
}

void free_tcp_state(tcp_state_t *tcpc) {
	pool_free(_TCP_STATE_EXPONENT, tcpc);
}

static inline void free_packet_memory(message_t msg) {
	if (msg.type == gestalt) freepage(msg.m.gestalt.gestalt);
}
/* Discard a packet without notification */
static inline void discard_silently(message_t msg) {
	TCPDEBUG_print("Dropping packet\n");
	free_packet_memory(msg);
}

/* Trying for pseudorandom (although not very random) initial sequences and 
 * packet ids (this breaks RFC 791  if used for sequences)
 */
static inline u_int32_t get_initial_id() {
	u_int64_t tsc;
	tsc = rdtsc();
	return (u_int32_t)(((tsc >> 16) ^ (tsc & 0xffffffff) ^ (tsc << 20)) & 0xffffffff);
}

static inline void tcp_insert_into_queue(tcp_queue_t *q, message_t msg, u_int32_t seq) {
	tcp_q_link_t * ql = pool_alloc(_TCP_Q_LINK_EXP);
	assert(ql != NULL);
	ql->first_seq = seq;
	memcpy(&(ql->usermsg), &msg, sizeof(message_t));
	QUEUE_ADDLINK(q, ql);
}
static inline void tcp_queue_deep_pop(tcp_queue_t *q) {
	/* Pop and free the head of a queue, including any pages of data */
	assert (! QUEUE_ISEMPTY(q));
	tcp_q_link_t * popped = q->head;
	QUEUE_DELETE(q);
	free_packet_memory(popped->usermsg);
	pool_free(_TCP_Q_LINK_EXP, popped);
}
static inline void tcp_deep_free_queue(tcp_queue_t *q) {
	while (! QUEUE_ISEMPTY(q)) 
		tcp_queue_deep_pop(q);
}
static inline void tcp_queue_pop_acked(tcp_queue_t *q, u_int32_t ack_next_expected) {
	/* TODO: Fix wraparround edge case */
	while (! QUEUE_ISEMPTY(q)) {
		if (q->head->first_seq + q->head->usermsg.m.gestalt.length > ack_next_expected)
			break;
		tcp_queue_deep_pop(q);
	}
}

#if 0
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
#endif


/*
 * setup connection defaults and some unique initial state
 * e.g. window size, MSS, IP ID etc
 */
static inline void init_tcp_connection(tcp_state_t *tcpc) {
	tcpc->rx.preferred_window_size = TCP_DEFAULT_PREFERRED_WINDOW_SIZE;
	tcpc->mss = TCP_DEFAULT_MSS; /* Until we know otherwise */
	tcpc->tx.initial_seq = get_initial_id(); /* TODO: implement correctly */
	tcpc->tx.next_ip_id = get_initial_id() % 0xffff; 
	tcpc->tx.seq_next = tcpc->tx.initial_seq;
	tcpc->tx.oldest_unack_seq = tcpc->tx.seq_next;

	/* RFC 2581: Initial cwnd must not be more than 2 segments, or more than 2 * SMSS
	 * (obviously don't assume remote is going to obey this)
	 *
	 * Let's be conservative. :-P
	 */
	tcpc->tx.congestion_window = tcpc->mss;
	tcpc->tx.slowstart_threshold = tcpc->rx.preferred_window_size;

	tcpc->tx.retransmit_timeout = 2500; /* ms */
	tcpc->tx.srtt = 2500;
	tcpc->tx.rtt_variance = 0;
	tcpc->tx.current_rto_id = 0;
}

/* Get a pointer to a free connection in the connection table
 * 
 * threadsafe
 *
 * returns NULL iff connection_table_items < TCP_MAX_CONNECTIONS
 */
static inline tcp_state_t * get_free_connection() {
	int i;

	for (i=0; i<TCP_MAX_CONNECTIONS; i++) {
		if (connection_table[i]->current_state == closed) {
			if (cmpxchg(connection_table[i]->current_state, allocated, closed) == closed)
				return connection_table[i];
		}
	}
	return NULL;
}

/* given an inbound TCP packet, find the matching connection if one exists
 *
 * pre: IP packet valid
 * TCP datagram may not be valid
 * returns connection state if local and remote ports and IPs are the same
 * otherwise returns connection if local port and IPs are same and 
 *           connection is in the LISTEN state
 * returns NULL if no matching connections of any kidn are found
 */
static inline tcp_state_t * match_tcp_connection(struct IPv4_Header *iphead) {
	struct TCP_Header *tcphead;
	tcp_state_t * listening_connection = NULL;
	int i;
	tcphead = (struct TCP_Header *)(( (char *)iphead ) + IPV4_EXTRACT_HEADERLEN(iphead));

	for (i=0; i< TCP_MAX_CONNECTIONS; ++i) {
		if (iphead->destination != connection_table[i]->endpoints.local_addr) continue;
		if (tcphead->destination_port != connection_table[i]->endpoints.local_port) continue;
		if (connection_table[i]->current_state == listen)
			listening_connection = connection_table[i]; /* Less specific */

		if (iphead->source != connection_table[i]->endpoints.remote_addr) continue;
		if (tcphead->source_port != connection_table[i]->endpoints.remote_port) continue;

		/* match on entire 4-tuple */
		return connection_table[i];
	}

	/* return a 2-tuple match on a listening port iff found
	 * otherwise return NULL
	 */
	return listening_connection;
}

/*
 * wrapper so that we can prefetch multiple pages later without having
 * to blow away the TLB each time
 */
static inline void * tcp_get_page() { return getpage(); }

/* Fill in a stub IP and TCP header for an outbound packet
 * IP: version, header size, ttl, proto, src addr, dest addr
 * TCP: src port, dest port
 *
 * All else are set to 0
 * Needed: 
 *   IP: total_length, identification, flags, header_checksum
 *   TCP: seq, ack, header_len (default set), flags, window, checksum, urg ptr
 */
static inline void stub_fill_outbound_header(tcp_state_t *tcpc, void *packet_base) {
	struct IPv4_Header *iphead = packet_base;
	struct TCP_Header *tcphead = packet_base + IPV4_MIN_HEADER_SIZE;

	*iphead = (struct IPv4_Header) {
			0x40 + (IPV4_MIN_HEADER_SIZE / 4),
			0, 0, 0, 0, 
			TCP_DEFAULT_TTL, IPV4_PROTO_TCP,
			0,
			tcpc->endpoints.local_addr,
			tcpc->endpoints.remote_addr };
	*tcphead = (struct TCP_Header) {
			tcpc->endpoints.local_port,
			tcpc->endpoints.remote_port,
			0, 0, (TCP_MIN_HEADER_SIZE << 2) & 0xf0, 0,
			htons(tcpc->rx.window_size), 0, 0 };
}

static inline void empty_tcpip_header(void *packet_base) {
	struct IPv4_Header *iphead = packet_base;
	struct TCP_Header *tcphead = packet_base + IPV4_MIN_HEADER_SIZE;

	*iphead = (struct IPv4_Header) {
			0x40 + (IPV4_MIN_HEADER_SIZE / 4),
			0, 0, 0, 0, 
			TCP_DEFAULT_TTL, IPV4_PROTO_TCP,
			0, 0, 0 };
	*tcphead = (struct TCP_Header) {
			0, 0, 0, 0, (TCP_MIN_HEADER_SIZE << 2) & 0xf0 , 0, 0 };
}


/* cleanup a connection that has been reset */
static void  cleanup_reset_connection(tcp_state_t * tcpc) {
	tcpc->current_state = allocated;
	tcpc->endpoints.local_addr = 0;
	tcpc->endpoints.remote_addr = 0;
	tcpc->endpoints.local_port = 0;
	tcpc->endpoints.remote_port = 0;
	tcp_deep_free_queue(&tcpc->rxwindowdata);
	tcp_deep_free_queue(&tcpc->txwindowdata);
	tcpc->current_state = closed;
}


/* ************************************************************************* */

static inline void rst_unknown_packet(message_t msg) {
	struct IPv4_Header * iphead = msg.m.gestalt.gestalt;
	struct TCP_Header * tcphead;
	tcphead = (struct TCP_Header *)(msg.m.gestalt.gestalt + IPV4_EXTRACT_HEADERLEN(iphead));

	if (tcphead->flags & TCP_FLAG_RST) {
		/* Don't RST an RST */
		discard_silently(msg);
		return;
	}

	tcp_connection_t endpoints;
	endpoints.local_addr = ipv4_local_ip;
	endpoints.remote_addr = iphead->source;
	endpoints.local_port = tcphead->destination_port;
	endpoints.remote_port = tcphead->source_port;

	u_int32_t seq = 0;
	u_int32_t ack = 0;
	if (tcphead->flags & TCP_FLAG_ACK) { 
		seq = tcphead->ack_num; 
	} else { 
		ack = ntohl(tcphead->sequence_num);
		ack += msg.m.gestalt.length - IPV4_EXTRACT_HEADERLEN(iphead) - TCP_EXTRACT_HEADERLEN(tcphead);
		if (tcphead->flags & (TCP_FLAG_SYN | TCP_FLAG_FIN))
			++ack;	/* SYN and FIN occupy 1 sequence # */
		ack = htonl(ack);
	}

	empty_tcpip_header(iphead);
	tcphead = (struct TCP_Header *)(msg.m.gestalt.gestalt + IPV4_MIN_HEADER_SIZE);
	memcpy((char *)(&(iphead->source)), (char *)(&endpoints), sizeof(endpoints));
	tcphead->sequence_num = seq;
	tcphead->ack_num = ack;
	tcphead->flags = ack ? (TCP_FLAG_RST | TCP_FLAG_ACK) : TCP_FLAG_RST;
	tcphead->checksum = 0;
	tcphead->checksum =  ipv4_tcp_checksum(endpoints.local_addr, endpoints.remote_addr, tcphead, TCP_MIN_HEADER_SIZE);
	msg.m.gestalt.length = IPV4_MIN_HEADER_SIZE  + TCP_MIN_HEADER_SIZE;

	TCPDEBUG_print("(sending RST)\n");
	ipv4_checksum_and_send(msg);
}

static inline void checksum_outbound_tcp(tcp_state_t * tcpc, message_t msg, size_t tcp_seg_size) {
	struct TCP_Header * tcphead = (struct TCP_Header *)(msg.m.gestalt.gestalt + IPV4_MIN_HEADER_SIZE);
	tcphead->checksum =  ipv4_tcp_checksum(tcpc->endpoints.local_addr, tcpc->endpoints.remote_addr, tcphead, tcp_seg_size);
}

static message_t build_outbound_tcp_packet(tcp_state_t * tcpc, u_int32_t seq, u_int32_t ack,u_int16_t flags, u_int16_t window, u_int16_t urgent_ptr, size_t data_length, bool do_checksum) {
	message_t msg;
	msg.type = gestalt;
	msg.m.gestalt.gestalt = tcp_get_page();
	msg.m.gestalt.length = IPV4_MIN_HEADER_SIZE + TCP_MIN_HEADER_SIZE + data_length;
	struct IPv4_Header *iphead = msg.m.gestalt.gestalt;
	struct TCP_Header *tcphead = msg.m.gestalt.gestalt + IPV4_MIN_HEADER_SIZE;

	stub_fill_outbound_header(tcpc, iphead);
	iphead->total_length = msg.m.gestalt.length;
	iphead->identification = htons(tcpc->tx.next_ip_id);
	tcpc->tx.next_ip_id++;
	tcphead->sequence_num = seq;
	tcphead->ack_num = ack;
	tcphead->flags = flags;
	tcphead->window = window;
	tcphead->urgent_pointer = urgent_ptr;
	if (do_checksum)
		checksum_outbound_tcp(tcpc, msg, TCP_MIN_HEADER_SIZE + data_length);

	return msg;
}

/* Just send ... an ACK! */
static inline void send_an_ack(tcp_state_t * tcpc) {
	ipv4_checksum_and_send(
		build_outbound_tcp_packet(tcpc,
			htonl(tcpc->tx.seq_next),
			htonl(tcpc->rx.seq_expected),
			TCP_FLAG_ACK, htons(tcpc->rx.window_size), 0,
			0, TCP_CALC_CHECKSUM)
	);
}

/***********************************************************************  
 *  Received packet event handlers 
 */

static inline void handle_listen_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	tcp_state_t * newtcpc;

	/* Only state change for SYN packets */
	if (!(p.tcphead->flags & TCP_FLAG_SYN))  {
		rst_unknown_packet(msg); return;
	}

	/* Don't state change for packets that have ACK, RST or FIN set */
	if (p.tcphead->flags & (TCP_FLAG_ACK | TCP_FLAG_RST | TCP_FLAG_FIN))  {
		if (p.tcphead->flags & TCP_FLAG_RST)
			discard_silently(msg);
		else
			rst_unknown_packet(msg);
		return;
	}

	/* Duplicate the TCP state for the new connection */
	newtcpc = get_free_connection();
	if (newtcpc == NULL) {
		TCPDEBUG_print("Couldn't get a new free connection slot. Sending RST.\n");
		rst_unknown_packet(msg);
		return;
	}
	newtcpc->current_state = allocated;
	memcpy(newtcpc, tcpc, sizeof(tcp_state_t));
	newtcpc->txwindowdata.head = NULL;
	newtcpc->txwindowdata.tail = NULL;

	newtcpc->soapbox_from_application = tcpc->soapbox_from_application;
	newtcpc->soapbox_to_application = tcpc->soapbox_to_application;

	/* TODO: check remote address isn't broadcast etc */
	newtcpc->endpoints.remote_addr = p.iphead->source;
	newtcpc->endpoints.remote_port = p.tcphead->source_port;

	/* TODO: Figure out why these are tripping */
#if 0
	assert(newtcpc->endpoints.local_addr == p.iphead->destination);
	assert(newtcpc->endpoints.local_port == p.tcphead->destination_port);
#endif

	newtcpc->soapbox_from_application = get_new_anon_soapbox();
	assert(newtcpc->soapbox_from_application);

	tcpc = newtcpc;

	/* Setup initial seq, seq_next, etc */
	init_tcp_connection(tcpc);

	tcpc->rx.initial_seq = ntohl(p.tcphead->sequence_num);
	tcpc->rx.seq_expected = tcpc->rx.initial_seq + 1;

	tcpc->tx.seq_next += 1; /* Consume 1 seq # for the SYN */

	/* Send SYN and ACK incoming SYN */
	ipv4_checksum_and_send( build_outbound_tcp_packet(tcpc, 
				htonl(tcpc->tx.initial_seq), htonl(tcpc->rx.seq_expected), 
				TCP_FLAG_SYN | TCP_FLAG_ACK, htons(tcpc->rx.window_size), 0,
				0, TCP_CALC_CHECKSUM)
				);

	TCPDEBUG_print("State change: allocated -> SYN_RECEIVED\n");
	tcpc->current_state = syn_received;

	/* TODO:  actually send packet to be processed by SYN_RECEIVED state handler is implied by RFC793 */
	free_packet_memory(msg);
}

/* given the header of an inbound TCP packet, and the TCP connection state
 * return >0 iff the seq # is in our receive window
 * otherwise return 0
 */
static inline int is_packet_in_rx_window(tcp_state_t * tcpc, tcp_packetinfo_t p) {
	u_int32_t segseq = ntohl(p.tcphead->sequence_num);
	u_int32_t rxwin_left = tcpc->rx.seq_expected;
	u_int32_t rxwin_right = rxwin_left + tcpc->rx.window_size;
	
	/* Empty segment but seq as expected */
	if (p.tcpdatalen == 0 && tcpc->rx.window_size == 0 && segseq == rxwin_left)
	    	return 1;

	/* Empty segment but seq within (non 0 sized) window */
	if (p.tcpdatalen == 0 && tcpc->rx.window_size > 0 && segseq >= rxwin_left && segseq < rxwin_right)
	    	return 1;

	/* Data in segment with 0 sized window */
	if (p.tcpdatalen > 0 && tcpc->rx.window_size == 0)
		return 0;
	
	/* Data in segment and non-0 sized window */
	if (p.tcpdatalen > 0 && tcpc->rx.window_size > 0) {
		if (segseq >= rxwin_left && segseq < rxwin_right)
			return 1;
		if (segseq+p.tcpdatalen-1 >= rxwin_left && segseq+p.tcpdatalen-1 < rxwin_right)
			return 1;
	}

	return 0;
}

static inline void print_tcp_header(struct TCP_Header *tcphead) {
	printf(" <sp=%u,dp=%u,seq=%u,ack=%u [%c%c%c%c%c%c] win=%u,csum=%4x,up=%u>\n",
		ntohs(tcphead->source_port), ntohs(tcphead->destination_port),
		ntohl(tcphead->sequence_num), ntohl(tcphead->ack_num),
		(tcphead->flags & TCP_FLAG_URG) ? 'U' : '.', (tcphead->flags & TCP_FLAG_ACK) ? 'A' : '.',
		(tcphead->flags & TCP_FLAG_PSH) ? 'P' : '.', (tcphead->flags & TCP_FLAG_RST) ? 'R' : '.',
		(tcphead->flags & TCP_FLAG_SYN) ? 'S' : '.', (tcphead->flags & TCP_FLAG_FIN) ? 'F' : '.',
		ntohs(tcphead->window), ntohs(tcphead->checksum), ntohs(tcphead->urgent_pointer));
}

static inline void handle_established_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	/* TODO: Do packet prediction to bypass most of this for the expected case */
	/* TODO: Queue out of order packets to be handled in sequence order when possible */

	if (! is_packet_in_rx_window(tcpc, p)) {
		TCPDEBUG_print("Segment not in rx window\n");
		if (p.tcphead->flags & TCP_FLAG_RST)  
			{ discard_silently(msg); return; }
		send_an_ack(tcpc);
		free_packet_memory(msg);
		return;
	}

	/* segment is within rx window. Check for other weirdness */

	if (p.tcphead->flags & (TCP_FLAG_RST | TCP_FLAG_SYN))  {
		/* TODO: Notify application */
		TCPDEBUG_print("RST or SYN received. Bad state. Resetting connection."); 
		cleanup_reset_connection(tcpc);
		discard_silently(msg);
		return;
	}
	if (! (p.tcphead->flags & TCP_FLAG_ACK)) {
		discard_silently(msg); /* ACK should have been set and wasn't */
		return;
	}

	/* Update tx state based on segment ack field */
	/* TODO: Fix 32-bit wraparound here (and elsewhere) */
	u_int32_t seg_ack = ntohl(p.tcphead->ack_num);
	u_int32_t seg_seq = ntohl(p.tcphead->sequence_num);
	if (seg_ack > tcpc->tx.seq_next) {
		/* Errr, how can you ack something we haven't sent? */
		send_an_ack(tcpc);
		free_packet_memory(msg);
		return;
	}
	if (seg_ack > tcpc->tx.oldest_unack_seq) {
		/* Yay. They've acked more stuff */
		/* TODO: send more stuff */
		tcpc->tx.oldest_unack_seq = seg_ack;

		/* Invalidate the current RTO timer */
		tcpc->tx.current_rto_id++;

		/* Free fully acked data blocks */
		tcp_queue_pop_acked(&tcpc->txwindowdata, seg_ack);

		/* Set another timer iff there is unacked data outstanding */
		if (tcpc->tx.oldest_unack_seq < tcpc->tx.seq_next)
			add_timer(tcpc->soapbox_from_application, 
				  tcpc->tx.retransmit_timeout * 1000, 1,
				  tcpc->tx.current_rto_id);
	}

	if (seg_ack >= tcpc->tx.oldest_unack_seq) {
		/* Update the send window!
		 *
		 * Only update the send window based on a more recent
		 * segment than last time. If the seq and ack are the
		 * same, check anyhow; The remote might be opening up 
		 * their window
		 */
		if ((seg_seq > tcpc->tx.last_win_seg_seq) || 
		    ((seg_seq == tcpc->tx.last_win_seg_seq) && (seg_ack >= tcpc->tx.last_win_seg_ack)) ) {
			/* update TX window state
			 * Accept shrinking windows, even if caused by out of order packets :-( 
			 */
			tcpc->tx.window_size = ntohs(p.tcphead->window);
			tcpc->tx.last_win_seg_seq = seg_seq;
			tcpc->tx.last_win_seg_ack = seg_ack;
		}
	}

	/* TODO: Check for URG flag, and deal with urgent pointer.
	 * This is probably going to require DeLiRiuM rant queuing to
	 * handle a queue-head push as well as the default tail append
	 */

	/* TODO: REMOVE THIS BAD BAD BAD HACK! */
	if (p.tcphead->flags & (TCP_FLAG_FIN))  {
		signal_application(tcpc, remote_reset);
		
		TCPDEBUG_print("FIN received. Resetting connection."); 
		ipv4_checksum_and_send(
			build_outbound_tcp_packet(tcpc,
				htonl(tcpc->tx.seq_next),
				htonl(seg_seq + 1),
				TCP_FLAG_ACK | TCP_FLAG_FIN, htons(tcpc->rx.window_size), 0,
				0, TCP_CALC_CHECKSUM)
		);
		cleanup_reset_connection(tcpc);
		free_packet_memory(msg);
		return;
	}

	/* Process any data inside segment and send it to the user. Yay! 
	 */ 
	if (p.tcpdatalen > 0) {
		if (tcpc->rx.seq_expected != seg_seq) {
			TCPDEBUG_print("Erk! Out of order packet. Can't send to user.\n");
			TCPDEBUG_print("Panicing and dropping. Also immediately ACKing to trigger fast retransmit\n");
			send_an_ack(tcpc);
			free_packet_memory(msg);
			return;
		}

		if (tcpc->rx.window_size < p.tcpdatalen) {
			assert(tcpc->rx.window_size > 0);
			/* Blunt hack to ignore the data bigger than the window */
			p.tcpdatalen = tcpc->rx.window_size;
		}

		/* Update the receive window size as per RFC 1122 (4.2.3.3) */
		if ( (tcpc->rx.preferred_window_size - tcpc->rx.window_size - p.tcpdatalen)  >=
		     _min( tcpc->rx.preferred_window_size / 2, tcpc->mss) ) {
			tcpc->rx.window_size = tcpc->rx.preferred_window_size - p.tcpdatalen;
		}

		tcpc->rx.window_size -= p.tcpdatalen;
		tcpc->rx.seq_expected += p.tcpdatalen;

		/* Shift down in memory and overwrite the headers. memcpy should deal iff dest < src 
	 	 * Then just change the logical message length and queue it to the application.
	 	 *
	 	 * TODO: Ponder if it is worthwhile just changing the msg.m.gestalt.gestalt pointer
	 	 *       and adding page allocation metadata to msg.m.gestalt. This make this
	 	 *       zero-copy, and also have explicit memory handling (although it would
	 	 *       unfortunately restrict msg.m.gestalt to page-sized memory ops)
	 	 */
		memcpy(msg.m.gestalt.gestalt, p.tcpdata, p.tcpdatalen); 
		msg.m.gestalt.length = p.tcpdatalen;
		msg.reply_to = tcpc->soapbox_from_application;
		rant(tcpc->soapbox_to_application, msg);
	} 
	else {
		/* Release the memory here as responsibility to do so wasn't
		 * done by the data handler
		 */
		free_packet_memory(msg);
	}

	/* TODO: Don't just ack here. REALLY.
	 * Should queue it for sending on the next TX event (timer or app generated)
	 * as long as it's not >= 500ms away.
	 */
	send_an_ack(tcpc);
}

static inline void handle_syn_rcvd_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	/* Make sure the packet falls within our acceptable rx window */
	
	if (! is_packet_in_rx_window(tcpc, p)) {
		TCPDEBUG_print("not in rx window. ACKing and dropping.");
		if (! (p.tcphead->flags & TCP_FLAG_RST))  {
			send_an_ack(tcpc);
		}
		else { TCPDEBUG_print("(not ACKing due to RST)"); }
		free_packet_memory(msg);
		return;
	}

	if (p.tcphead->flags & TCP_FLAG_SYN) {
		/* SYN flag in TCP window invalud.
		 * RFC1122 requires state change back to listen if we are passive
		 */
		/* TODO: Do this properly */
		TCPDEBUG_print("SYN received in SYN_RECEIVED state. State change to CLOSED.\n");
		discard_silently(msg);
		cleanup_reset_connection(tcpc);
		return;
	}

	if (! (p.tcphead->flags & TCP_FLAG_ACK)) {
		/* No ACK flag. Discard */
		TCPDEBUG_print("No ACK flag in SYN_RECEIVED state. Discarding.\n");
		discard_silently(msg);
		return;
	}

	if (ntohl(p.tcphead->ack_num) > tcpc->tx.seq_next || 
	    tcpc->tx.oldest_unack_seq > ntohl(p.tcphead->ack_num)  ) {

	    /* Hmmm. Unexplained (or at least incorrect) ACK. RFC 793 
	     * says we send an RST (I think. It reads weirdly there.
	     * Or I'm under caffeinated.) */
		rst_unknown_packet(msg);
		return;
		
	}

	/* RFC 1122: State change to ESTABLISHED, set :
	 *
	 * SND.WND <- SEG.WND
	 * SND.WL1 <- SEG.SEQ
	 * SND.WL2 <- SEG.ACK
	 */ 

	/* Set the transmit window size based on what we have been told from 
	 * the remote end */
	tcpc->tx.window_size = ntohl(p.tcphead->window);

	/* Update the stored seq and ack from the same segment */
	tcpc->tx.last_win_seg_seq = ntohl(p.tcphead->sequence_num);
	tcpc->tx.last_win_seg_ack = ntohl(p.tcphead->ack_num);

	/* open up the receive window :-) */
	tcpc->rx.window_size = tcpc->rx.preferred_window_size;

	/* Hook up the application soapbox to the correct receiver now that
	 * we are in the TCP thread and connection is established.
	 *
	 * TODO: Notify client of connection state changes via a signal
	 *
	 * TODO: Handle better having the same listening port having multiple
	 *       connections. Will probably have to generate a soapbox pair on
	 *       connect and send it to the application
	 */
	/* note: supplicate can deal with being called multiple times,
	 * and we don't want to memory leak any msgs rxed between a renounce 
	 * and a supplicate just to change handlers */
	extern void tcp_handle_outbound_data(message_t msg);
	supplicate(tcpc->soapbox_from_application, tcp_handle_outbound_data);
	
	TCPDEBUG_print("State change: SYN_RECEIVED -> ESTABLISHED\n");
	tcpc->current_state = established;
	signal_application(tcpc, listener_has_new_established);

	/* RFC 793 says 'Continue processing' the packet. 
	 * We'll just feed it into the ESTABLISHED state packet handler
	 */
	handle_established_inbound(tcpc, msg, p);
}

static inline void handle_syn_sent_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	TCPDEBUG_print("stub\n"); discard_silently(msg); 
}

static inline void handle_passive_close_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	TCPDEBUG_print("stub\n"); discard_silently(msg); 
}

static inline void handle_active_close_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	TCPDEBUG_print("stub\n"); discard_silently(msg); 
}

/* **********************************************************************  */

/* Dump spurious message data from an application.
 * WARNING: This may be running either in or out of the TCP thread
 *
 * pre: connection state in (closed, assigned, listening, fin_wait_1, 
 *      fin_wait_2, last_ack, closing) or connection has no known state
 */
void tcp_ignore_data(message_t msg) { 
	TCPDEBUG_print("Ignored application data\n");
	free_packet_memory(msg);
}

/* Create a new outbound segment containing count bytes from inbuf with
 * for the connection tcpc with the seq # of sequence
 *
 * TODO: Rework this to be zero-copy. So many other things are.
 */
static inline message_t create_new_data_segment(tcp_state_t * tcpc, void *inbuf, size_t count, u_int32_t sequence, u_int16_t flags) {
	message_t seg_msg;

	seg_msg = build_outbound_tcp_packet(tcpc, 
			htonl(sequence), htonl(tcpc->rx.seq_expected), flags, 
			htonl(tcpc->tx.window_size), 0,
			count, TCP_DONT_CALC_CHECKSUM);
	memcpy(((char *)seg_msg.m.gestalt.gestalt)+IPV4_MIN_HEADER_SIZE+TCP_MIN_HEADER_SIZE, inbuf, count);
	checksum_outbound_tcp(tcpc, seg_msg, count + TCP_MIN_HEADER_SIZE);
	return seg_msg;
}

static inline void queue_outbound_data_segment(tcp_state_t * tcpc, message_t msg) {
	/* TODO: Actually queue for transmission rather than sending if appropriate*/
	/* TODO: Check that the receive window is big enough */
	/* TODO: Place data on un-acked data queue */
	ipv4_checksum_and_send(msg);
}

static inline void queue_outbound_user_data(tcp_state_t *tcpc, message_t msg) {
	/* Okay, split the message up into segments of MSS and queue */
	assert(msg.type == gestalt);
	char * buf = msg.m.gestalt.gestalt;
	size_t bufsize = msg.m.gestalt.length;
	u_int32_t startseq = tcpc->tx.seq_next;

	if (bufsize == 0) {
		free_packet_memory(msg); 
		return;
	}

	while (bufsize) {
		size_t segsize;
		message_t newseg_msg;
		int flags = 0;

		segsize = _min(bufsize, tcpc->mss);
		if (tcpc->current_state == established || tcpc->current_state == syn_received) 
			flags |= TCP_FLAG_ACK;
		if (bufsize == segsize)
			flags |= TCP_FLAG_PSH;
		newseg_msg = create_new_data_segment(tcpc, buf, segsize, tcpc->tx.seq_next, flags);

		bufsize -= segsize;
		buf += segsize;
		tcpc->tx.seq_next += segsize;
		queue_outbound_data_segment(tcpc, newseg_msg);
	}

	/* Put the user data into the retransmission data queue */
	tcp_insert_into_queue(&tcpc->txwindowdata, msg, startseq);

	/* Restart the retransmission timer */
	tcpc->tx.current_rto_id++;
	add_timer(tcpc->soapbox_from_application, 
		  tcpc->tx.retransmit_timeout * 1000, 1, tcpc->tx.current_rto_id);
}

static inline void tcp_handle_rto_timer(tcp_state_t * tcpc, message_t msg) {
	if (tcpc->tx.current_rto_id != (u_int32_t)msg.m.signal)
		return; /* ignore expired timer */
	switch (tcpc->current_state) {
		case closed: case listen: case allocated: case time_wait:
			return;
		default: ;
	}
	printf("Blatently ignoring legit RTO timer. state: %s\n",
		tcp_state_string[(int)(tcpc->current_state)]);
}

/* Process data an application wishes to send out via TCP
 * Runs in the ipv4 thread
 */
void tcp_handle_outbound_data(message_t msg) {
	tcp_state_t * tcpc = NULL;
	int i;


	/* Find the associated TCP connection */
	for (i=0; i<TCP_MAX_CONNECTIONS; i++) {
		if (msg.destination == connection_table[i]->soapbox_from_application) {
			tcpc = connection_table[i];
			if (tcpc->current_state != listen)
				break;
		}
	}
	
	if (msg.type == signal) {
		/* RTO timer event */
		tcp_handle_rto_timer(tcpc, msg);
	}

	if (msg.type != gestalt) return;

	if (tcpc == NULL || tcpc->current_state == closed) {
		TCPDEBUG_print("spurious message received without associated connection. ignoring.\n");
		tcp_ignore_data(msg);
		return;
	}
	switch (tcpc->current_state) {
		case closed:
		case allocated:
		case listen:
		case fin_wait_1:
		case fin_wait_2:
		case closing:
		case last_ack:
		case time_wait:
			TCPDEBUG_print("application tried to send to a connection in an invalid state. ignoring\n");
			/* TODO: Notify the application of the error */
			tcp_ignore_data(msg);
			return;

		case syn_sent:
		case syn_received:
			TCPDEBUG_print("warning: sending data in SYN_SENT or SYN_RECEIVED is bad. Should queue.\n");
			/* fall-through */

		case established:
		case close_wait:
			queue_outbound_user_data(tcpc,msg);
			return;
	}
 	assert(0); /* EPIC FAIL */
}


/* **********************************************************************  */

/* Process TCP/IP packets from the outside world.
 * Running in the ipv4 thread
 */
void handle_inbound_tcp(message_t msg) {
	tcp_state_t * tcpc;
	tcp_packetinfo_t p;

	assert(msg.type == gestalt); /* Probably a bit late :-) */

	/* store some packet metadata.
	 * remember that this isn't bounds checked etc.
	 */
	p.packetlen = msg.m.gestalt.length;
	p.iphead = msg.m.gestalt.gestalt;
	p.ipheaderlen = IPV4_EXTRACT_HEADERLEN(p.iphead);
	p.tcphead = msg.m.gestalt.gestalt + p.ipheaderlen;
	p.tcpheaderlen = TCP_EXTRACT_HEADERLEN(p.tcphead);
	p.tcpoptions = msg.m.gestalt.gestalt + p.ipheaderlen + TCP_MIN_HEADER_SIZE;
	p.tcpdata = msg.m.gestalt.gestalt + p.ipheaderlen + p.tcpheaderlen;
	p.tcpdatalen = p.packetlen - p.ipheaderlen - p.tcpheaderlen;

	#ifdef TCPDEBUG
	print_tcp_header(p.tcphead);
	#endif

	if (ipv4_tcp_checksum(p.iphead->source, p.iphead->destination, p.tcphead, p.packetlen - p.ipheaderlen) != 0) {
		TCPDEBUG_print("bad TCP checksum\n");
		discard_silently(msg);
	}

	tcpc = match_tcp_connection(p.iphead);
	if (tcpc == NULL) {
		rst_unknown_packet(msg);
		return;
	}

	switch (tcpc->current_state) {
		case closed:
		case allocated:
			rst_unknown_packet(msg); return;
		case listen:
			handle_listen_inbound(tcpc, msg, p); return;
		case syn_received:
			handle_syn_rcvd_inbound(tcpc, msg, p); return;
		case syn_sent:
			handle_syn_sent_inbound(tcpc, msg, p); return;
		case established:
			handle_established_inbound(tcpc, msg, p); return;
		case close_wait:
		case last_ack:
			handle_passive_close_inbound(tcpc, msg, p); return;
		case fin_wait_1:
		case fin_wait_2:
		case closing:
		case time_wait:
			handle_active_close_inbound(tcpc, msg, p); return;
	}
	//assert(tcpc->current_state != tcpc->current_state); /* assert false */
	TCPDEBUG_print("Spurious state!\n");
	discard_silently(msg);
}


/* ************************************************************************* */

/* setup a TCP connection in the LISTEN state.
 * returns NULL iff it was unable to allocate the connection
 * otherwise returns a pointer to the state of the new TCP connection
 *
 * threadsafe 
 * (mainly because get_free_connection is and current_state is set to listen last)
 *
 * pre: port is not already listening
 */
tcp_state_t * tcp_create_new_listener(u_int16_t port, soapbox_id_t sb_from_application, soapbox_id_t sb_to_application) {
	tcp_state_t * tcpc;

	tcpc = get_free_connection();
	if (tcpc == NULL) return NULL; /* No free connections */
	assert(tcpc->current_state == allocated);

	tcpc->soapbox_from_application = sb_from_application;
	tcpc->soapbox_to_application = sb_to_application;
	tcpc->endpoints.local_addr = ipv4_local_ip;
	tcpc->endpoints.remote_addr = 0;
	tcpc->endpoints.local_port = port;
	tcpc->endpoints.remote_port = 0;

	/* Ignore data until connected */
	supplicate(tcpc->soapbox_from_application, tcp_ignore_data);

	tcpc->current_state = listen;

	return tcpc;
}

/* ************************************************************************* */

/*
 * Init the TCP part of the stack
 */
void tcp_init() {
	int i;

	TCPDEBUG_print("Setting up TCP\n");
	setup_pools();
	assert(sizeof(tcp_state_t) <= (2 << _TCP_STATE_EXPONENT));
	assert(sizeof(tcp_q_link_t) <= (2 << _TCP_Q_LINK_EXP));
	for (i=0; i< TCP_MAX_CONNECTIONS; ++i)
		connection_table[i] = alloc_new_tcp_state();

#define TCP_TEST_SERVER
#ifdef TCP_TEST_SERVER
	extern void setup_test_server();
	setup_test_server();
#endif
}
