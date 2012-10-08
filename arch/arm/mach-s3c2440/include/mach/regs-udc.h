#ifndef MACH_REGS_UDC_H
#define MACH_REGS_UDC_H

#include <mach/map.h>

#define FUNC_ADDR_REG	0x140
#define PWR_REG		0x144
#define EP_INT_REG	0x148
#define USB_INT_REG	0x158
#define EP_INT_EN_REG	0x15C
#define USB_INT_EN_REG	0x16C
#define FRAME_NUM1_REG	0x170
#define FRAME_NUM2_REG	0x174
#define INDEX_REG	0x178
#define MAXP_REG	0x180
#define EP0_CSR		0x184
#define IN_CSR1_REG	0x184
#define IN_CSR2_REG	0x188
#define OUT_CSR1_REG	0x190
#define OUT_CSR2_REG	0x194
#define OUT_FIFO_CNT1_REG	0x198
#define OUT_FIFO_CNT2_REG	0x19C
#define EPn_FIFO_REG(x)	(0x1C0 + 4*(x))

#define REG_UDC(x)	(S3C2440_VA_USB_DC + (x))

#endif
