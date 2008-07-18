#include <assert.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>

#include "delibrium/delibrium.h"

#include "tcp.h"

#ifndef TCP_TEST_SERVER_PORT
#define TCP_TEST_SERVER_PORT	7
#endif

static soapbox_id_t	tcp_test_outbound_sb;
static soapbox_id_t	tcp_test_inbound_sb;
static tcp_state_t *	tcp_test_tcp_state;

void tcp_test_server_listener(message_t msg) {
	/* Oh boy! A message from the internets! */
	if (msg.type == gestalt) {
		assert(msg.m.gestalt.length < 4000);
		((char *)msg.m.gestalt.gestalt)[msg.m.gestalt.length] = 0;
		#if 0
		printf("%s: state %d. recieved '%s'. Echoing.\n", __func__, tcp_test_tcp_state->current_state, (char *)msg.m.gestalt.gestalt);
		#endif
		rant(tcp_test_outbound_sb, msg);
	}
}

static void tcp_test_server_thread_entry() {
	supplicate(tcp_test_inbound_sb, tcp_test_server_listener);
}

/* A test TCP server */
void setup_test_server() {

	tcp_test_inbound_sb = get_new_anon_soapbox();
	tcp_test_outbound_sb = get_new_anon_soapbox();
	if (tcp_test_inbound_sb == 0 || tcp_test_outbound_sb == 0) {
		printf("%s: Erk! Couldn't allocate soapbox! Exiting.\n", __func__);
		return;
	}

	/* TODO: figure a way to do this asynchronously
	 */
	tcp_test_tcp_state = tcp_create_new_listener(htons(TCP_TEST_SERVER_PORT), tcp_test_outbound_sb, tcp_test_inbound_sb);
	if (tcp_test_tcp_state == NULL) {
		printf("%s: Erk! Couldn't create test server!\n", __func__);
		return;
	}

	/* Have the server run in another thread.
	 * Let's not avoid concurrency issues if they exist
	 */
	new_thread(tcp_test_server_thread_entry);
}
