#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <mach/regs-nand.h>
#include <mach/regs-clk.h>
#include <mach/regs-gpio.h>

struct s3c2440_nand {
	struct mtd_info mtd;
	struct nand_chip chip;
};

static struct s3c2440_nand s3c2440_nand;

static void s3c2440_nand_init_gpio(void)
{
	unsigned long clkcon, gpacon;

	clkcon = ioread32(REG_CLK(CLKCON));
	clkcon |= 1 << 4;
	iowrite32(clkcon, REG_CLK(CLKCON));

	gpacon = ioread32(REG_GPIO(GPACON));
	gpacon |= 0x5E0000;
	iowrite32(gpacon, REG_GPIO(GPACON));
}

static void s3c2440_nand_init_ctrl(void)
{
	unsigned long nfconf
	;
	nfconf = ioread32(REG_NAND(NFCONF));
	nfconf &= 0x1;
	nfconf |= NFCONF_TACLS(3) | NFCONF_TWRPH0(7) | NFCONF_TWRPH1(7);
	iowrite32(nfconf, REG_NAND(NFCONF));
	
	iowrite32(1, REG_NAND(NFCONT));
}

static void s3c2440_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf,
					int len)
{
	int i;

	for (i = 0; i < len; i++) {
		iowrite8(buf[i], REG_NAND(NFDATA));
	}
}

static void s3c2440_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		buf[i] = ioread8(REG_NAND(NFDATA));
	}
}

static void s3c2440_nand_select_chip(struct mtd_info *mtd, int chip)
{
	unsigned long nfcont;
	nfcont = ioread32(REG_NAND(NFCONT));

	if (chip == -1)
		nfcont |= 1 << 1;
	else
		nfcont &= ~(1 << 1);

	iowrite32(nfcont, REG_NAND(NFCONT));
}

static void s3c2440_nand_cmd_ctrl(struct mtd_info *mtd, int dat,
					unsigned int ctrl)
{
	if (dat == NAND_CMD_NONE)
		return;
	
	if (ctrl & NAND_CLE)
		iowrite8(dat, REG_NAND(NFCMMD));
	else
		iowrite8(dat, REG_NAND(NFADDR));
}

static int s3c2440_nand_dev_ready(struct mtd_info *mtd)
{
	unsigned long nfstat;
	
	nfstat = ioread8(REG_NAND(NFSTAT));
	return nfstat & 0x1;
}

static void s3c2440_nand_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	unsigned long nfcont;
	
	nfcont = ioread32(REG_NAND(NFCONT));
	nfcont |= 1 << 4;
	iowrite32(nfcont, REG_NAND(NFCONT));
}

static void dump_ecc()
{
	unsigned int nfmeccd0, nfmeccd1, nfmecc0, nfmecc1;
	nfmeccd0 = ioread32(REG_NAND(NFMECCD0));
	nfmeccd1 = ioread32(REG_NAND(NFMECCD1));
	nfmecc0 = ioread32(REG_NAND(NFMECC0));
	nfmecc1 = ioread32(REG_NAND(NFMECC1));

	printk(KERN_INFO "s3c2440 nand: nfmeccd0 %x nfmeccd1 %x nfmecc0 %x nfmecc1 %x\n", nfmeccd0, nfmeccd1, nfmecc0, nfmecc1);
}

static int s3c2440_nand_ecc_calculate_8bit(struct mtd_info *mtd,
			const uint8_t *dat, uint8_t *ecc_code)
{
	unsigned long nfmecc0;
	nfmecc0 = ioread32(REG_NAND(NFMECC0));

	ecc_code[0] = nfmecc0 & 0xFF;
	ecc_code[1] = (nfmecc0 >> 8) & 0xFF;
	ecc_code[2] = (nfmecc0 >> 16) & 0xFF;
	
	//dump_ecc();

	return 0;
}

static int s3c2440_nand_ecc_correct_8bit(struct mtd_info *mtd, uint8_t *dat,
			uint8_t *read_ecc, uint8_t *calc_ecc)
{
	int i;
	int result = 0;

	for (i = 0; i < 3; i++) {
		result |= read_ecc[i] ^ calc_ecc[i];
	}

	return (result > 0)?-result:result;
}

static void s3c2440_nand_init_nand_chip(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	unsigned long nfconf;

	chip->IO_ADDR_R = REG_NAND(NFDATA);
	chip->IO_ADDR_W = REG_NAND(NFDATA);

	chip->write_buf	= s3c2440_nand_write_buf;
	chip->read_buf	= s3c2440_nand_read_buf;
	chip->select_chip = s3c2440_nand_select_chip;
	chip->cmd_ctrl	= s3c2440_nand_cmd_ctrl;
	chip->dev_ready = s3c2440_nand_dev_ready;
	//chip->options = NAND_BUSWIDTH_16; /* 8bit width*/

	chip->ecc.mode = NAND_ECC_HW;
	chip->ecc.hwctl = s3c2440_nand_ecc_hwctl;
	nfconf = ioread32(REG_NAND(NFCONF));
	if (nfconf & 1) {
		panic("not imp 16bit bus width hw ecc");
	} else {
		chip->ecc.bytes = 3;
		chip->ecc.calculate = s3c2440_nand_ecc_calculate_8bit;
		chip->ecc.correct = s3c2440_nand_ecc_correct_8bit;
	}
}

static void s3c2440_nand_init_chip_again(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	if (mtd->writesize >= 2048)
		chip->ecc.size = 512;
	else
		chip->ecc.size = 256;
}

static int s3c2440_nand_init(void)
{
	s3c2440_nand_init_gpio();
	s3c2440_nand_init_ctrl();
	s3c2440_nand.mtd.priv = &s3c2440_nand.chip;
	s3c2440_nand.mtd.owner = THIS_MODULE;
	s3c2440_nand.mtd.name = &"s3c2440_nand";

	s3c2440_nand_init_nand_chip(&s3c2440_nand.mtd);

	nand_scan_ident(&s3c2440_nand.mtd, 1, NULL); /* always return 0 */

	s3c2440_nand_init_chip_again(&s3c2440_nand.mtd);

	nand_scan_tail(&s3c2440_nand.mtd);

	printk(KERN_INFO "s3c2440_nand:mtd_info subpage_sft = %d\n",
				s3c2440_nand.mtd.subpage_sft);
	return mtd_device_parse_register(&s3c2440_nand.mtd, NULL, 0, 0, NULL);
}

static void s3c2440_nand_exit(void)
{

}

module_init(s3c2440_nand_init);
module_exit(s3c2440_nand_exit);
