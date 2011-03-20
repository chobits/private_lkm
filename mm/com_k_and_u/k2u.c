#include <linux/init.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/mm.h>

static unsigned long p;
static int __init k2u_init(void)
{
	p = get_zeroed_page(GFP_KERNEL);
	if (!p) {
		printk(KERN_ALERT "cannot alloc free page\n");
		return -ENOMEM;
	}
	SetPageReserved(virt_to_page(p));
	strcpy((char *)p, "hello u2k!\n");
	/* get the addr from dmesg | tail and use in u2k.c */
	printk(KERN_ALERT "pa:0x%lx\n", __pa(p));
	return 0;
}

static void __exit k2u_exit(void)
{
	printk(KERN_ALERT "user write: %s\n", (char *)p);
	ClearPageReserved(virt_to_page(p));
	free_pages(p, 0);
}

MODULE_LICENSE("GPL");

module_init(k2u_init);
module_exit(k2u_exit);
