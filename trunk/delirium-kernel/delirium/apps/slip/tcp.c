
#include <assert.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>

#include "delibrium/delibrium.h"
#include "delibrium/pool.h"
#include "delibrium/timer.h"

#include "tcp.h"

#define TCP_TEST_SERVER
#define TCP_TEST_SERVER_PORT	7

#define TCPDEBUG
#ifdef TCPDEBUG
#define TCPDEBUG_print(_s)	{print("[tcp] "); print( (char *) __func__ ); print(": " _s);}
#else
#define TCPDEBUG_print(_s)
#endif

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


/* Get a pointer to a free connection in the connection table and
 * increment connection_table_items
 *
 * returns NULL iff connection_table_items < TCP_MAX_CONNECTIONS
 */
static inline tcp_state_t * get_free_connection() {
	int i;

	for (i=0; i<TCP_MAX_CONNECTIONS; i++) {
		if (connection_table[i]->current_state == closed) {
			connection_table[i]->current_state = allocated;
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
			0, 0, (TCP_MIN_HEADER_SIZE << 2) & 0xf0, 0, 0, 0, 0 };
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

/* Trying for pseudorandom (although not very random) initial sequences
 * I don't think this is a bad idea, but I will read some of the more
 * recent RFCs and probably do what everyone else is.
 */
static inline u_int32_t get_initial_sequence() {
	u_int64_t tsc;
	tsc = rdtsc();
	return (u_int32_t)(((tsc >> 16) ^ (tsc & 0xffffffff) ^ (tsc << 20)) & 0xffffffff);
}

/* setup a TCP connection in the LISTEN state.
 * returns NULL iff it was unable to allocate the connection
 * otherwise returns a pointer to the state of the new TCP connection
 *
 * pre: port is not already listening
 */
static tcp_state_t * create_new_listener(u_int16_t port, soapbox_id_t sb_from_application, soapbox_id_t sb_to_application) {
	tcp_state_t * tcpc;

	tcpc = get_free_connection();
	if (tcpc == NULL) return NULL; /* No free connections */
	assert(tcpc->current_state == allocated);

	tcpc->endpoints.local_addr = ipv4_local_ip;
	tcpc->endpoints.remote_addr = 0;
	tcpc->endpoints.local_port = port;
	tcpc->endpoints.remote_port = 0;
	tcpc->current_state = listen;

	return tcpc;
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
	tcphead->sequence_num = seq;
	tcphead->ack_num = ack;
	tcphead->flags = flags;
	tcphead->window = window;
	tcphead->urgent_pointer = urgent_ptr;
	checksum_outbound_tcp(tcpc, msg);

	return msg;
}

/* **********************************************************************  */

/* Received packet event handlers */


static inline void handle_listen_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	tcp_state_t * newtcpc;

	TCPDEBUG_print("Entry\n");
	/* Only state change for SYN packets */
	if (!(p.tcphead->flags & TCP_FLAG_SYN))  {
		TCPDEBUG_print("No SYN on packet\n");
		rst_unknown_packet(msg); return;
	}
	TCPDEBUG_print("SYN set on packet\n");

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
	memcpy(newtcpc, tcpc, sizeof(tcp_state_t));
	newtcpc->txwindow.head = NULL;
	newtcpc->txwindow.tail = NULL;
	newtcpc->current_state = allocated;

	/* TODO: check remote address isn't broadcast etc */
	newtcpc->endpoints.remote_addr = p.iphead->source;
	newtcpc->endpoints.remote_port = p.tcphead->source_port;
	assert(newtcpc->endpoints.local_addr == p.iphead->destination);
	assert(newtcpc->endpoints.local_port == p.tcphead->destination_port);

	tcpc = newtcpc;

	tcpc->rx.initial_seq = ntohl(p.tcphead->sequence_num);
	tcpc->rx.seq_expected = tcpc->rx.initial_seq + 1;

	/* TODO: Delay processing of any data in packet */
	free_packet_memory(msg);

	tcpc->tx.initial_seq = get_initial_sequence();
	tcpc->tx.seq_next = tcpc->tx.initial_seq + 1;

	TCPDEBUG_print("Sending SYN ACK\n");
	message_t ack_packet = build_outbound_tcp_packet(tcpc, 
				htonl(tcpc->tx.initial_seq), htonl(tcpc->rx.seq_expected), 
				TCP_FLAG_SYN | TCP_FLAG_ACK, 0, 0);
	ipv4_checksum_and_send(ack_packet);

	TCPDEBUG_print("State change: allocated -> SYN_RECEIVED\n");
	tcpc->current_state = syn_received;
}

static inline void handle_syn_rcvd_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	TCPDEBUG_print("stub\n"); discard_silently(msg); 
}

static inline void handle_syn_sent_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	TCPDEBUG_print("stub\n"); discard_silently(msg); 
}

static inline void handle_established_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	TCPDEBUG_print("stub\n"); discard_silently(msg); 
}

static inline void handle_passive_close_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	TCPDEBUG_print("stub\n"); discard_silently(msg); 
}

static inline void handle_active_close_inbound(tcp_state_t * tcpc, message_t msg, tcp_packetinfo_t p) {
	TCPDEBUG_print("stub\n"); discard_silently(msg); 
}

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

	TCPDEBUG_print("received TCP packet\n");

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

#ifdef TCP_TEST_SERVER

static soapbox_id_t	tcp_test_outbound_sb;
static soapbox_id_t	tcp_test_inbound_sb;
static tcp_state_t *	tcp_test_tcp_state;

void tcp_test_server_listener(message_t msg) {
	/* Oh boy! A message from the internets! */
	if (msg.type == gestalt) {
		assert(msg.m.gestalt.length < 4096);
		((char *)msg.m.gestalt.gestalt)[msg.m.gestalt.length] = 0;
		printf("%s: state %d. recieved '%s'. Echoing.\n", __func__, tcp_test_tcp_state->current_state, (char *)msg.m.gestalt.gestalt);
		rant(tcp_test_outbound_sb, msg);
	}
}

static void tcp_test_server_thread_entry() {
	supplicate(tcp_test_inbound_sb, tcp_test_server_listener);
}

/* A test TCP server */
static void setup_test_server() {

	/* TODO: Anonomous soapboxes so that we don't have to worry about namespace collisions
	 *       when doing this sort of thing
	 */
	tcp_test_inbound_sb = get_new_soapbox("/ip/tcp/tcp_test_server/inbound");
	if (tcp_test_inbound_sb == 0) {
		printf("%s: Erk! Couldn't allocate inbound soapbox!\n", __func__);
		return;
	}
	tcp_test_outbound_sb = get_new_soapbox("/ip/tcp/tcp_test_server/outbound");
	if (tcp_test_outbound_sb == 0) {
		printf("%s: Erk! Couldn't allocate outbound soapbox!\n", __func__);
		return;
	}

	/* TODO: figure a way to do this asynchronously
	 */
	tcp_test_tcp_state = create_new_listener(htons(TCP_TEST_SERVER_PORT), tcp_test_outbound_sb, tcp_test_inbound_sb);
	if (tcp_test_tcp_state == NULL) {
		printf("%s: Erk! Couldn't open test server!\n", __func__);
		return;
	}

	/* Have the server run in another thread.
	 * Let's not avoid concurrency issues if they exist
	 */
	new_thread(tcp_test_server_thread_entry);
}

/* END ifdef TCP_TEST_SERVER */
#endif


/* ************************************************************************* */

/*
 * Init the TCP part of the stack
 */
void tcp_init() {
	int i;

	TCPDEBUG_print("Setting up TCP\n");
	printf("Testing TSC: 0x%8x 0x%8x 0x%8x\n", get_initial_sequence(), get_initial_sequence(), get_initial_sequence());
	setup_pools();

	connection_table_items = 0;
	for (i=0; i< TCP_MAX_CONNECTIONS; ++i)
		connection_table[i] = alloc_new_tcp_state();

#ifdef TCP_TEST_SERVER
	setup_test_server();
#endif
}
