#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/usb/gadget.h>
#include <linux/usb/ch9.h>
#include <mach/irqs.h>
#include <mach/regs-clk.h>
#include <mach/regs-gpio.h>
#include <mach/regs-udc.h>
#include "s3c2440_udc.h"

irqreturn_t s3c2440_gadget_int_handle(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}

static int s3c2440_ep_enable(struct usb_ep *ep, 
		const struct usb_endpoint_descriptor *desc)
{
	struct s3c2440_ep *s3c2440_ep = container_of(ep, struct s3c2440_ep, ep);

	iowrite8(s3c2440_ep->index, REG_UDC(INDEX_REG));
	if (unlikely(s3c2440_ep->index == 0))
		iowrite8(1, REG_UDC(MAXP_REG));
	else
		iowrite8(1 << 3, REG_UDC(MAXP_REG));

	if (s3c2440_ep->index != 0) {
		if (desc->bEndpointAddress & USB_DIR_IN) {
			iowrite8(1 << 3, REG_UDC(IN_CSR1_REG));
			iowrite8(1 << 5, REG_UDC(IN_CSR2_REG));
		} else {
			iowrite8(1 << 4, REG_UDC(OUT_CSR1_REG));
			iowrite8(0, REG_UDC(IN_CSR2_REG));
		}
	}
	iowrite8(1 << s3c2440_ep->index, REG_UDC(EP_INT_EN_REG));

	return 0;
}

static int s3c2440_ep_disable(struct usb_ep *ep)
{
	struct s3c2440_ep *s3c2440_ep = container_of(ep, struct s3c2440_ep, ep);
	unsigned int inten = ioread8(REG_UDC(EP_INT_EN_REG));

	iowrite8(s3c2440_ep->index, REG_UDC(INDEX_REG));

	inten &= ~(1 << s3c2440_ep->index);
	iowrite8(inten, REG_UDC(EP_INT_EN_REG));

	return 0;
}

static void s3c2440_ep_free_request(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req);
}

static int s3c2440_ep_queue(struct usb_ep *ep, struct usb_request *req,
		gfp_t gfp_flags)
{
	return 0;
}

static int s3c2440_ep_dequeue(struct usb_ep *ep, struct usb_request *req)
{
	return 0;
}

static struct usb_request *s3c2440_ep_alloc_request(struct usb_ep *ep,
		gfp_t gfp_flags)
{
	struct usb_request *req = kzalloc(sizeof(struct usb_request),
			gfp_flags);

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
	unsigned long gpccon, gpcdat;
	clkcon = ioread32(REG_CLK(CLKCON));
	clkcon |= 1 << 7;
	iowrite32(clkcon, REG_CLK(CLKCON));

	iowrite32(0, REG_UDC(PWR_REG));
	iowrite32(0, REG_UDC(USB_INT_EN_REG));


	gpccon = ioread32(REG_GPIO(GPCCON));
	gpccon &= ~(3 << 10);
	gpccon |= 1 < 10;
	iowrite32(gpccon, REG_GPIO(GPCCON));
	gpcdat = ioread32(REG_GPIO(GPCDAT));
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
	s3c2440_gadget_hw_enable();
	s3c2440_ep_enable(gadget->ep0, NULL);
	enable_irq(IRQ_USBD);
	return 0;
}

static int s3c2440_gadget_udc_stop(struct usb_gadget *gadget,
			struct usb_gadget_driver *gadget_driver)
{
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
	}
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
	}
}

static int s3c2440_udc_init(void)
{	
	int ret;

	s3cc2440_gadget_init(&s3c2440_gadget);
	ret = request_irq(IRQ_USBD, s3c2440_gadget_int_handle, 0, "usbd", NULL);
	if (ret) {
		printk(KERN_INFO "s3c2440 udc:request irq error\n");
		goto request_irq_error;
	}

	ret = usb_add_gadget_udc(NULL, &s3c2440_gadget.gadget);
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

static void s3c2440_udc_exit(void)
{

}

module_init(s3c2440_udc_init);
module_exit(s3c2440_udc_exit);
