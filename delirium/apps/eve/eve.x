/*
 * Delirium Linker Script
 */

SECTIONS
{
  . = 0x150000;
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
}
