#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <asm-generic/errno-base.h>

#define pk(str,...) printk(KERN_ALERT str "\n", __VA_ARGS__)
#define pk0(str) printk(KERN_ALERT "[*]" str "\n")

static unsigned int ioaddr;
//static struct pci_dev *pdev;

int pd_p(struct pci_dev *dev, const struct pci_device_id *id)
{
	int err;
	pk0("in probe1");
//k	pdev = dev;
	err = pci_enable_device(dev);
	if (err < 0) {
		pk0("pci_enable_device");
		return err;
	}
	pci_set_master(dev);	
	pk0("in probe2");

	ioaddr = pci_resource_start(dev, 0);
	if (!ioaddr) {
		pk0("ioaddr NULL");
		return -ENODEV;	
	}

	pk0("in probe3");
	if (!request_region(ioaddr, 0x20, "pd_p")) {
		pk0("request_region");
		return -EBUSY;
	}
	
	pk0("in probe4");
	return 0;
}

void pd_r(struct pci_dev *dev)
{
	pk0("in pd remove, but there is no real remove func :(");

}

static DEFINE_PCI_DEVICE_TABLE(pd_tl) = {
	{PCI_DEVICE(0x1022, 0x2000),},
	{}
};
/*
static const struct pci_device_id pd_tl[] __devinitconst = {
	{
		.vendor = 0x1022,
		.device = 0x2000,
		.subvendor = PCI_ANY_ID,
		.subdevice = PCI_ANY_ID,
	},
	{ 0, }
};
*/

MODULE_DEVICE_TABLE(pci, pd_tl);

static struct pci_driver pd = {
	.name = "PCI_TEST",
	.probe = pd_p,
	.remove = __devexit_p(pd_r),
	.id_table = pd_tl,
};

#define pkpci(ptr, member, type) \
	printk(KERN_ALERT #member ": %" #type "\n", (ptr)->member)

int __init hello_init(void)
{
	if (!pci_register_driver(&pd))
		return -1;
/*
	if (pdev) {
		pkpci(pdev, revision, d);
		pkpci(pdev, hdr_type, d);
		pkpci(pdev, pcie_cap, d);
		pkpci(pdev, pcie_type, d);
		pkpci(pdev, rom_base_reg, d);
		pkpci(pdev, pin, d);
		pkpci(pdev, dma_mask, llx);
		pkpci(pdev, irq, u);
	}
*/
	return 0;
}

void __exit hello_exit(void)
{
	pci_unregister_driver(&pd);

}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);
