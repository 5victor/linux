#ifndef S3C2440_UDC_H
#define S3C2440_UDC_H

struct s3c2440_ep {
	struct list_head queue;
	int index;
	struct usb_ep ep;
};

enum ep0_state {IDLE, DATA_IN, DATA_OUT};

struct s3c2440_gadget {
	struct usb_gadget_driver *driver;
	struct usb_gadget gadget;
	struct s3c2440_ep *ep;
	enum ep0_state ep0_state;
};

#endif
