#ifndef __MACH_REGS_GPIO_H
#define __MACH_REGS_GPIO_H

#define GPACON	0X0
#define GPADAT	0X4
#define GPBCON	0X10
#define GPBDAT	0X14
#define GPBUP	0X18
#define GPCCON	0X20
#define GPCDAT	0X24
#define GPCUP	0X28
#define GPDCON	0X30
#define GPDDAT	0X34
#define GPDUP	0X38
#define GPECON	0X40
#define GPEDAT	0X44
#define GPEUP	0X48
#define GPFCON	0X50
#define GPFDAT	0X54
#define GPFUP	0X58
#define GPGCON	0X60
#define GPGDAT	0X64
#define GPGUP	0X68
#define GPHCON	0X70
#define GPHDAT	0X74
#define GPHUP	0X78
#define GPJCON	0XD0
#define GPJDAT	0XD4
#define GPJUP	0XD8
#define MISCCR	0X80
#define DCLKCON	0X84
#define EXTINT0	0X88
#define EXTINT1	0X8C
#define EXTINT2	0X90
#define EINTFLT0	0X94
#define EINTFLT1	0X98
#define EINTFLT2	0X9C
#define EINTFLT3	0XA0
#define EINTMASK	0XA4
#define EINTPEND	0XA8
#define GSTATUS0	0XAC
#define GSTATUS1	0XB0
#define GSTATUS2	0XB4
#define GSTATUS3	0XB8
#define GSTATUS4	0XBC
#define MSLCON		0XCC

#include <mach/map.h>
#define REG_GPIO(x)	(S3C2440_VA_GPIO + (x))
#endif
