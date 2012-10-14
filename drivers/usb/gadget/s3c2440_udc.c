#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/usb/gadget.h>
#include <linux/usb/ch9.h>
#include <mach/irqs.h>
#include <mach/regs-clk.h>
#include <mach/regs-gpio.h>
#include <mach/regs-udc.h>
#include "s3c2440_udc.h"

#define DEBUG

#ifdef DEBUG
#define 	debug(fmt,ARG...)	printk(fmt,##ARG)
#else
#define		debug(fmt,ARG...)
#endif

#define UDC_EP_NUM 5


#ifdef DEBUG

static void dump_usb_ctrlrequest(struct usb_ctrlrequest *ctrl_req)
{
	printk(KERN_INFO "s3c2440 udc:dump usb_ctrlrequest\n"
			"\tbRequestType:0x%x\n"
			"\tbRequest:0x%x\n"
			"\twValue:0x%x\n"
			"\twIndex:0x%x\n"
			"\twLength:0x%x\n",
			ctrl_req->bRequestType, ctrl_req->bRequest,
			ctrl_req->wValue, ctrl_req->wIndex, ctrl_req->wLength);
}

#else

#define dump_usb_ctrlrequest(x)

#endif

#ifdef DEBUG

static void dump_usb_request(struct usb_request *req)
{
	printk(KERN_INFO "s3c2440 udc:dump usb_request\n"
			"\tlength:0x%x\n"
			"\tactual:0x%x\n"
			"\tzero:%d\n",
			req->length, req->actual, req->zero);
	print_hex_dump(KERN_INFO, NULL, DUMP_PREFIX_OFFSET, 8, 1,
			req->buf, req->length, 0);
}

#else

#define dump_usb_request(x)

#endif

static void s3c2440_ep_write_fifo(int index, char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		iowrite8(buf[i], REG_UDC(EPn_FIFO_REG(index)));
	}
}

static void s3c2440_ep_read_fifo(int index, char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		buf[i] = ioread8(REG_UDC(EPn_FIFO_REG(index)));
	}
}

/*

static void s3c2440_ep0_read_ctrl_req(struct usb_ctrlrequest *ctrl_req)
{
	u8 ep0_csr = ioread8(REG_UDC(EP0_CSR));

	s3c2440_ep_read_fifo(0, ctrl_req, sizeof(struct usb_ctrlrequest));
	ep0_csr |= 1 << 6;
	iowrite8(ep0_csr, REG_UDC(EP0_CSR));
}

static void s3c2440_ep0_zero_data_end(void)
{
	u8 ep0_csr = ioread8(REG_UDC(EP0_CSR));

	//ep0_csr |= 1 << 1;
	ep0_csr |= 1 << 3;
	iowrite8(ep0_csr, REG_UDC(EP0_CSR));
}

static void s3c2440_ep0_data_end(void)
{
	u8 ep0_csr = ioread8(REG_UDC(EP0_CSR));

	ep0_csr |= 1 << 3;
	iowrite8(ep0_csr, REG_UDC(EP0_CSR));
}

*/

static void s3c2440_ep0_out_data_end(void)
{
	u8 ep0_csr = ioread8(REG_UDC(EP0_CSR));

	ep0_csr |= (1 << 6) | (1 << 3);
	iowrite8(ep0_csr, REG_UDC(EP0_CSR));
}

static void s3c2440_ep0_in_data_end(void)
{
	u8 ep0_csr = ioread8(REG_UDC(EP0_CSR));

	ep0_csr |= (1 << 3) | (1 << 1);
	iowrite8(ep0_csr, REG_UDC(EP0_CSR));
}

static void s3c2440_ep0_out_rdy(void)
{

	u8 ep0_csr = ioread8(REG_UDC(EP0_CSR));

	ep0_csr |= (1 << 6);
	iowrite8(ep0_csr, REG_UDC(EP0_CSR));
}

static void s3c2440_ep0_in_rdy(void)
{
	u8 ep0_csr = ioread8(REG_UDC(EP0_CSR));

	ep0_csr |= (1 << 1);
	iowrite8(ep0_csr, REG_UDC(EP0_CSR));

}

static void s3c2440_ep_complete_request(struct usb_ep *ep,
			struct usb_request *req, int status)
{
	list_del_init(&req->list);
	req->status = status;
	req->complete(ep, req);
}

static void s3c2440_ep0_handle_idle(struct s3c2440_gadget *s3c2440_gadget)
{
	struct usb_ctrlrequest ctrl_req;

	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);
	
	s3c2440_ep_read_fifo(0, &ctrl_req, sizeof(ctrl_req));
	dump_usb_ctrlrequest(&ctrl_req);

	switch(ctrl_req.bRequest) {
		case USB_REQ_SET_ADDRESS:
			debug(KERN_INFO "s3c2440 udc: USB_REQ_SET_ADDRESS\n");
			iowrite8((1 << 7) | ctrl_req.wValue,
					REG_UDC(FUNC_ADDR_REG));
			s3c2440_ep0_out_data_end();
			break;
		case USB_REQ_GET_DESCRIPTOR:
			s3c2440_gadget->ep0_rest = ctrl_req.wLength;
			s3c2440_gadget->ep0_state = DATA_IN;
			s3c2440_ep0_out_rdy();
			s3c2440_gadget->driver->setup(&s3c2440_gadget->gadget,
						&ctrl_req);
			break;
	}
}

static void s3c2440_ep0_out_request(struct s3c2440_ep *ep,
					struct usb_request *req)
{
	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);

}

static void s3c2440_ep0_in_request(struct s3c2440_ep *ep,
					struct usb_request *req)
{
	int index = req->actual;
	int len;

	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);
	dump_usb_request(req);

	len = req->length - req->actual;

	len = len >= ep->ep.maxpacket ? ep->ep.maxpacket : len;

	s3c2440_ep_write_fifo(0, &((char *)req->buf)[index], len);
	req->actual += len;
	if (req->actual == req->length)
		s3c2440_ep0_in_data_end();
	else
		s3c2440_ep0_in_rdy();
}

static void s3c2440_ep0_handle(struct s3c2440_gadget *s3c2440_gadget)
{
	u8 ep0_csr = ioread8(REG_UDC(EP0_CSR));
	struct s3c2440_ep *ep0 = &s3c2440_gadget->ep[0];
	struct usb_request *req = list_empty(&ep0->queue) ? NULL :
			list_entry(ep0->queue.next, struct usb_request, list);
	
	debug(KERN_INFO "s3c2440 udc:EP0_CSR = 0x%x\n", ep0_csr);

	if (ep0_csr & (1 << 4)) {
		s3c2440_gadget->ep0_state = IDLE;
		debug(KERN_INFO "s3c2440 udc:control SETUP_END\n");
		iowrite8(1 << 7, REG_UDC(EP0_CSR));
		//if (ep0_csr & 1)
		//	iowrite8(1 << 6, REG_UDC(EP0_CSR));
		//return ;
	}

	if ((ep0_csr & (1 << 1)) && !(ep0_csr & 1))
		return;

	if (req != NULL && req->actual == req->length &&
			s3c2440_gadget->ep0_state == DATA_IN) {
		s3c2440_ep_complete_request(&ep0->ep, req, 0);
		s3c2440_gadget->ep0_state = IDLE;
		debug(KERN_INFO "s3c2440 udc:ep0  data in complete\n");
	}
	
	switch(s3c2440_gadget->ep0_state) {
	case IDLE:
		if (ep0_csr & 1)
			s3c2440_ep0_handle_idle(s3c2440_gadget);
		break;
	case DATA_IN:
		if (!(ep0_csr & (1 << 1)) && req != NULL) {
				s3c2440_ep0_in_request(ep0, req);
		} //那个usb_request什么时候出队，还是因为ep0的关系不出队，只把length,actual什么的改变。
		break;
	case DATA_OUT:
		if (ep0_csr & 1)
			//s3c2440_ep0_handle_data_out(s3c2440_gadget);
		break;
	}
}

static void s3c2440_ep_out_data(struct s3c2440_ep *ep)
{
	int len = 0;
	u8 out_csr1_reg;

	iowrite8(ep->index, REG_UDC(INDEX_REG));
	out_csr1_reg = ioread8(REG_UDC(OUT_CSR1_REG));

	if (!(out_csr1_reg & 0x1))
		return;

	len = ioread8(REG_UDC(OUT_FIFO_CNT1_REG));
	len |= ioread8(REG_UDC(OUT_FIFO_CNT2_REG)) << 8;
	debug(KERN_INFO "s3c2440 udc:s3c2440_ep_out_data len = %d\n", len);


}

static void s3c2440_ep_in_data(struct s3c2440_ep *ep)
{

}

static void s3c2440_ep_handle(struct s3c2440_ep *ep)
{
	if (list_empty(&ep->queue))
		return ;
	if (usb_endpoint_dir_in(ep->ep.desc))
		s3c2440_ep_in_data(ep);
	else
		s3c2440_ep_out_data(ep);
}

irqreturn_t s3c2440_gadget_int_handle(int irq, void *dev_id)
{
	u8 ep_int_reg;
	u8 usb_int_reg;

	int i;
	struct s3c2440_gadget *s3c2440_gadget = dev_id;
	struct s3c2440_ep *ep = s3c2440_gadget->ep;

	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);

	usb_int_reg = ioread8(REG_UDC(USB_INT_REG));
	iowrite8(usb_int_reg, REG_UDC(USB_INT_REG));
	debug(KERN_INFO "s3c2440 udc:usb_int_reg 0x%x\n", usb_int_reg);
	if (usb_int_reg & (1 << 2)) {
		s3c2440_gadget->ep0_state = IDLE;
		debug(KERN_INFO "s3c2440_udc:usb reset\n");
	}

	ep_int_reg = ioread8(REG_UDC(EP_INT_REG));
	iowrite8(ep_int_reg, REG_UDC(EP_INT_REG));
	debug(KERN_INFO "s3c2440 udc:ep_int_reg 0x%x\n", ep_int_reg);

	if (ep_int_reg & 1)
		s3c2440_ep0_handle(s3c2440_gadget);

	for (i = 1; i < UDC_EP_NUM; i++) {
		if (ep_int_reg & (1 << i))
			s3c2440_ep_handle(&ep[i]);
	}

	return IRQ_HANDLED;
}

static int s3c2440_ep_enable(struct usb_ep *usb_ep, 
		const struct usb_endpoint_descriptor *desc)
{
	u8 ep_int_en_reg;
	struct s3c2440_ep *ep = container_of(usb_ep, struct s3c2440_ep, ep);

	debug(KERN_INFO "s3c2440 udc:%s ep index %d\n", __func__, ep->index);

	iowrite8(ep->index, REG_UDC(INDEX_REG));
	if (unlikely(ep->index == 0))
		iowrite8(1, REG_UDC(MAXP_REG));
	else
		iowrite8(1 << 3, REG_UDC(MAXP_REG));

	if (ep->index != 0) {
		if (desc->bEndpointAddress & USB_DIR_IN) {
			iowrite8(1 << 3, REG_UDC(IN_CSR1_REG));
			iowrite8(1 << 5, REG_UDC(IN_CSR2_REG));
		} else {
			iowrite8(1 << 4, REG_UDC(OUT_CSR1_REG));
			iowrite8(0, REG_UDC(IN_CSR2_REG));
		}
	}
	ep_int_en_reg = ioread8(REG_UDC(EP_INT_EN_REG));
	ep_int_en_reg |= 1 << ep->index;
	iowrite8(ep_int_en_reg, REG_UDC(EP_INT_EN_REG));

	return 0;
}

static int s3c2440_ep_disable(struct usb_ep *usb_ep)
{
	struct s3c2440_ep *ep = container_of(usb_ep, struct s3c2440_ep, ep);
	unsigned int ep_int_en_reg = ioread8(REG_UDC(EP_INT_EN_REG));
	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);

	iowrite8(ep->index, REG_UDC(INDEX_REG));

	ep_int_en_reg &= ~(1 << ep->index);
	iowrite8(ep_int_en_reg, REG_UDC(EP_INT_EN_REG));

	return 0;
}

static void s3c2440_ep_free_request(struct usb_ep *usb_ep,
					struct usb_request *req)
{
	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);
	kfree(req);
}

static int s3c2440_ep_queue(struct usb_ep *usb_ep, struct usb_request *req,
		gfp_t gfp_flags)
{
	struct s3c2440_ep *ep = container_of(usb_ep, struct s3c2440_ep, ep);
	struct s3c2440_gadget *gadget = ep->gadget;

	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);
	req->actual = 0;
	req->status = -EINPROGRESS;

	if (unlikely(ep->index == 0)) {
		if (!list_empty(&ep->queue))
			printk(KERN_INFO "s3c2440 udc:ep0 queue is not emtpy but usb_ep_queue called \n");
		if (gadget->ep0_state == DATA_IN)
			s3c2440_ep0_in_request(ep, req);
		return 0;
	}
	list_add_tail(&req->list, &ep->queue);

	return 0;
}

static int s3c2440_ep_dequeue(struct usb_ep *usb_ep, struct usb_request *req)
{
	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);
	return 0;
}

static struct usb_request *s3c2440_ep_alloc_request(struct usb_ep *usb_ep,
		gfp_t gfp_flags)
{
	struct usb_request *req = kzalloc(sizeof(struct usb_request),
			gfp_flags);

	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);

	if (req)
		INIT_LIST_HEAD(&req->list);

	return req;
}



static struct usb_ep_ops s3c2440_ep_ops = {
	.enable = s3c2440_ep_enable,
	.disable = s3c2440_ep_disable,
	.alloc_request = s3c2440_ep_alloc_request,
	.free_request = s3c2440_ep_free_request,
	.queue = s3c2440_ep_queue,
	.dequeue = s3c2440_ep_dequeue,
};

static struct s3c2440_ep s3c2440_ep[] = {
	{
		.index = 0,
		.ep = {
			.name	= "ep0",
			.ops	= &s3c2440_ep_ops,
			.maxpacket	= 8,
		},
	}, {
		.index = 1,
		.ep = {
			.name		= "ep1",
			.ops		= &s3c2440_ep_ops,
			.maxpacket	= 64,
		},
	}, {
		.index = 2,
		.ep = {
			.name		= "ep2",
			.ops		= &s3c2440_ep_ops,
			.maxpacket	= 64,
		},
	}, {
		.index = 3,
		.ep = {
			.name		= "ep3",
			.ops		= &s3c2440_ep_ops,
			.maxpacket	= 64,
		},
	}, {
		.index = 4,
		.ep = {
			.name		= "ep4",
			.ops		= &s3c2440_ep_ops,
			.maxpacket	= 64,
		},
	},
};

static int s3c2440_gadget_get_frame(struct usb_gadget *gadget)
{
	return 0;
}

static void s3c2440_gadget_hw_enable(void)
{
	unsigned long clkcon;
	u32 upllcon = ioread32(REG_CLK(UPLLCON));

	clkcon = ioread32(REG_CLK(CLKCON));
	clkcon |= 1 << 7;
	iowrite32(clkcon, REG_CLK(CLKCON));
	debug(KERN_INFO "s3c2440 udc:UPLLCON=0x%x\n", upllcon);

	iowrite32(0, REG_UDC(PWR_REG));
	iowrite32((1 << 2) | (1 << 1) | 1, REG_UDC(USB_INT_EN_REG));

}

static void s3c2440_gadget_pullup(void)
{
	u32 gpccon, gpcdat, gpcup;
	gpccon = ioread32(REG_GPIO(GPCCON));
	gpccon &= ~(3 << 10);
	gpccon |= 1 << 10;
	iowrite32(gpccon, REG_GPIO(GPCCON));

	gpcup = ioread32(REG_GPIO(GPCUP));
	gpcup |= 1 << 5;
	iowrite32(gpcup, REG_GPIO(GPCUP));

	gpcdat = ioread32(REG_GPIO(GPCDAT));
	debug(KERN_INFO "s3c2440 udc:gpcdat = 0x%x\n", gpcdat);
	gpcdat |= 1 << 5;
	iowrite32(gpcdat, REG_GPIO(GPCDAT));
}

static void s3c2440_gadget_hw_disable(void)
{
	unsigned long clkcon, gpcdat;
	clkcon = ioread32(REG_CLK(CLKCON));
	clkcon &= ~(1 << 7);
	iowrite32(clkcon, REG_CLK(CLKCON));

	gpcdat = ioread32(REG_GPIO(GPCDAT));
	gpcdat &= ~(1 << 5);
	iowrite32(gpcdat, REG_GPIO(GPCDAT));
}

static int s3c2440_gadget_udc_start(struct usb_gadget *gadget,
		struct usb_gadget_driver *gadget_driver)
{
	struct s3c2440_gadget *s3c2440_gadget;

	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);

	s3c2440_gadget = container_of(gadget, struct s3c2440_gadget, gadget);
	s3c2440_gadget->driver = gadget_driver;

	s3c2440_gadget_hw_enable();
	s3c2440_ep_enable(gadget->ep0, NULL);
	s3c2440_gadget_pullup();

	enable_irq(IRQ_USBD);
	return 0;
}

static int s3c2440_gadget_udc_stop(struct usb_gadget *gadget,
			struct usb_gadget_driver *gadget_driver)
{
	struct s3c2440_gadget *s3c2440_gadget;

	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);

	s3c2440_gadget = container_of(gadget, struct s3c2440_gadget, gadget);
	s3c2440_gadget->driver = NULL;

	s3c2440_gadget_hw_disable();
	s3c2440_ep_disable(gadget->ep0);
	disable_irq(IRQ_USBD);
	return 0;
}
		

static struct usb_gadget_ops s3c2440_gadget_ops = {
	.get_frame		= s3c2440_gadget_get_frame,
	.udc_start		= s3c2440_gadget_udc_start,
	.udc_stop		= s3c2440_gadget_udc_stop,
};

static struct s3c2440_gadget s3c2440_gadget = {
	.gadget = {
		.ops = &s3c2440_gadget_ops,
		.name = "s3c2440-gadget",
		.speed = USB_SPEED_FULL,
		.dev = {
			.init_name = "s3c2440-gadget",
		},
	},
	.ep = s3c2440_ep,

};

static void s3cc2440_gadget_init(struct s3c2440_gadget *s3c2440_gadget)
{
	int i;

	s3c2440_gadget->driver = NULL;
	s3c2440_gadget->gadget.ep0 = &s3c2440_ep[0].ep;

	s3c2440_gadget->ep0_state = IDLE;

	INIT_LIST_HEAD(&s3c2440_gadget->gadget.ep_list);
	INIT_LIST_HEAD(&s3c2440_ep[0].ep.ep_list);

	for (i = 0; i < ARRAY_SIZE(s3c2440_ep); i++) {
		if (i != 0)
			list_add_tail(&s3c2440_ep[i].ep.ep_list,
					&s3c2440_gadget->gadget.ep_list);
		INIT_LIST_HEAD(&s3c2440_ep[i].queue);
		s3c2440_ep[i].gadget = s3c2440_gadget;
	}
}

static int s3c2440_udc_probe(struct platform_device *pdev)
{	
	int ret;
	debug(KERN_INFO "s3c2440 udc:%s\n", __func__);

	s3cc2440_gadget_init(&s3c2440_gadget);
	ret = request_irq(IRQ_USBD, s3c2440_gadget_int_handle, 0, "usbd",
				&s3c2440_gadget);
	if (ret) {
		printk(KERN_INFO "s3c2440 udc:request irq error\n");
		goto request_irq_error;
	}
	
	s3c2440_gadget.gadget.dev.parent = &pdev->dev;
	device_register(&s3c2440_gadget.gadget.dev);

	ret = usb_add_gadget_udc(&pdev->dev, &s3c2440_gadget.gadget);
	if (ret) {
		printk(KERN_INFO "s3c2440 udc:usb_add_gadget_udc fail %d\n",
				ret);
		goto add_gadget_fail;
	}

	return 0;

add_gadget_fail:
	free_irq(IRQ_USBD, NULL);
request_irq_error:
	return ret;
}

static int s3c2440_udc_remove(struct platform_device *pdev)
{
	device_unregister(&s3c2440_gadget.gadget.dev);
	usb_del_gadget_udc(&s3c2440_gadget.gadget);
	free_irq(IRQ_USBD, NULL);

	return 0;
}

static struct platform_driver s3c2440_udc_driver = {
	.probe		= s3c2440_udc_probe,
	.remove		= s3c2440_udc_remove,
	.driver		= {
		.name	= "s3c2440-udc",
		.owner	= THIS_MODULE,
	},
};

static int s3c2440_udc_init(void)
{
	return platform_driver_register(&s3c2440_udc_driver);
}

static void s3c2440_udc_exit(void)
{

}

module_init(s3c2440_udc_init);
module_exit(s3c2440_udc_exit);
