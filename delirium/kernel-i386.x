/*
 * Delirium Kernel Linker Script
 */

/*
*PHDRS {
*   data PT_LOAD ; 
*}
*/
SECTIONS {
  . = 0x100000;
  .text : { 
  	*(.delirium_kernel_start) 
  	*(.text) 
   }
  .data : { *(.data) }
  .bss : { *(.bss) }
  .end : {
  	arch/i386/boot.o (.delirium_kernel_end) 
   }
}
