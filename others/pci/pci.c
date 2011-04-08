#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <asm-generic/errno-base.h>
#include <linux/netdevice.h>

#define pk(str,...) printk(KERN_ALERT "[*]"str "\n", __VA_ARGS__)
#define pk0(str) printk(KERN_ALERT "[*]" str "\n")

static unsigned int ioaddr;
static struct pci_dev *pcidev;
static int __devinit 
pd_p(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int err;
	

	pk0("in probe");
	err = pci_enable_device(pdev);
	if (err < 0) {
		pk0("pci_enable_device");
		return err;
	}
	pci_set_master(pdev);	

	pk0("set master");

	ioaddr = pci_resource_start(pdev, 0);
	if (!ioaddr) {
		pk0("ioaddr NULL");
		return -ENODEV;	
	}
	pk("ioaddr:0x%x", ioaddr);

	if (!pci_dma_supported(pdev, 0xffffffff)) {
		pk0("cannot supported");
	}
	pk0("dma supported!");

	if (!request_region(ioaddr, 0x20, "pd_p")) {
		pk0("cannot request region");
		return -EBUSY;
	}
	pk0("request_region");
	pcidev = pdev;
	return 0;
}

static void pd_r(struct pci_dev *pdev)
{
	struct net_device *dev;
	if (pdev) {
		if ((dev = pci_get_drvdata(pdev)) != NULL) {
			unregister_netdev(dev);
		}
		else
			pk0("no net device");
		release_region(ioaddr, 0x20);
		pci_disable_device(pdev);
		pk0("release region and disable pci");	
	}
}

static int pd_s(struct pci_dev *pcidev, pm_message_t state)
{
	pk0("in pd suspend");
	return 0;
}
static int pd_rs(struct pci_dev *pcidev)
{
	pk0("in pd resume");
	return 0;

}


static DEFINE_PCI_DEVICE_TABLE(pd_tl) = {
	{PCI_DEVICE(0x1022, 0x2000),},
	{PCI_DEVICE(0x1200, 0x2001),},
	{}
};

MODULE_DEVICE_TABLE(pci, pd_tl);

static struct pci_driver pd = {
	.name = "PCI_TEST",
	.probe = pd_p,
	.remove = __devexit_p(pd_r),
	.id_table = pd_tl,
	.suspend = pd_s,
	.resume = pd_rs,
};

#define pkpci(ptr, member, type) \
	printk(KERN_ALERT #member ": %" #type "\n", (ptr)->member)

int __init hello_init(void)
{
	if (pci_register_driver(&pd) != 0) {
		pk0("err register pci");
		return -1;
	}

	if (pcidev) {
		pkpci(pcidev, revision, d);
		pkpci(pcidev, hdr_type, d);
		pkpci(pcidev, pcie_cap, d);
		pkpci(pcidev, pcie_type, d);
		pkpci(pcidev, rom_base_reg, d);
		pkpci(pcidev, pin, d);
		pkpci(pcidev, dma_mask, llx);
		pkpci(pcidev, irq, u);
	}

	return 0;
}

void __exit hello_exit(void)
{
	pci_unregister_driver(&pd);

}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);
