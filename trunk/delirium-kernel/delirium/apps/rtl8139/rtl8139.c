/* World's quickest and hackiest RTL8139 driver 
 * Written for Delirium in a burst of haste
 *
 * Copyright (c) 2008  David Basden <davidb-delirium@rcpt.to>
 */
#include <delirium.h>
#include <i386/interrupts.h>
#include <i386/io.h>
#include "delibrium/delibrium.h"
#include "pci.h"

#define R8139_RBSTART		0x30
#define R8139_COMMAND		0x37
#define R8139_IMR		0x3c
#define R8139_ISR		0x3e
#define R8139_RCR		0x44
#define R8139_CONFIG_1		0x52

#define R8139_out_b(_off,_b)	outb(baseio + (_off), (_b))
#define R8139_out_w(_off,_s)	outw(baseio + (_off), (_s))
#define R8139_out_l(_off,_l)	outl(baseio + (_off), (_l))
#define R8139_in_b(_off)	inb(baseio + (_off));
#define R8139_in_w(_off)	inw(baseio + (_off));
#define R8139_in_l(_off)	inl(baseio + (_off));


static int baseio;
static int hwirq;

/* hackish buffer space */
u_int8_t _rx_buffer[8192 + 16]; /* 8k buffer. 16 byte header */
u_int8_t _tx_buffer[16384 + 16]; /* 16k buffer. 16 byte */

void rtl8139_handle_irq() {
	print("rtl8139_handle_irq()\n");
	R8139_out_w(R8139_IMR, 0x0005); /* reset interrupt flags */
	int i;
	for (i=0; i<1000; ++i) {
		if (i % 32 == 0) printf("\n%4x: ",i);
		printf("%2x ", _rx_buffer[i]);
	}
}

static int init_rtl8139_pci(int irq, int base_port) {
	dump_pci_config();

	printf("%s: finding PCI config space... ", __func__);
	u_int32_t pciaddr = find_pci_by_id(0x10ec, 0x8139);
	if (!pciaddr) {
		print("Couldn't find PCI device 10ec:8139\n");
		return 0;
	}
	printf("0x%8x\n", pciaddr);

	int barnum;
	for (barnum = 0; barnum <= 5; ++barnum) {
		int bar = read_PCI_reg(pciaddr, PCI_REG_BAR0 + (barnum << 2));
		if (! (bar & 1)) 
			continue; /* Ignore memory mapped area for now */
		printf("%s: PCI BAR%d: 0x%8x\n", __func__, barnum, bar ^ 1);
		if (barnum != 0) {
			printf("%s: expected this to be BAR0. Aborting PCI setup\n");
			return 0;
		}
		break;
	}

	int irqreg = read_PCI_reg(pciaddr, PCI_REG_INT);
	printf("%s: IRQ pin %d. IRQ line %d.\n", __func__, (irqreg >> 8) & 0xff, irqreg & 0xff);
	int statcmd = read_PCI_reg(pciaddr, PCI_REG_STATCMD);
	write_PCI_reg(pciaddr, PCI_REG_INT, ((irqreg & 0xffffff00) | irq));
	irqreg = read_PCI_reg(pciaddr, PCI_REG_INT);
	printf("%s: IRQ reset. IRQ pin now %d. IRQ line now %d.\n", __func__, (irqreg >> 8) & 0xff, irqreg & 0xff);
	write_PCI_reg(pciaddr, PCI_REG_BAR0, base_port | 0x1 );
	printf("%s: IO base port reset. IO port now %x.", __func__,  read_PCI_reg(pciaddr, PCI_REG_BAR0) ^ 1);
	write_PCI_reg(pciaddr, PCI_REG_STATCMD, 0x07);
	statcmd = read_PCI_reg(pciaddr, PCI_REG_STATCMD);
	printf("%s: status/command PCI reg: status %4x, cmd %4x\n", __func__,  statcmd >> 16, statcmd & 0xffff);

	return 1;
}
static void init_rtl8139(int irq, int base_port) {
	hwirq = irq;
	baseio = base_port;

	if (!  init_rtl8139_pci(irq, base_port))
		return;

	printf("%s: setting up. irq: %d, io: 0x%x\n", __func__, hwirq, baseio);
	print("irqhook... ");
	add_c_interrupt_handler(hwirq, &rtl8139_handle_irq);
	print("powering on... ");
	R8139_out_b(R8139_CONFIG_1, 0x00);	/* Switch on power */
	yield();
	print("resetting.. ");
	R8139_out_b(R8139_COMMAND, 0x10);	/* Reset */
	u_int16_t scratch;
	scratch = R8139_in_b(R8139_COMMAND);
	while (scratch & 10) {
		print("|");
		yield(); /* Wait until reset is complete */
		scratch = R8139_in_b(R8139_COMMAND);
	}
	
	/* tell the NIC where the Rx buffer is */
	print("set rxbuf... ");
	R8139_out_l(R8139_RBSTART, (u_int32_t *) _rx_buffer); 
	print("enable TX and RX... ");
	R8139_out_b(R8139_COMMAND, 0x0c); /* TE and RE bits */
	R8139_out_l(R8139_COMMAND, 0x0c);
	print("irq flags... ");
	R8139_in_w(R8139_IMR); /* reset interrupt flags */
	R8139_out_w(R8139_IMR, 0x000f); /* reset interrupt flags */
	R8139_out_w(R8139_ISR, 0x0005); /* reset interrupt flags */
	print("listen... ");
	R8139_out_l(0x4C, 0x0); /* RX missed */
	R8139_out_b(0x50, 0xC0); /* 9346 */
	R8139_out_b(R8139_CONFIG_1, 0x60); /* FDX */
	R8139_out_b(0x50, 0x00); /* 9346 */
	R8139_out_l(R8139_RCR, 0x0f); /* Listen for everything :-) */
	R8139_out_b(R8139_COMMAND, 0x0c); /* TE and RE bits */

	print("done.\n");
}


#define QEMU_8139_IRQ	11
#define QEMU_8139_IO	0xc100

void dream() {
	init_rtl8139(QEMU_8139_IRQ, QEMU_8139_IO);
}
