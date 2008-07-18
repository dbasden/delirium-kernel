
#include <assert.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>
#include <cpu.h>

#include "delibrium/delibrium.h"
#include "delibrium/pool.h"
#include "delibrium/timer.h"

#include "tcp.h"


#define TCPDEBUG
#ifdef TCPDEBUG
#define TCPDEBUG_print(_s)	{print("[tcp] "); print( (char *) __func__ ); print(": " _s);}
#else
#define TCPDEBUG_print(_s)
#endif

/* Obvious problem */
#define _min(_a,_b)	(((_a) < (_b)) ? (_a) : (_b))


#define TCP_MAX_CONNECTIONS	16

static tcp_state_t * connection_table[TCP_MAX_CONNECTIONS];
static size_t connection_table_items;

#define _TCP_STATE_EXPONENT	7

static tcp_state_t * alloc_new_tcp_state() {
	tcp_state_t * tcp;
	tcp = pool_alloc(_TCP_STATE_EXPONENT);
	tcp->txwindow.head = NULL;
	tcp->txwindow.tail = NULL;
	tcp->current_state = closed;

	return tcp;
}

void free_tcp_state(tcp_state_t *tcp) {
	pool_free(_TCP_STATE_EXPONENT, tcp);
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

	TCPDEBUG_print("sending RST\n");

	ipv4_checksum_and_send(msg);
}

static inline void checksum_outbound_tcp(tcp_state_t * tcpc, message_t msg) {
	struct TCP_Header * tcphead = (struct TCP_Header *)(msg.m.gestalt.gestalt + IPV4_MIN_HEADER_SIZE);
	tcphead->checksum =  ipv4_tcp_checksum(tcpc->endpoints.local_addr, tcpc->endpoints.remote_addr, tcphead, TCP_MIN_HEADER_SIZE);
}

static message_t build_outbound_tcp_packet(tcp_state_t * tcpc, u_int32_t seq, u_int32_t ack,u_int16_t flags, u_int16_t window, u_int16_t urgent_ptr) {
	message_t msg;
	msg.type = gestalt;
	msg.m.gestalt.gestalt = tcp_get_page();
	msg.m.gestalt.length = IPV4_MIN_HEADER_SIZE + TCP_MIN_HEADER_SIZE;
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
	checksum_outbound_tcp(tcpc, msg);

	return msg;
}

/* Just send ... an ACK! */
static inline void send_an_ack(tcp_state_t * tcpc) {
	ipv4_checksum_and_send(
		build_outbound_tcp_packet(tcpc,
			htonl(tcpc->tx.seq_next),
			htonl(tcpc->rx.seq_expected),
			TCP_FLAG_ACK, htons(tcpc->rx.window_size), 0)
	);
}


/* **********************************************************************  */

/* Received packet event handlers */


static inline void handle_listen_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	tcp_state_t * newtcpc;

	/* Only state change for SYN packets */
	if (!(p.tcphead->flags & TCP_FLAG_SYN))  {
		TCPDEBUG_print("No SYN on packet\n");
		rst_unknown_packet(msg); return;
	}

	/* Don't state change for packets that have ACK, RST or FIN set */
	if (p.tcphead->flags & (TCP_FLAG_ACK | TCP_FLAG_RST | TCP_FLAG_FIN))  {
		TCPDEBUG_print("extra flags set on packet\n");
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
	newtcpc->txwindow.head = NULL;
	newtcpc->txwindow.tail = NULL;

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

	tcpc = newtcpc;

	/* Setup initial seq, seq_next, etc */
	init_tcp_connection(tcpc);

	tcpc->rx.initial_seq = ntohl(p.tcphead->sequence_num);
	tcpc->rx.seq_expected = tcpc->rx.initial_seq + 1;

	tcpc->tx.seq_next += 1; /* Consume 1 seq # for the SYN */

	/* Send SYN and ACK incoming SYN */
	ipv4_checksum_and_send( build_outbound_tcp_packet(tcpc, 
				htonl(tcpc->tx.initial_seq), htonl(tcpc->rx.seq_expected), 
				TCP_FLAG_SYN | TCP_FLAG_ACK, htons(tcpc->rx.window_size), 0)
				);

	TCPDEBUG_print("State change: allocated -> SYN_RECEIVED\n");
	tcpc->current_state = syn_received;

	/* TODO: 
	 * - actually send packet to be processed by SYN_RECEIVED state handler is implied by RFC793
	 */
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
	if (p.tcpdatalen == 0 && tcpc->rx.window_size == 0 
	    && segseq == rxwin_left)
	    	return 1;

	/* Empty segment but seq within (non 0 sized) window */
	if (p.tcpdatalen == 0 && tcpc->rx.window_size > 0 
	    && segseq >= rxwin_left
	    && segseq < rxwin_right)
	    	return 1;

	/* Data in segment with 0 sized window */
	if (p.tcpdatalen > 0 && tcpc->rx.window_size == 0)
		return 0;
	
	/* Data in segment and non-0 sized window */
	if (p.tcpdatalen > 0 && tcpc->rx.window_size > 0) {
		/* segseq within window */
		if (segseq >= rxwin_left && segseq < rxwin_right)
			return 1;
	
		if (segseq+p.tcpdatalen-1 >= rxwin_left 
		    && segseq+p.tcpdatalen-1 < rxwin_right)
			return 1;
	}

	return 0;
}

static inline void print_tcp_header(struct TCP_Header *tcphead) {
	printf(" <sp=%u,dp=%u,seq=%u,ack=%u [%c%c%c%c%c%c] win=%u,csum=%4x,up=%u>\n",
		ntohs(tcphead->source_port), ntohs(tcphead->destination_port),
		ntohl(tcphead->sequence_num), ntohl(tcphead->ack_num),
		(tcphead->flags & TCP_FLAG_URG) ? 'U' : '.',
		(tcphead->flags & TCP_FLAG_ACK) ? 'A' : '.',
		(tcphead->flags & TCP_FLAG_PSH) ? 'P' : '.',
		(tcphead->flags & TCP_FLAG_RST) ? 'R' : '.',
		(tcphead->flags & TCP_FLAG_SYN) ? 'S' : '.',
		(tcphead->flags & TCP_FLAG_FIN) ? 'F' : '.',
		ntohs(tcphead->window), ntohs(tcphead->checksum), ntohs(tcphead->urgent_pointer));
}

static inline void handle_established_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
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
		/* TODO: Notify application */
		TCPDEBUG_print("FIN received. Resetting connection."); 
		ipv4_checksum_and_send(
			build_outbound_tcp_packet(tcpc,
				htonl(tcpc->tx.seq_next),
				htonl(seg_seq + 1),
				TCP_FLAG_ACK | TCP_FLAG_FIN, htons(tcpc->rx.window_size), 0)
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

		#if 0
		/* TODO: Update rx window size (correctly) 
		 * WARNING: This logic mgiht be munted !!! */
		if (tcpc->rx.window_size <= (tcpc->rx.preferred_window_size / 2))
			tcpc->rx.window_size = tcpc->rx.preferred_window_size - tcpc->rx.window_size;
		#endif

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
		rant(tcpc->soapbox_to_application, msg);
	} 

	/* TODO: Don't just ack here. REALLY.
	 * Should queue it for sending on the next TX event (timer or app generated)
	 * as long as it's not >= 500ms away.
	 */
	send_an_ack(tcpc);
	free_packet_memory(msg);
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
	tcpc->current_state = established;
	TCPDEBUG_print("State change: SYN_RECEIVED -> ESTABLISHED\n");

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

/* Process data from an application and send it out to the world
 *
 * Run in the ipv4 thread
 */
void tcp_handle_outbound_data(message_t msg) {
	tcp_state_t * tcpc = NULL;
	int i;

	if (msg.type != gestalt) return;

	/* Find the associated TCP connection */
	for (i=0; i<TCP_MAX_CONNECTIONS; i++) {
		if (msg.destination == connection_table[i]->soapbox_from_application) {
			tcpc = connection_table[i];
			break;
		}
	}

	if (tcpc == NULL || tcpc->current_state == closed) {
		TCPDEBUG_print("spurious message received without associated connection. ignoring.\n");
		tcp_ignore_data(msg);
		return;
	}

	TCPDEBUG_print("stub\n");
	free_packet_memory(msg);
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
	assert(tcpc->current_state != tcpc->current_state); /* assert false */
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

	connection_table_items = 0;
	for (i=0; i< TCP_MAX_CONNECTIONS; ++i)
		connection_table[i] = alloc_new_tcp_state();


#define TCP_TEST_SERVER
#ifdef TCP_TEST_SERVER
	extern void setup_test_server();
	setup_test_server();
#endif
}
