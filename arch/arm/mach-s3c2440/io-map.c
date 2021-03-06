#include <linux/kernel.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <mach/map.h>

static struct map_desc s3c2440_iodesc[] __initdata = {
	{
		.virtual	= S3C2440_VA_MEM_CTRL,
		.pfn		= __phys_to_pfn(S3C2440_PA_MEM_CTRL),
                .length		= SZ_4K,
                .type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_INT_CTRL,
		.pfn		= __phys_to_pfn(S3C2440_PA_INT_CTRL),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_CLK_PW,
		.pfn		= __phys_to_pfn(S3C2440_PA_CLK_PW),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_GPIO,
		.pfn		= __phys_to_pfn(S3C2440_PA_GPIO),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_UART0,
		.pfn		= __phys_to_pfn(S3C2440_PA_UART0),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_UART1,
		.pfn		= __phys_to_pfn(S3C2440_PA_UART1),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_UART2,
		.pfn		= __phys_to_pfn(S3C2440_PA_UART2),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_TIMER,
		.pfn		= __phys_to_pfn(S3C2440_PA_TIMER),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_NAND,
		.pfn		= __phys_to_pfn(S3C2440_PA_NAND),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= S3C2440_VA_USB_DC,
		.pfn		= __phys_to_pfn(S3C2440_PA_USB_DC),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

void s3c2440_map_io(struct map_desc *desc, int size)
{
	iotable_init(s3c2440_iodesc, ARRAY_SIZE(s3c2440_iodesc));

	if (desc)
		iotable_init(desc, size);
}	
