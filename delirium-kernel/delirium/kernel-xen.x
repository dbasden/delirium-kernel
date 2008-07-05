/*
 * Delirium Kernel Linker Script
 */

SECTIONS {
/* 
 * We would normally use:
 *     . = 0x100000; 
 * but dom0 in xen isn't allowed to load in the first gig. Bah, humbug.
 * To make matters worse, Xen doesn't let us use the top gig either, so
 * at ELF load of the DeLiRiuM kernel, we have:
 *
 *     +---------------------------------------+ 4096MB
 *     | ####### Always reserved by Xen ###### |
 *     +---------------------------------------+ 3072MB
 *     | mapped for dom0 kernel load           |
 *     :                                       :
 *     +---------------------------------------+ 1024MB
 *     | Reserved by Xen during dom0 bootstrap |
 *     +---------------------------------------+ 0MB
 *
 * Just to be funny, I'm going to load in near 0, and play the domU game
 */
  . = 0x00010000;
  .delirium_kernel_start : { arch/xen/boot.o (.delirium_kernel_start) }
  .text : { *(.text) } 
  .data  :{ *(.data) }
  .rodata :{ *(.rodata) }
  .bss  :{ *(.bss) }
  .delirium_kernel_end  :{ arch/xen/boot.o (.delirium_kernel_end) } 
  .note  :{ *(.note*) }
}
