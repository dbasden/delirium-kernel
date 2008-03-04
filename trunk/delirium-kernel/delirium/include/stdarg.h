#ifndef _STDARG_H
#define _STDARG_H

#define	va_list			void *
#define va_start(ap, prev)	(ap = ((&(prev))+1))
#define va_arg(ap, type)	(*(type *)(((ap)+=(sizeof(type)))-(sizeof(type))))
#define va_end(ap)		(ap = NULL)

#endif
