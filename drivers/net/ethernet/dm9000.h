#ifndef __DM9000_H
#define __DM9000_H

#define NCR	0x00
#define NSR	0x01
#define TCR	0x02
#define TSR1	0x03
#define TSR2	0x04
#define RCR	0x05
#define RSR	0x06
#define ROCR	0x07
#define BPTR	0x08
#define FCTR	0x09
#define FCR	0x0A
#define EPCR	0x0B
#define EPAR	0x0C
#define EPDR	0x0D
#define WCR	0x0F
#define PAR	0x10
#define MAR	0x16
#define GPCR	0x1E
#define GPR	0x1F
#define TRPA	0x22
#define RWPA	0x24
#define VID	0x28
#define PID	0x2A
#define CHIPR	0x2C
#define SMCR	0x2F
#define MRCMDX	0xF0
#define MRCMD	0xF2
#define MRR	0xF4
#define MWCMDX	0xF6
#define MWCMD	0xF8
#define MWR	0xFA
#define TXPL	0xFC
#define ISR	0xFE
#define ISR_PTS	(1 << 1)
#define ISR_PRS	(1)

#define IMR	0xFF

#endif
