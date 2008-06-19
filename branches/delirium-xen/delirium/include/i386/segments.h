#ifndef __SEGMENTS_H
#define __SEGMENTS_H

#include <delirium.h>
#include <i386/gdt.h>

/* Yay for prototypes! */

void config_segments();


/* Segment Descriptors */
#define _dtpack		__attribute__ ((packed))
#define _dtalign	__attribute__ ((aligned(8)))

typedef struct {
	/* First 16 bits of limit */
	u_int16_t	limit_0_15	_dtpack;

	/* First 16 bits of base */
	u_int16_t	base_0_15	_dtpack;

	/* Base bits 16-23 */
	u_int8_t	base_16_23	_dtpack;

	/* Access flags */
	u_int8_t	access		_dtpack;

	/* G, D, P flags and limit bits 16-19 */
	u_int8_t	gdp_limit_16_19	_dtpack;

	/* Base bits 24-31 */
	u_int8_t	base_24_31	_dtpack;
} segment_desc _dtalign;

typedef struct {
	u_int16_t	length	_dtpack;
	u_int32_t	*base	_dtpack;
} desc_table_ref _dtalign;


/* Flags */

#define SEG_PRESENT	0x80		/* BIT(7) */
#define	SEG_GRANULARITY	0x80		/* BIT(7) */
#define SEG_PAGED	0x80		/* BIT(7) */
#define SEG_32		0x40		/* BIT(6) */
#define	SEG_APP_SEGMENT	0x01		/* BIT(0) */


/* Descriptor priviledge level */
#define SEG_DPL_0	0x00
#define	SEG_DPL_1	0x20		/* BIT(5) */
#define SEG_DPL_2	0x40		/* BIT(6) */
#define SEG_DPL_3	0x60		/* (BIT(5) | BIT(6)) */

/* Descriptor type and access */
#define SEG_DATA	0x10		/* BIT(4)  */
#define SEG_CODE	0x18 		/* (BIT(4) | BIT(3)) */
#define SEG_DATA_WRITE	0x11		/* (BIT(4) | BIT(1)) */
#define SEG_CODE_READ	0x19		/* (BIT(4) | BIT(3) | BIT(1)) */

#endif
