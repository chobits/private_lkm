#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>

static unsigned long p = 0;
int __init hello_init(void)
{
	unsigned char *pp;
	p = __get_free_pages(GFP_KERNEL, 0);	
	SetPageReserved(virt_to_page(p));
	printk(KERN_ALERT "p = 0x%lx\n", p);

	pp = (unsigned char *)p;

	pp[0] = 0xa;
	pp[1] = 0xb;
	
	return 0;
}
void __exit hello_exit(void)
{
	ClearPageReserved(virt_to_page(p));
	free_pages(p, 0);
	printk(KERN_ALERT "Goodbye kernel\n");
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

