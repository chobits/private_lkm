#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <net/protocol.h>

MODULE_LICENSE("GPL");

unsigned int clearcr0(void)
{
	unsigned int cr0 = 0;
	unsigned int ret;
	asm volatile ("movl %%cr0, %%eax"
			: "=a"(cr0));
	ret = cr0;
	
	cr0 &= 0xfffeffff;
	
	asm volatile ("movl %%eax, %%cr0"
			:: "a"(cr0));
	return ret;

}

void resumecr0(unsigned int value)
{
	asm volatile ("movl %%eax, %%cr0"
			:: "a"(value));
}

void ofun(void)
{
	printk("overflow!\n");
	while (1);
}
static unsigned char *nargs_h;
static int __init hello_init(void)
{
	unsigned int orig = 0;
	nargs_h = (unsigned char *)0xc07db440;
	orig = clearcr0();

	nargs_h[0] = 7 * sizeof(unsigned long);

	resumecr0(orig);
	printk(KERN_ALERT "init ok\n");
	printk("0x%x\n", (unsigned int)ofun);
	

	return 0;
}

static void __exit hello_exit(void)
{
	
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
