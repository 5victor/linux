#ifndef __MACH_TIMER_H
#define __MACH_TIMER_H

#define	TCFG0	0x0
#define	TCFG1	0x4
#define	TCON	0x8
#define	TCNTB0	0xC
#define	TCMPB0	0x10
#define	TCNTO0	0x14
#define	TCNTB1	0x18
#define	TCMPB1	0x1C
#define	TCNTO1	0x20
#define	TCNTB2	0x24
#define	TCMPB2	0x28
#define	TCNTO2	0x28
#define	TCMPB3	0x34
#define	TCNTO3	0x38
#define	TCNTB4	0x3C
#define	TCNTO4	0x40

#include <mach/map.h>

#define REG_TIMER(x)    (S3C2440_VA_TIMER + (x))

#endif
