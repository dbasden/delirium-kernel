#ifndef __LUCID_H
#define __LUCID_H

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define Assert assert
#define kprintf		printf
#define kprint(_str)	printf(_str)
#define	kmemcpy		memcpy
#define kmemset		memset
#define kstrcmp		strcmp
#define add_timer	add_ktimer

#endif
