#include <delirium.h>
#include <assert.h>
#include <ipc.h>
#include <soapbox.h>
#include <rant.h>

#include "delibrium/delibrium.h"

#include "tcp.h"

#define TCP_TEST_SERVER_PORT	7
#define TCP_HTTP_TEST_SERVER_PORT	80

static soapbox_id_t	tcp_test_outbound_sb;
static soapbox_id_t	tcp_test_inbound_sb;
static tcp_state_t *	tcp_test_tcp_state;

static soapbox_id_t	tcp_http_test_outbound_sb;
static soapbox_id_t	tcp_http_test_inbound_sb;
static tcp_state_t *	tcp_http_test_tcp_state; /* don't actually need to keep this */

/* Simple echo-test server */
void tcp_test_server_listener(message_t msg) {
	/* Oh boy! A message from the internets! */
	if (msg.type == gestalt) {
		assert(msg.m.gestalt.length < 4000);
		((char *)msg.m.gestalt.gestalt)[msg.m.gestalt.length] = 0;
		rant(msg.reply_to, msg);
	}
	else if (msg.type == signal) {
		switch (msg.m.signal) {
			case listener_has_new_established:
				printf("%s: 0x%x: listener_has_new_established\n", __func__, msg.reply_to);
				break;
			case remote_reset:
				printf("%s: 0x%x: remote_reset\n", __func__, msg.reply_to);
				break;
			case connection_closed:
				printf("%s: 0x%x: connection_closed\n", __func__, msg.reply_to);
				break;
			case source_quench:
				printf("%s: 0x%x: source_quench\n", __func__, msg.reply_to);
				break;
			case zero_length_window:
				printf("%s: 0x%x: zero_length_window\n", __func__, msg.reply_to);
				break;
			default:
				printf("%s: 0x%x: unknown signal %d\n", __func__, msg.reply_to, msg.m.signal);
		}
				
	}
}

#define FOUR_OH_OH  "HTTP/1.0 400 Bad Request\r\nServer: DeLiRiuM TCP Test Server\r\nContent-Length: 18\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n400 - Bad Request\r\n";
#define FOUR_OH_FOUR "HTTP/1.0 404 Not Found\r\nServer: DeLiRiuM TCP Test Server\r\nContent-Length: 16\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n404 - Not Found\r\n";

static inline int is_get_request(message_t msg) {
	return msg.m.gestalt.length > 5 && strncmp("GET /", msg.m.gestalt.gestalt, 5) == 0;
}
static inline int is_root_get_request(message_t msg) {
	return msg.m.gestalt.length > 13 && strncmp("GET / HTTP/1.", msg.m.gestalt.gestalt, 13) == 0;
}
static inline void fill_400(message_t *msg) {
	char * foo = FOUR_OH_OH; 
	strcpy(msg->m.gestalt.gestalt, foo);
	msg->m.gestalt.length = strlen(foo);
}
static inline void fill_404(message_t *msg) {
	char * fof = FOUR_OH_FOUR; 
	strcpy(msg->m.gestalt.gestalt, fof);
	msg->m.gestalt.length = strlen(fof);
}
static inline void fill_ok(message_t *msg) {
	char *p = msg->m.gestalt.gestalt;
	int i;
	char *vga = (char *)0xb8000;
	#if 0
	char *a = "HTTP/1.0 200 OK\r\nServer: DeLiRiuM TCP Test Server\r\nContent-Type: text-html\r\nContent-Length: 142\r\nConnection: close\r\n\r\n";
	char *b = "<html>\n<head>\n  <title>DeLiRiuM TCP Test Server</title>\n</head>\n<body>\n<h1>DeLiRiuM TCP Test Server</h1>\n<p>Still Not King.</p></body></html>\n";
	#endif
	char *a = "HTTP/1.0 200 OK\r\nServer: DeLiRiuM TCP Test Server\r\nContent-Type: text-html\r\nContent-Length: 2235\r\nConnection: close\r\n\r\n" \
	"<html>\n<head>\n <title>DeLiRiuM TCP Test Server</title>\n</head>\n<body>\n" \
	"<h1>DeLiRiuM TCP Test Server</h1><p>Still Not King.</p>" \
	"<pre><code style=\"background-color: black; color: white;\">";
	char *c = "</code></pre></body></html>\n";
	size_t lena = strlen(a);
	size_t lenc = strlen(c);
	memcpy(p, a, lena);
	p += lena;
	for (i=0; i<2000; ++i, ++p) {
		if (i && i % 80 == 0) *p++ = '\n';
		*p = *vga; vga += 2;
	}
	memcpy(p, c, lenc);
	p += lenc;
	*p = 0;
	msg->m.gestalt.length = lena+lenc+(80*25)+24;
}

/* way too simple pseudo HTTP/1.0 server listener 
 * May be confused by... anything.
 * Even cows.
 */
void tcp_http_test_server_listener(message_t msg) {
	if (msg.type == gestalt) {
		assert(msg.m.gestalt.length < 4000);
		((char *)msg.m.gestalt.gestalt)[msg.m.gestalt.length] = 0;
		if (! is_get_request(msg))
			fill_400(&msg);
		else if (! is_root_get_request(msg)) 
			fill_404(&msg);
		else
			fill_ok(&msg);
		rant(msg.reply_to, msg);

		message_t sigmsg;
		sigmsg.type = signal;
		sigmsg.reply_to = msg.destination;
		sigmsg.m.signal = local_finish_sending;
		rant(msg.reply_to, sigmsg);
	}
}

static void tcp_test_server_thread_entry() {
	if (tcp_test_tcp_state != NULL)
		supplicate(tcp_test_inbound_sb, tcp_test_server_listener);
	if (tcp_http_test_tcp_state != NULL)
		supplicate(tcp_http_test_inbound_sb, tcp_http_test_server_listener);
}

/* A test TCP server */
void setup_test_server() {

	tcp_test_inbound_sb = get_new_anon_soapbox();
	tcp_test_outbound_sb = get_new_anon_soapbox();
	tcp_http_test_inbound_sb = get_new_anon_soapbox();
	tcp_http_test_outbound_sb = get_new_anon_soapbox();
	if (tcp_test_inbound_sb == 0 || tcp_test_outbound_sb == 0
	    || tcp_http_test_inbound_sb == 0 || tcp_http_test_outbound_sb == 0) {
		printf("%s: Erk! Couldn't allocate soapbox! Exiting.\n", __func__);
		return;
	}

	tcp_test_tcp_state = tcp_create_new_listener(htons(TCP_TEST_SERVER_PORT), tcp_test_outbound_sb, tcp_test_inbound_sb);
	if (tcp_test_tcp_state == NULL) 
		printf("%s: Erk! Couldn't create test server!\n", __func__);

	tcp_http_test_tcp_state = tcp_create_new_listener(htons(TCP_HTTP_TEST_SERVER_PORT), tcp_http_test_outbound_sb, tcp_http_test_inbound_sb);
	if (tcp_http_test_tcp_state == NULL) 
		printf("%s: Erk! Couldn't create test HTTP server!\n", __func__);

	/* Have the server run in another thread.
	 * Let's not avoid concurrency issues if they exist
	 */
	new_thread(tcp_test_server_thread_entry);
}
