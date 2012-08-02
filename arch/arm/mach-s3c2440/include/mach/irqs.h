
#ifndef __MACH_IRQS_H
#define __MACH_IRQS_H

#define	IRQ_EINT0	0
#define	IRQ_EINT1	1
#define	IRQ_EINT2	2
#define	IRQ_EINT3	3
#define	IRQ_EINT4_7	4
#define	IRQ_EINT8_23	5
#define	IRQ_CAM	6
#define	IRQ_NBATT_FLT	7
#define	IRQ_TICK	8
#define	IRQ_WDT_AC97	9
#define	IRQ_TIMER0	10
#define	IRQ_TIMER1	11
#define	IRQ_TIMER2	12
#define	IRQ_TIMER3	13
#define	IRQ_TIMER4	14
#define	IRQ_UART2	15
#define	IRQ_LCD		16
#define	IRQ_DMA0	17
#define	IRQ_DMA1	18
#define	IRQ_DMA2	19
#define	IRQ_DMA3	20
#define	IRQ_SDI		21
#define	IRQ_SPI0	22
#define	IRQ_UART1	23
#define	IRQ_NFCON	24
#define	IRQ_USBD	25
#define	IRQ_USBH	26
#define	IRQ_IIC		27
#define	IRQ_UART0	28
#define	IRQ_SPI1	29
#define	IRQ_RTC		30
#define	IRQ_ADC		31

/* SUB INT */

#define IRQ_AC97	46
#define IRQ_WDT	45
#define IRQ_CAM_P	44
#define IRQ_CAM_C	43
#define IRQ_ADC_S	42
#define IRQ_TC	41
#define IRQ_ERR2	40
#define IRQ_TXD2	39
#define IRQ_RXD2	38
#define IRQ_ERR1	37
#define IRQ_TXD1	36
#define IRQ_RXD1	35
#define IRQ_ERR0	34
#define IRQ_TXD0	33
#define IRQ_RXD0	32

/* EINT */
#define IRQ_EINT4	47
#define IRQ_EINT5	48
#define IRQ_EINT6	49
#define IRQ_EINT7	50
#define IRQ_EINT8	51
#define IRQ_EINT9	52
#define IRQ_EINT10	53
#define IRQ_EINT11	54
#define IRQ_EINT12	55
#define IRQ_EINT13	56
#define IRQ_EINT14	57
#define IRQ_EINT15	58
#define IRQ_EINT16	59
#define IRQ_EINT17	60
#define IRQ_EINT18	61
#define IRQ_EINT19	62
#define IRQ_EINT20	63
#define IRQ_EINT21	64
#define IRQ_EINT22	65
#define IRQ_EINT23	66

#define NR_IRQS	67

#endif
