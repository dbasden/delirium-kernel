#include "delibrium/delibrium.h"
#include "delibrium/serial.h"

void dream() {
	print("Initialising serial port at 0x3f8 to 9600\n");
	init_serial(0x3f8, 9600);
	print("Writing to UART ... :");
	blocking_send_serial(0x3f8, ':');
	print("-");
	blocking_send_serial(0x3f8, '-');
	print(")");
	blocking_send_serial(0x3f8, ')');
	print("\n");
}
