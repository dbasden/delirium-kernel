#include "delibrium/delibrium.h"

void dream() {
	print("Initialising serial port at 0x3f8 to 9600\n");
	init_serial(0x3f8, 9600);
	print("Writing to UART ... :");
	send_serial(0x3f8, ':');
	print("-");
	send_serial(0x3f8, '-');
	print(")");
	send_serial(0x3f8, ')');
	print("\n");
}
