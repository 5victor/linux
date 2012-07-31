
#include <linux/types.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/init.h>

static void mini2440_map_io(void)
{
	s3c2440_map_io(NULL, 0);
}

MACHINE_START(MINI2440, "mini2440")
	.map_io	= mini2440_map_io,
	.init_irq = s3c2440_init_irq,
	.init_early = s3c2440_init_early,
	.timer = &s3c2440_timer,
MACHINE_END
