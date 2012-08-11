#ifndef __MACH_UNCOMPRESS_H
#define __MACH_UNCOMPRESS_H

#include <linux/io.h>
#include <mach/map.h>
#include <mach/regs-uart.h>


#define UART0_REG(x)	(S3C2440_PA_UART0 + x)


static inline void  arch_decomp_setup(void)
{
}

static void putc(int ch)
{
	while (!(ioread32(UART0_REG(UTRSTAT)) & (1 << 2)))
		;
	iowrite32(ch, UART0_REG(UTXH));
}


static inline void flush(void)
{
}

#endif
