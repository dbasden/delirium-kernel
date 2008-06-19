/*
 * Delerium - klib
 *
 * Simple kernel calls in place of a libc
 *
 * Note: Most of these calls do no bounds checking.
 */


#include <delirium.h>
#include <stdarg.h>
#include <cpu.h>
#include "kvga.h"
#include "klib.h"

static inline size_t kstrlen(char *str) {
	size_t len;

	for (len=0; *(str++) != '\0'; len++)
		;

	return len;
}

static inline int kstrcpy(char *dest, char *src) {
	unsigned int i;
	
	for (i=0;;i++) {
		dest[i] = src[i];
		if (dest[i] == '\0') 
			return i;
	}
}

int kstrcmp(char *s1, char *s2) {
	while (*s1 != 0 && *s2 != 0 && *s1 == *s2) {
		s1++;
		s2++;
	}
	return *s1 - *s2;
}


/*
 * reverse string in-place
 */
static inline void krevstr(char *str, size_t slen) {
	int i;

#define _swap_inplace(a,b)	{ (a) ^= (b); (b) ^= (a); (a) ^= (b); }

	for (i=0; i<(slen/2); i++)
		_swap_inplace(str[i], str[slen-i-1]);
}

/* 
 * unsigned long int to string 
 */
static inline int kutos(char *dest, u_int32_t src, size_t base, size_t pad) {
	char *destsave = dest;

	if (pad) {
		kmemset(dest, '0', pad);
		dest[pad] = '\0';
	}

	if (src == 0)
		return (pad ? pad : (kstrcpy(dest, "0")));

	while (src) {
		*(dest++) = (src % base) + (((src%base)<10) ? '0' : ('a'-10));
		src /= base;
	}

	if (! pad) 
		*dest = '\0';

	krevstr(destsave, pad ? pad : (dest - destsave));
	return pad ? pad : (dest - destsave);
}

/*
 * signed int to string
 */
static inline int kitos(char *dest, int32_t src, size_t base, int pad) {

	if (src < 0) {
		*(dest++) = '-';
		src = 0 - src;
		return kutos(dest, (u_int32_t) src, base, pad) + 1;
	}
	return kutos(dest, (u_int32_t) src, base, pad);
}


/*
 * kprintf
 *
 * Remember that there is implicit promotion with float, char, shorts etc
 * to double or int.
 *
 */
void kprintf(char *format, ...) {
	va_list argp;
	int pad;
	size_t d;
	unsigned int u;
	unsigned char c;
	char deststring[1024];
	char *dest;
	char *s;

	dest = deststring;

	va_start(argp, format);
	for(;*format != '\0';format++) {

		// Format identifier
		// 
		if (*format == '%') {
			format++ ; // Skip the %
			for (pad=0; 
			     ((*format)>='0') && ((*format)<='9'); 
			     format++) {
				pad += (*format)-'0';
			}
			switch (*format) {
				case 'l':	/* Long: Assume sizeof(n)=4 */
				format++;
			 case 's':	/* String */
				s = va_arg(argp, char *);
				dest += kstrcpy(dest, s);
				break;
			 case 'u':	/* Unsigned int */
				u = va_arg(argp, int);
				dest += kutos(dest, u,10,pad);
				break;
			 case 'd':	/* Signed int */
				d = va_arg(argp, int);
				dest += kitos(dest, d,10,pad);
				break;
			 case 'x':	/* Unsigned int as hex */
				u = va_arg(argp, int);
				dest += kutos(dest, u,16,pad);
				break;
			 case 'b':	/* Unsigned int as binary */
				u = va_arg(argp, int);
				dest += kutos(dest, u,2,pad);
				break;
			 case 'c':	/* Unsigned char literal */
				c = va_arg(argp, int) & 0xff;
				*(dest++) = c;
				break;
			 case '%':	/* literal % */
				*(dest++) = '%';
				break;
			 default:
				kstrcpy(dest, "(err)");
				dest += 5;
			}
		} else {
			*dest = *format;
			dest++;
		}

	}
	va_end(argp);
	*dest = '\0';
	vgaprint(deststring);
}

/*
 * simple kprint
 */
void inline kprint(char *str) {
	vgaprint(str);
}

void kmemcpy(char *dest, char *src, unsigned int count) {
	unsigned int i;

	for (i=0; i<count; i++)
		dest[i] = src[i];
}



void kmemset(char *mem, char byte, size_t count) {
	while (count--) 
		*(mem++) = byte;
}

void kmemstamp(char *mem, char *stamp, unsigned int stamplen, unsigned int count) {
	unsigned int i;

	for (i=0; i<count; i++)
		kmemcpy(mem+(i*stamplen),stamp,stamplen);
}


