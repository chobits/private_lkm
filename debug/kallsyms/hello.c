#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/init.h>


int __init hello_init(void)
{
	unsigned long ret;
	printk(KERN_ALERT "hello kernel\n");
	ret = kallsyms_lookup_name("sys_call_table");
	if (ret)
		printk(KERN_ALERT "ret:0x%lx\n", ret);
	else
		return -1;

	return 0;
}
void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");

}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

