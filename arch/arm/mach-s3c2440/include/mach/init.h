#ifndef __INIT_H
#define __INIT_H

#include <asm/mach/map.h>
#include <asm/mach/time.h>

void s3c2440_map_io(struct map_desc *desc, int size);
void s3c2440_init_irq(void);
void s3c2440_clk_init(void);

void s3c2440_init_early(void)
{
	s3c2440_clk_init();
}

extern struct sys_timer s3c2440_timer;

#endif
