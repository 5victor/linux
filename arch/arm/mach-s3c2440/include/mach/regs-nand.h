#include <mach/map.h>

#define	NFCONF	0x0
#define NFCONF_TACLS(x)	((x) << 12)
#define NFCONF_TWRPH0(x)	((x) << 8)
#define NFCONF_TWRPH1(x)	((x) << 4)

#define	NFCONT	0x4
#define NFCONT_REG_NCE	(1 << 1)
#define NFCONT_MODE	(1)

#define NFCMMD	0x8

#define NFADDR	0xC

#define NFDATA	0x10

#define NFMECCD0	0x14

#define NFMECCD1	0x18

#define NFSECCD		0x1C

#define NFSTAT		0x20

#define NFESTAT0	0x24

#define NFESTAT1	0x28

#define NFMECC0		0x2C

#define NFMECC1		0x30

#define NFSECC		0x34

#define NFSBLK		0x38

#define NFEBLK		0x3C

#define REG_NAND(x)	((x) + S3C2440_VA_NAND)
