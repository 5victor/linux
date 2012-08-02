#ifndef	__MACH_REGS_INT_H
#define	__MACH_REGS_INT_H

#include <mach/map.h>

#define	SRCPND		0x0
#define	INTMOD		0x4
#define	INTMSK		0x8
#define	PRIORITY	0xC
#define	INTPND		0x10
#define	INTOFFSET	0x14
#define	SUBSRCPND	0x18
#define	INTSUBMSK	0x1c

#define REG_INT(x)	(S3C2440_VA_INT_CTRL + (x))

#endif
