#ifndef __SLIP_H
#define __SLIP_H

#define SLIP_END	192
#define SLIP_ESC	219
#define SLIP_ESC_END	220
#define SLIP_ESC_ESC	221

void slip_init(u_int16_t base_port, u_int8_t hw_int, size_t speed);

#endif
