#include <linux/module.h>
#include <linux/kernel.h>


static struct module *p;
int __init hello_init(void)
{
	p = __module_address(0xe199a1ae);
	if (p)
		printk(KERN_ALERT "module name:%s\n", p->name);
	else
		printk(KERN_ALERT "no module\n");	
	return 0;
}
void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");

}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

