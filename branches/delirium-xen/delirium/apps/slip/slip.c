#include "delibrium/delibrium.h"
#include "slip.h"

void dream() {
	print("Starting SLIP driver on port 0x3f8 at 9600\n");
	slip_init(0x3f8, 9600);
}
