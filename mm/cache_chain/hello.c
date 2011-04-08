#include <linux/module.h>
#include <linux/kernel.h>

#define pk(str,...) printk(KERN_ALERT str, __VA_ARGS__)

{
	printk(KERN_ALERT "hello kernel\n");
	return 0;
}

void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");

}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

