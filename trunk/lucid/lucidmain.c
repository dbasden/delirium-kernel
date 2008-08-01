#include <stdio.h>

#include "multitask.h"
#include "rant.h"

extern void dream();
extern int do_serial_select();
extern int do_tun_select();


/* Main hook */
int main() {
	fprintf(stderr, "%s: calling setup_soapbox()\n",__func__);
	extern void setup_soapbox();
	setup_soapbox();
	extern void ktimer_init(int);
	ktimer_init(1000);
	fprintf(stderr, "%s: calling dream()\n",__func__);
	dream();
	fprintf(stderr, "%s: finished dream()\n",__func__);
	fprintf(stderr, "%s: thread refcount is %d\n",__func__,get_thread_info().refcount);
	while (get_thread_info().refcount) {
	     	do_serial_select();
		do_tun_select();
		believe();
	}
	fprintf(stderr, "%s: thread refcount is %d\n",__func__,get_thread_info().refcount);
	return 0;
}
