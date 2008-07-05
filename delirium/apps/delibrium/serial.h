#ifndef __SERIAL_H
#define __SERIAL_H

#include <i386/io.h>

#define _SERIAL_is_Data_Waiting( _baseport ) ({ unsigned char __s_tmp_c; (__s_tmp_c = inb( (_baseport)+5 ) & 1); })
#define _SERIAL_Readb( _baseport ) ({ unsigned char __s_tmp_c; (__s_tmp_c = inb( (_baseport))); })
#define _SERIAL_is_Spare_Outbuf( _baseport ) ({ unsigned char __s_tmp_c; (__s_tmp_c = inb( (_baseport)+5 ) & 0x20) != 0; })
#define _SERIAL_Sendb( _baseport , _byte ) (outb( (_baseport), (_byte)))

/*
 * Setup a 16[45]50 UART
 *
 * This will also enable interrupts on the serial port; Register an
 * interrupt handler as well.
 */
void init_serial(u_int16_t baseport, size_t speed);

/* consume and discard buffer */
inline void consume_serial_buffer(u_int16_t);

/* Don't use this. 
 *
 * Really. 
 * 
 *  It's ugly, and it spins, and it's doomy
 */
inline char blocking_read_serial(u_int16_t baseport);

/* Spin until there is free outbound buffer to send a char.
 * Avoid using this if possible
 */
inline void blocking_send_serial(u_int16_t baseport, char ch);

#endif
