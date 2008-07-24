/*
 * pci bus stuff for DeLiRiuM
 * (Really just some simple stuff to get devices up and running.)
 *
 * Copyright (c)2008 David Basden <davidb-delirium@rcpt.to>
 */

#include <delirium.h>
#include "delibrium/delibrium.h"

#ifdef ARCH_i386
#include <i386/io.h>
#endif

#include "pci.h"

inline u_int32_t read_PCI_reg(u_int32_t address, u_int32_t reg) {
	outl(PCI_CONFIG_ADDRESS, (address & 0xffffff00) | ((reg & 0xfc)));
	return inl(PCI_CONFIG_DATA);
}
inline void write_PCI_reg(u_int32_t address, u_int32_t reg, u_int32_t val) {
	outl(PCI_CONFIG_ADDRESS, (address & 0xffffff00) | ((reg & 0xfc)));
	outl(PCI_CONFIG_DATA, val);
}

inline u_int32_t read_PCI(u_int32_t bus, u_int32_t device, u_int32_t func, u_int32_t reg) {
	outl(PCI_CONFIG_ADDRESS, BUILD_PCI_ADDRESS(bus, device,func,reg));
	return inl(PCI_CONFIG_DATA);
}
inline void write_PCI(u_int32_t bus, u_int32_t device, u_int32_t func, u_int32_t reg, u_int32_t val) {
	outl(PCI_CONFIG_ADDRESS, BUILD_PCI_ADDRESS(bus, device,func,reg));
	outl(PCI_CONFIG_DATA, val);
}

/* get the PCI address for the first device matching vendor_id and device_id
 * returns 0 if not found
 */
u_int32_t find_pci_by_id(u_int32_t vendor_id, u_int32_t device_id) {
	int bus, device, func;
	for (bus=0; bus<256; ++bus) {
		if (! PCI_EXISTS(bus,0,0)) continue;

		for (device=0; device<32; ++device) {
			if (! PCI_EXISTS(bus,device,0)) continue;

			for (func=0; func< 8; ++func) {
				u_int32_t reg = read_PCI(bus,device,func,0);
				if (reg == ((vendor_id & 0xffff) | (device_id << 16)))
					return BUILD_PCI_ADDRESS(bus,device,func,0);
			}
		}
	}
	return 0;
}

void dump_pci_config() {
	int bus, device, func;
	for (bus=0; bus<256; ++bus) {
		if (! PCI_EXISTS(bus,0,0)) 
			continue;
		printf("\nBus %d\n", bus);

		for (device=0; device<32; ++device) {
			if (! PCI_EXISTS(bus,device,0)) 
				continue;
			printf("|\n|-- Device %d\n", device);
			for (func=0; func< 8; ++func) {
				if (! PCI_EXISTS(bus,device,func)) 
					continue;
				u_int32_t reg = read_PCI(bus,device,func,0);
				printf("\t|-- %d:%d.%d\t%4x:%4x\n", bus,device,func, reg & 0xffff, reg >> 16);
			}
		}
	}
}
