#include <delirium.h>
#include "delibrium/delibrium.h"
#include "delibrium/pool.h"

#include "tcp.h"

#define TCPDEBUG
#ifdef TCPDEBUG
#define TCPDEBUG_print(_s)	{print((char *) __FUNCTION__); print(_s);}
#else
#define TCPDEBUG_print(_s)
#endif

#define TCP_MAX_CONNECTIONS	16

static tcp_state_t * connection_table[TCP_MAX_CONNECTIONS];

#define _TCP_STATE_EXPONENT	7

static tcp_state_t * alloc_new_tcp_state() {
	tcp_state_t * tcp;
	tcp = pool_alloc(_TCP_STATE_EXPONENT);
	tcp->txwindow.head = NULL;
	tcp->txwindow.tail = NULL;
	tcp->current_state = closed;

	return tcp;
}
static void free_tcp_state(tcp_state_t *tcp) {
	pool_free(_TCP_STATE_EXPONENT, tcp);
}

void tcp_init() {
	int i;

	TCPDEBUG_print("Setting up TCP\n");
	setup_pools();

	for (i=0; i< TCP_MAX_CONNECTIONS; ++i)
		connection_table[i] = alloc_new_tcp_state();
}
