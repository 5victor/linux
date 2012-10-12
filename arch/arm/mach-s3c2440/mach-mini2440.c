#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/init.h>

static struct platform_device s3c2440_udc_device = {
	.name		= "s3c2440-udc",
	.id		= -1,
};

static struct platform_device *mini2440_pdevs[] = {
	&s3c2440_udc_device,
};

static void mini2440_init_machine(void)
{
	platform_add_devices(mini2440_pdevs, ARRAY_SIZE(mini2440_pdevs));
}

static void mini2440_map_io(void)
{
	s3c2440_map_io(NULL, 0);
}

MACHINE_START(MINI2440, "mini2440")
	.map_io	= mini2440_map_io,
	.init_irq = s3c2440_init_irq,
	.init_early = s3c2440_init_early,
	.timer = &s3c2440_timer,
	.init_machine = mini2440_init_machine,
MACHINE_END
