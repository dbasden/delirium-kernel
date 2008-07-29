/*
 * Delibrium
 *
 * Simple start at a libc
 */


#include <delirium.h>
#include <stdarg.h>

void memcpy(void *dest, void *src, size_t count) {
	while (count--)
		*((u_int8_t *)dest++) = *((u_int8_t *)src++);
}


void memset(char *mem, char byte, size_t count) {
	while (count--) 
		*(mem++) = byte;
}

void memstamp(char *mem, char *stamp, unsigned int stamplen, unsigned int count) {
	unsigned int i;

	for (i=0; i<count; i++)
		memcpy(mem+(i*stamplen),stamp,stamplen);
}



size_t strlen(char *str) {
	size_t len;

	for (len=0; *(str++) != '\0'; len++)
		;

	return len;
}

int strcpy(char *dest, char *src) {
	unsigned int i;
	
	for (i=0;;i++) {
		dest[i] = src[i];
		if (dest[i] == '\0') 
			return i;
	}
}

int strcmp(char *s1, char *s2) {
	while (*s1 != 0 && *s2 != 0 && *s1 == *s2) {
		s1++;
		s2++;
	}
	return *s1 - *s2;
}

int strncmp(char *s1, char *s2, size_t len) {
	while (*s1 != 0 && *s2 != 0 && *s1 == *s2 && len) {
		s1++;
		s2++;
		len--;
	}
	if (len == 0) { s1--; s2--; }
	return *s1 - *s2;
}


/*
 * reverse string in-place
 */
void revstr(char *str, size_t slen) {
	int i;

#define _swap_inplace(a,b)	{ (a) ^= (b); (b) ^= (a); (a) ^= (b); }

	for (i=0; i<(slen/2); i++)
		_swap_inplace(str[i], str[slen-i-1]);
}

/* 
 * unsigned long int to string 
 */
int utos(char *dest, u_int32_t src, size_t base, size_t pad) {
	char *destsave = dest;

	if (pad) {
		memset(dest, '0', pad);
		dest[pad] = '\0';
	}

	if (src == 0)
		return (pad ? pad : (strcpy(dest, "0")));

	while (src) {
		*(dest++) = (src % base) + (((src%base)<10) ? '0' : ('a'-10));
		src /= base;
	}

	if (! pad) 
		*dest = '\0';

	revstr(destsave, pad ? pad : (dest - destsave));
	return pad ? pad : (dest - destsave);
}


/* signed string to int */
int atoi(char *str) {
        int out = 0;
        int sign = 1;

        while (*str != '\0' && (*str == ' ' || *str == '\t'))
                str++;

        if (*str == '-') {
                sign = -1;
                str++;
        }
        while ((*str != '\0') && ((*str >= '0') && (*str <='9'))) {
            out *= 10;
            out += *str - '0';
            str++;
        }
       out = out * sign;

       return out;
}



/*
 * signed int to string
 */
int itos(char *dest, int32_t src, size_t base, int pad) {

	if (src < 0) {
		*(dest++) = '-';
		src = 0 - src;
		return utos(dest, (u_int32_t) src, base, pad) + 1;
	}
	return utos(dest, (u_int32_t) src, base, pad);
}


void printf(char *format, ...) {
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
				case 'l':
				++format;
			 case 's':	/* String */
				s = va_arg(argp, char *);
				dest += strcpy(dest, s);
				break;
			 case 'u':	/* Unsigned int */
				u = va_arg(argp, int);
				dest += utos(dest, u,10,pad);
				break;
			 case 'd':	/* Signed int */
				d = va_arg(argp, int);
				dest += itos(dest, d,10,pad);
				break;
			 case 'x':	/* Unsigned int as hex */
				u = va_arg(argp, int);
				dest += utos(dest, u,16,pad);
				break;
			 case 'b':	/* Unsigned int as binary */
				u = va_arg(argp, int);
				dest += utos(dest, u,2,pad);
				break;
			 case 'c':	/* Unsigned char literal */
				c = va_arg(argp, int) & 0xff;
				*(dest++) = c;
				break;
			 case '%':	/* literal % */
				*(dest++) = '%';
				break;
			 default:
				strcpy(dest, "(err)");
				dest += 5;
			}
		} else {
			*dest = *format;
			dest++;
		}

	}
	va_end(argp);
	*dest = '\0';
	print(deststring);
}

void hexdump(char *data, size_t len) {
	unsigned int i;

	for (i=0; i<len; i++)
		printf("0x%02x ", data[i]);

	printf("\n");
}


