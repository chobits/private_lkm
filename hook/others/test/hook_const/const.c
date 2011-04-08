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

static struct net_protocol *tcpp;
static unsigned char *nargs;
static int __init hello_init(void)
{
	unsigned int orig = 0;
	tcpp = (struct net_protocol *)0xc07dcf6c;

	orig = clearcr0();
	tcpp->no_policy = 1;
	resumecr0(orig);
	printk(KERN_ALERT "init ok\n");
	

	return 0;
}

static void __exit hello_exit(void)
{
	
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
