#ifndef __MACH_REGS_UART_H
#define __MACH_REGS_UART_h

#define	ULCON	0x0
#define ULCON_PARITY(x)		((x) << 3)
#define ULCON_STOPBIT(x)	((x) << 2)
#define ULCON_WORDLEN(x)	((x) << 0)

#define UCON	0x4
#define UCON_CLOCKSEL(x)	((x) << 10)
#define UCON_TXINT(x)		((x) << 9)
#define UCON_RXINT(x)		((x) << 8)
#define UCON_RXTIMEOUT(x)	((x) << 7)
#define UCON_RXERINT(x)		((x) << 6)
#define UCON_SENDBRK		(1 << 4)
#define UCON_TXMODE(x)		((x) << 2)
#define UCON_RXMODE(x)		((x) << 0)

#define UFCON	0x8
#define UFCON_TXTRIGGER(x)	((x) << 6)
#define UFCON_RXTRIGGER(x)	((x) << 4)
#define UFCON_TXFIFORST		(1 << 2)
#define UFCON_RXFIFORST		(1 << 1)
#define UFCON_FIFOEN		(1 << 0)

#define UMCON	0xC
#define UTRSTAT	0x10
#define UTRSTAT_TXEMPTY		(1 << 2)
#define UTRSTAT_TXBUFEMPTY	(1 << 1)
#define UTRSTAT_RXREADY		(1 << 0)

#define	UERSTAT	0x14
#define UERSTAT_BREAK		(1 << 3)
#define UERSTAT_FRAME		(1 << 2)
#define UERSTAT_PARITY		(1 << 1)
#define UERSTAT_OVERRUN		(1 << 0)

#define UFSTAT	0x18
#define UFSTAT_TXFULL		(1 << 14)
#define UFSTAT_TXCNT(x)		(((x) >> 8) & 0x3F)
#define UFSTAT_RXFULL		(1 << 6)
#define UFSTAT_RXCNT(x)		((x) & 0x3F)

#define UMSTAT	0x1C
#define	UTXH	0x20
#define URXH	0x24
#define UBRDIV	0x28

#endif
