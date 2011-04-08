#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>


int __init hello_init(void)
{
	struct file_system_type **p;
	p = (struct file_system_type **)0xc0b0f234;
	for (; *p; p = &(*p)->next) 
		printk(KERN_ALERT "%s\n", (*p)->name);	
	return 0;
}

void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");

}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);
