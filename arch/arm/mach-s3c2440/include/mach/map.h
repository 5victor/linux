#ifndef __MACH_MAP_H
#define __MACH_MAP_H

/* PA defines */

#define S3C2440_PA_MEM_CTRL	0x48000000

#define S3C2440_PA_USB_HC	0x49000000

#define S3C2440_PA_INT_CTRL	0x4A000000

#define S3C2440_PA_DMA		0x4B000000

#define S3C2440_PA_CLK_PW	0x4C000000

#define S3C2440_PA_LCD		0x4D000000

#define S3C2440_PA_NAND		0x4E000000

#define S3C2440_PA_CAM_IF	0x4F000000

#define S3C2440_PA_UART		0x50000000

#define S3C2440_PA_TIMER	0x51000000

#define S3C2440_PA_USB_DC	0x52000000

#define S3C2440_PA_WATCHDOG	0x53000000

#define S3C2440_PA_IIC		0x54000000

#define S3C2440_PA_IIS		0x55000000

#define S3C2440_PA_GPIO		0x56000000

#define S3C2440_PA_RTC		0x57000043

#define S3C2440_PA_ADC		0x58000000

#define S3C2440_PA_SPI		0x59000000

#define S3C2440_PA_SD		0x5A000000

#define S3C2440_PA_AC97		0x5B000000


#define S3C2440_PA_UARTx(n)	(S3C2440_PA_UART + 0x4000 * n)

#define S3C2440_PA_UART0	S3C2440_PA_UARTx(0)

#define S3C2440_PA_UART1	S3C2440_PA_UARTx(1)

#define S3C2440_PA_UART2	S3C2440_PA_UARTx(2)

/* VA defines */
#define S3C2440_VA_BASE		0xF8000000
#define MAP_VA(addr,n)		(S3C2440_VA_BASE+0x1000*(n) + \
					((addr)&0xFFF))

#define S3C2440_VA_MEM_CTRL	MAP_VA(S3C2440_PA_MEM_CTRL, 0)

#define S3C2440_VA_USB_HC	MAP_VA(S3C2440_PA_USB_HC, 1)

#define S3C2440_VA_INT_CTRL	MAP_VA(S3C2440_PA_INT_CTRL, 2)

#define S3C2440_VA_DMA		MAP_VA(S3C2440_PA_DMA, 3)

#define S3C2440_VA_CLK_PW	MAP_VA(S3C2440_PA_CLK_PW, 4)

#define S3C2440_VA_LCD		MAP_VA(S3C2440_PA_LCD, 5)

#define S3C2440_VA_NAND		MAP_VA(S3C2440_PA_NAND, 6)

#define S3C2440_VA_CAM_IF	MAP_VA(S3C2440_PA_CAM_IF, 7)

#define S3C2440_VA_TIMER	MAP_VA(S3C2440_PA_TIMER, 8)

#define S3C2440_VA_USB_DC	MAP_VA(S3C2440_PA_USB_DC, 9)

#define S3C2440_VA_WATCHDOG	MAP_VA(S3C2440_PA_WATCHDOC, 10)

#define S3C2440_VA_IIC		MAP_VA(S3C2440_PA_IIC, 11)

#define S3C2440_VA_IIS		MAP_VA(S3C2440_PA_IIS, 12)

#define S3C2440_VA_GPIO		MAP_VA(S3C2440_PA_GPIO, 13)

#define S3C2440_VA_RTC		MAP_VA(S3C2440_PA_RTC, 14)

#define S3C2440_VA_ADC		MAP_VA(S3C2440_PA_ADC, 15)

#define S3C2440_VA_SPI		MAP_VA(S3C2440_PA_SPI, 15)

#define S3C2440_VA_SD		MAP_VA(S3C2440_PA_SD, 16)

#define S3C2440_VA_AC97		MAP_VA(S3C2440_PA_AC97, 17)

#define MAP_VA_UART(n)		(0xF9000000+0x1000000*(n)+ \
					(S3C2440_PA_UARTx(n) & 0xFFF))

#define S3C2440_VA_UART0	MAP_VA_UART(0)
#define S3C2440_VA_UART1	MAP_VA_UART(1)
#define S3C2440_VA_UART2	MAP_VA_UART(2)

#endif
