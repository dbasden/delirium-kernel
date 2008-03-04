#ifndef __I386_PIC_H
#define __I386_PIC_H

inline void pic_mask_all_interrupts();
inline void pic_unmask_interrupt(char intr);
inline void pic_mask_interrupt(char intr);
void configure_pics();

#endif
