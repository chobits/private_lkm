/* a simple lkm example */
#include <linux/init.h>
#include <linux/module.h>

static int __init init(void)
{
	printk(KERN_EMERG "hello kernel\n");
	return 0;
}

static void __exit exit(void)
{
	printk(KERN_EMERG "goodbye kernel\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xiaochen Wang <wangxiaochen0@gmail.com>");

module_init(init);
module_exit(exit);
