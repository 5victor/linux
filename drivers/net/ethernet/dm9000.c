#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/delay.h>

#include "dm9000.h"

//#define DEBUG

#ifdef DEBUG
#define debug(fmt, ARG...)	printk(fmt, ##ARG)
#else
#define debug(fmt, ARG...)
#endif

#ifdef DEBUG
#define bt()	dump_stack()
#else
#define bt()
#endif

#define DM9000_INDEX		0x20000000
#define DM9000_DATA_OFFSET	4

struct dm9k_info {
	void	*index;
	void	*data;
	int	queue_packet;
	int	packet2_len;
	int	ip_summed;
};

struct rx_info {
	u8	ready;
	u8	status;
	u16	lenght;
} __attribute__ ((packed));

static u8 read_reg_u8(struct dm9k_info *info, u8 index)
{
	iowrite8(index, info->index);
	return ioread8(info->data);
}

static void write_reg_u8(struct dm9k_info *info, u8 index, u8 data)
{
	iowrite8(index, info->index);
	iowrite8(data, info->data);
}

static u16 read_reg_u16(struct dm9k_info *info, u8 index)
{
	u16 reg;
	iowrite8(index + 1, info->index);
	reg = ioread8(info->data);
	reg <<= 8;
	iowrite8(index, info->index);
	reg |= ioread8(info->data) & 0xFF;

	return reg;
}

static void write_reg_u16(struct dm9k_info *info, u8 index, u16 data)
{
	iowrite8(index, info->index);
	iowrite8(data & 0xFF, info->data);
	iowrite8(index + 1, info->index);
	iowrite8((data >> 8) & 0xFF, info->data);
}

static void read_bulk_u8(struct dm9k_info *info, u8 index, u8 *buf, int size)
{
	int i;
	iowrite8(index, info->index);
	for (i = 0; i < size; i++) {
		buf[i] = ioread8(info->data);
	}
}

static void write_bulk_u8(struct dm9k_info *info, u8 index, u8 *buf, int size)
{
	int i;
	iowrite8(index ,info->index);
	for (i = 0; i < size; i++) {
		iowrite8(buf[i], info->data);
	}
}

static void read_bulk_u16(struct dm9k_info *info, u8 index, u16 *buf, int size)
{
	int i;
	iowrite8(index, info->index);
	for (i = 0; i < size / 2; i++) {
		buf[i] = ioread16(info->data);
	}
	if (size % 2) {
		((u8 *)buf)[i] = ioread8(info->data);
	}
}

static void write_bulk_u16(struct dm9k_info *info, u8 index, u16 *buf, int size)
{
	int i;
	iowrite8(index, info->index);
	for (i = 0; i < (size + 1) / 2; i++) {
		iowrite16(buf[i], info->data);
	}
}

#ifdef DEBUG
static void dump_skb(struct sk_buff *skb)
{
	printk(KERN_INFO "dm9000 dump skbuff:\n");
	print_hex_dump(KERN_INFO, "\t", DUMP_PREFIX_OFFSET,
		16, 1, skb->data, skb->len, 0);
}
#else
#define dump_skb(skb)

#endif

static void dm9000_dump_reg(struct dm9k_info *info)
{
	int i;

	printk(KERN_INFO "dm9000 dump reg:\n");
	for (i = 0; i < SMCR; i++) {
		printk(KERN_INFO "\tindex %x:0x%x\n", i, read_reg_u8(info, i));
	}
}

static void dm9000_reset(struct dm9k_info *info)
{
	u8 reg_u8;

	write_reg_u8(info, NCR, 1);
	udelay(10);
	do {
		reg_u8 = read_reg_u8(info, NCR);
	} while (reg_u8 & 0x1);
}

static void dm9000_init(struct dm9k_info *info)
{
	u16 reg_u16;
	
	reg_u16 = read_reg_u16(info, VID);
	printk(KERN_INFO "dm9000:VID 0x%x, PID 0x%x\n", reg_u16,
			read_reg_u16(info, PID));
	printk(KERN_INFO "dm9000:ISR 0x%x\n", read_reg_u8(info, ISR));

	write_reg_u8(info, GPR, 0);

	write_reg_u8(info, ISR, 0x0F);	
	write_reg_u8(info, IMR, 0xFF);

	write_reg_u8(info, RCR, 1);

	write_reg_u8(info, TCR, 0);
	write_reg_u8(info, BPTR, 0x3f);
	write_reg_u8(info, FCR, 0xff);
	write_reg_u8(info, SMCR, 0);
}

static void dm9000_tx_packet(struct dm9k_info *info, int ip_summed,  int len)
{
	debug(KERN_INFO "dm9000:tx packet len = %d\n", len);

	if (ip_summed == CHECKSUM_NONE) {
		debug(KERN_INFO "dm9000: CHECKSUM_NONE");
	}

	write_reg_u16(info, TXPL, len);

	write_reg_u8(info, TCR, 1);

}

static void dm9000_rx_packet(struct net_device *ndev)
{
	struct dm9k_info *info = netdev_priv(ndev);
	u8 ready = read_reg_u8(info, MRCMDX);
	struct rx_info rx_info;
	struct sk_buff *skb;
	void *buf;

	debug(KERN_INFO "dm9000:%s\n", __func__);
	if (!ready) {
		printk(KERN_INFO "dm9000:packet not ready interrupt ocurr\n");
		return ;
	}

	if (ready != 1) {
		printk(KERN_INFO "dm9000:packet rx ready != 1\n");
		return ;
	}
	
	read_bulk_u16(info, MRCMD, &rx_info, sizeof(struct rx_info)); 
	
	if (rx_info.status & RSR_LCS) {
		printk(KERN_INFO "dm9000: RSR_LCS\n");
		return ;
	}

	if (rx_info.status & RSR_RWTO) {
		printk(KERN_INFO "dm9000: RSR_RWTO\n");
		return ;
	}

	if (rx_info.status & RSR_PLE) {
		printk(KERN_INFO "dm9000: RSR_PLE\n");
		return ;
	}

	if (rx_info.status & RSR_AE) {
		printk(KERN_INFO "dm9000: RSR_AE\n");
		return ;
	}

	if (rx_info.status & RSR_CE) {
		printk(KERN_INFO "dm9000: RSR_CE CRC ERROR\n");
		return ;
	}

	if (rx_info.status & RSR_FOE) {
		printk(KERN_INFO "dm9000: RSR_FOE fifo overflow\n");
		return ;
	}

	skb = dev_alloc_skb(rx_info.lenght + 4);
	if (skb != NULL) {
		debug(KERN_INFO "dm9000:dmup skb pointer \n"); 
		skb_reserve(skb, NET_IP_ALIGN);
		buf = skb_put(skb, rx_info.lenght - 4);
		read_bulk_u16(info, MRCMD, buf, rx_info.lenght);
		
		debug(KERN_INFO "dump rx packet:\n");
		dump_skb(skb);

		skb->protocol = eth_type_trans(skb, ndev);
		debug(KERN_INFO "dm9000 FOR DEBUG delay:netif_rx called\n");
		netif_rx(skb);
	}
}

irqreturn_t dm9000_int_handler(int irq, void *dev_id)
{
	struct net_device *ndev = dev_id;
	struct dm9k_info *info = netdev_priv(ndev);
	u8 isr = read_reg_u8(info, ISR);

	debug(KERN_INFO "----------------------------Start %s--------------"
			"-----------------\n", __func__);	
	debug(KERN_INFO "dm9000_int_handler:isr 0x%x\n", isr);

	if (isr & ISR_ROS) {
		debug(KERN_INFO "dm9000:ISR_ROS\n");
	}

	if (isr & ISR_ROOS) {
		debug(KERN_INFO "dm9000:ISR_ROOS\n");
	}

	if (isr & ISR_PTS) {
		info->queue_packet--;
		if (info->queue_packet > 0) {
			dm9000_tx_packet(info, info->ip_summed, info->packet2_len);
			netif_start_queue(ndev);
		}
	}

	if (isr & ISR_PRS) {
		dm9000_rx_packet(ndev);
	}


	write_reg_u8(info, ISR, isr);
	debug(KERN_INFO "----------------------------End %s--------------"
			"-----------------\n", __func__);	

	return IRQ_HANDLED;
}

static int dm9000_open(struct net_device *ndev)
{
	struct dm9k_info *info = netdev_priv(ndev);
	u8 nsr;
	dm9000_reset(info);
	dm9000_init(info);
	enable_irq(ndev->irq);
	info->queue_packet = 0;
	debug("dm9000_open: tx queue stopped %d\n", netif_queue_stopped(ndev));

	do {
		nsr = read_reg_u8(info, NSR);
	} while (!(nsr & 0x40));
	printk(KERN_INFO "dm9000: link ok\n");
	
	netif_start_queue(ndev);

	netif_carrier_on(ndev);

	debug("dm9000_open: tx queue stopped %d\n", netif_queue_stopped(ndev));
	
	return 0;
}

static int dm9000_stop(struct net_device *ndev)
{
	struct dm9k_info *info = netdev_priv(ndev);

	netif_stop_queue(ndev);
	disable_irq(ndev->irq);

	write_reg_u8(info, GPR, 0x1);
	write_reg_u8(info, IMR, 0x0);
	write_reg_u8(info, RCR, 0x0);

	return 0;
}

static netdev_tx_t dm9000_start_xmit(struct sk_buff *skb,
			struct net_device *ndev)
{
	struct dm9k_info *info = netdev_priv(ndev);

	debug(KERN_INFO "%s:sk_buff %p\n", __func__, skb);
	debug(KERN_INFO "sk_buff: len = %d, data_len = %d\n", skb->len,
					skb->data_len);
	dump_skb(skb);
	
	if (info->queue_packet > 1)
		return NETDEV_TX_BUSY;
	debug(KERN_INFO "dm9000:TX Pointer:%d\n", read_reg_u16(info, MWR));
	write_bulk_u16(info, MWCMD, skb->data, skb->len);
	debug(KERN_INFO "dm9000:TX Pointer:%d\n", read_reg_u16(info, MWR));
	if (info->queue_packet++ == 0)
		dm9000_tx_packet(info, skb->ip_summed, skb->len);
	else {
		netif_stop_queue(ndev);
		info->packet2_len = skb->len;
		info->ip_summed = skb->ip_summed;
	}

	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}

static void dm9000_set_rx_mode(struct net_device *ndev)
{
	int i;
	struct dm9k_info *info = netdev_priv(ndev);

	for (i = 0; i < 6; i++) {
		write_reg_u8(info, PAR + i, ndev->dev_addr[i]);
	}
}

static void dm9000_timeout(struct net_device *ndev)
{
}

static int dm9000_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{

}

static struct net_device_ops dm9000_netdev_ops = 
{
	.ndo_open	= dm9000_open,
	.ndo_stop	= dm9000_stop,
	.ndo_start_xmit	= dm9000_start_xmit,
	.ndo_tx_timeout	= dm9000_timeout,
	.ndo_set_rx_mode	= dm9000_set_rx_mode,
	.ndo_do_ioctl	= dm9000_ioctl,
	.ndo_change_mtu	= eth_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= eth_mac_addr,
};

static int dm9000_module_init(void)
{
	struct net_device *ndev;
	struct dm9k_info *info;
	int ret;

	int i;

	ndev = alloc_etherdev(sizeof(struct dm9k_info));
	info = netdev_priv(ndev);
	info->index = ioremap(DM9000_INDEX, SZ_4K);
	printk(KERN_INFO "Map DM9000 INDEX REG ADDR:%p\n", info->index);
	info->data = info->index + DM9000_DATA_OFFSET;
	printk(KERN_INFO "Map DM9000 DATA REG ADDR:%p\n", info->data);
	ndev->irq = IRQ_EINT7;

	ndev->netdev_ops = &dm9000_netdev_ops;
	ndev->watchdog_timeo = msecs_to_jiffies(1000);

	s3c2440_eint_trigger(ndev->irq, high_level);
	request_irq(ndev->irq, dm9000_int_handler, 0, "dm9000", ndev);
	disable_irq(ndev->irq);
/*	dm9000_reset(info);
	dm9000_init(info);
*/
	if (!is_valid_ether_addr(ndev->dev_addr)) {
		random_ether_addr(ndev->dev_addr);
	}

	ret = register_netdev(ndev);
	
	if (ret) {
		printk(KERN_INFO "dm9000:register_netdev fail %d\n", ret);
		goto error_netdev;
	}

	return 0;

	free_netdev(ndev);
error_netdev:
	return ret;
}

static void dm9000_module_exit(void)
{

}

module_init(dm9000_module_init);
module_exit(dm9000_module_exit);
