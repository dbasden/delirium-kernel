#ifndef __PCI_H
#define __PCI_H
/*
 * pci bus stuff for DeLiRiuM
 * (Really just some simple stuff to get devices up and running.)
 *
 * Copyright (c)2008 David Basden <davidb-delirium@rcpt.to>
 */

#include <delirium.h>

#define PCI_CONFIG_ADDRESS	0x0cf8
#define PCI_CONFIG_DATA		0x0cfc
#define PCI_ENABLE_BIT		(1 << 31)

#define PCI_REG_STATCMD	0x04
#define PCI_REG_BAR0	0x10
#define PCI_REG_INT	0x3C

#define BUILD_PCI_ADDRESS(_bus,_device,_func,_reg) ((((_bus) & 0xff) << 16) | (((_device) & 0x1f) << 11) | (((_func) & 0x7) << 8) | ((_reg) & 0xfc)  | PCI_ENABLE_BIT)

#define PCI_EXISTS(_bus, _dev, _func) (read_PCI(_bus,_dev,_func,0) != 0xffffffff)

inline u_int32_t read_PCI_reg(u_int32_t address, u_int32_t reg);
inline u_int32_t read_PCI(u_int32_t bus, u_int32_t device, u_int32_t func, u_int32_t reg);

inline void write_PCI_reg(u_int32_t address, u_int32_t reg, u_int32_t val);
inline void write_PCI(u_int32_t bus, u_int32_t device, u_int32_t func, u_int32_t reg, u_int32_t val);

/* get the PCI address for the first device matching vendor_id and device_id
 * returns 0 if not found
 */
u_int32_t find_pci_by_id(u_int32_t vendor_id, u_int32_t device_id);

void dump_pci_config();
#endif
