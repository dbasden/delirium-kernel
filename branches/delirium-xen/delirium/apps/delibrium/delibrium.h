#ifndef __DELIBRIUM_H
#define __DELIBRIUM_H

/*
 * Delibrium
 *
 * Simple start at a libc
 *
 * (c)2003-2005 David Basden
 */


#include <delirium.h>
#include <stdarg.h>

void memcpy(char *dest, char *src, unsigned int count);
void memset(char *mem, char byte, size_t count);
void memstamp(char *mem, char *stamp, unsigned int stamplen, unsigned int count);
size_t strlen(char *str);
int strcpy(char *dest, char *src);
int strcmp(char *s1, char *s2);
void revstr(char *str, size_t slen);
int utos(char *dest, u_int32_t src, size_t base, size_t pad);
int itos(char *dest, int32_t src, size_t base, int pad);
int atoi(char *);
void printf(char *format, ...);
void hexdump(char *data, size_t len);
void new_thread(void *threadEntry);

void init_serial(u_int16_t baseport, size_t speed);
inline char read_serial(u_int16_t baseport);
inline void send_serial(u_int16_t baseport, char ch);

void *get_current_page_dir();

#endif
