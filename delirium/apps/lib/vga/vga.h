#ifndef __VGA_H
#define __VGA_H

#define VGA_MISC_OUT_REG	0x03c2

#define VGA_SEQ_INDEX_REG	0x03c4
#define VGA_SEQ_DATA_REG	0x03c5

#define VGA_PEL_W_INDEX_REG	0x03c8
#define VGA_PEL_DATA_REG	0x03c9

#define VGA_CRTC_INDEX_REG	0x03d4
#define VGA_CRTC_DATA_REG	0x03d5

#define VGA_CRTC_STATUS_REG	0x03da

#define VGA_GFX_INDEX_REG	0x03ce
#define VGA_GFX_DATA_REG	0x03cf

#define VGA_PAL_REG		0x03c0
#define VGA_PAL_READ_REG	0x03c1

/* CRT Controller Registers */
#define VGA_CRTC_MODE_CTL	0x17

/* Sequence Controller Registers */
#define VGA_SEQ_CLOCK_MODE	0x01
#define VGA_SEQ_MAP_MASK_REG	0x02
#define VGA_SEQ_MEM_MODE_REG	0x04

/* Palette Controller Registers */
#define VGA_PAL_UNBLANK	(1<<5)
#define VGA_PAL_MODE	0x10 // Mode control register

/* Graphics Controller Registers */
#define VGA_GFX_MODE	0x05 // Mode control register
#define VGA_GFX_MGR	0x06 // Misc graphics register

/* Write to a windowed register, and restore the original index afterwards */
#define WRITE_VGA_SUB(_index, _data, _offset, _value) ({\
	char _saveidx; \
	_saveidx = inb(_index);\
	outb(_index, _offset);\
	outb(_data, _value);\
	outb(_index, _saveidx);	} )
#define READ_VGA_SUB(_index, _data, _offset) ( {\
	char _saveidx; \
	char _out; \
	_saveidx = inb(_index);\
	outb(_index, _offset);\
	_out = inb(_data);\
	outb(_index, _saveidx);\
	_out; } )

#define WRITE_VGA_PAL(_index, _data) ({ inb(VGA_CRTC_STATUS_REG); outb(VGA_PAL_REG, _index); outb(VGA_PAL_REG, _data); })
#define READ_VGA_PAL(_index) ({ char _out; inb(VGA_CRTC_STATUS_REG); outb(VGA_PAL_REG, _index); _out = inb(VGA_PAL_READ_REG); _out;})

#define WRITE_VGA_GFX(_idx, _byte)	\
		WRITE_VGA_SUB(VGA_GFX_INDEX_REG,VGA_GFX_DATA_REG,_idx,_byte)
#define READ_VGA_GFX(_idx)	\
		READ_VGA_SUB(VGA_GFX_INDEX_REG,VGA_GFX_DATA_REG,_idx)
#define WRITE_VGA_SEQ(_idx, _byte)	\
		WRITE_VGA_SUB(VGA_SEQ_INDEX_REG,VGA_SEQ_DATA_REG,_idx,_byte)
#define READ_VGA_SEQ(_idx)	\
		READ_VGA_SUB(VGA_SEQ_INDEX_REG,VGA_SEQ_DATA_REG,_idx)
#define WRITE_VGA_CRTC(_idx, _byte)	\
		WRITE_VGA_SUB(VGA_CRTC_INDEX_REG,VGA_CRTC_DATA_REG,_idx,_byte)
#define READ_VGA_CRTC(_idx)	\
		READ_VGA_SUB(VGA_CRTC_INDEX_REG,VGA_CRTC_DATA_REG,_idx)



#define SET_PLANE(_n)	WRITE_VGA_SEQ(VGA_SEQ_MAP_MASK_REG, _n)
#define SET_VGA_PAL(_i,_r,_g,_b) ({outb(VGA_PEL_W_INDEX_REG, (_i));\
		outb(VGA_PEL_DATA_REG, (_r));\
		outb(VGA_PEL_DATA_REG, (_g));\
		outb(VGA_PEL_DATA_REG, (_b));\
		})


typedef u_int8_t vreg;
struct vga_regs {
	vreg	misc_output_reg;
	
	/* Sequencer Registers */
	vreg	sequencer[5];
	vreg	crt_controller[25];
	vreg	graphics_controller[9];
	vreg	attribute_controller[21];
};

void vga_saveregs(struct vga_regs *saved);
void vga_loadregs(struct vga_regs *r);
void vga_openwindow();
void vga_demo();
void vga_graphicsfun();
					 

#endif
