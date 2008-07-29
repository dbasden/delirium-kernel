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

#ifndef ARCH_lucid
void memcpy(void *dest, void *src, size_t count);
void memset(char *mem, char byte, size_t count);
void memstamp(char *mem, char *stamp, unsigned int stamplen, unsigned int count);
size_t strlen(char *str);
int strcpy(char *dest, char *src);
int strcmp(char *s1, char *s2);
void printf(char *format, ...);
int strncmp(char *s1, char *s2, size_t len);
int strncpy(char *s1, char *s2, size_t len);
#endif
void revstr(char *str, size_t slen);
int utos(char *dest, u_int32_t src, size_t base, size_t pad);
int itos(char *dest, int32_t src, size_t base, int pad);
int atoi(char *);

void hexdump(char *data, size_t len);
void new_thread(void *threadEntry);
void *get_current_page_dir();

#ifdef ARCH_lucid
#include <stdio.h>
#include <string.h>
#endif

#define	print(_arg)	printf(_arg)

#endif
