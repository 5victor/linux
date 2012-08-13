#ifndef __MACH_REGS_CLK_H
#define __MACH_REGS_CLK_H

#include <mach/map.h>

#define	LOCKTIME	0x0
#define	MPLLCON		0x4
#define	UPLLCON		0x8
#define	CLKCON		0xC
#define	CLKSLOW		0x10
#define	CLKDIVN		0x14
#define	CAMDIVN		0x18

#define REG_CLK(x)	(S3C2440_VA_CLK_PW + (x))

#endif
