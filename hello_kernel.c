#include <linux/init.h>
#include <linux/module.h>

static int __init init(void)
{
	printk(KERN_EMERG "OK%d\n", 11);
	return 0;
}

static void __exit exit(void)
{

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wang xiao chen <wangxiaochen0@gmail.com>");

module_init(init);
module_exit(exit);
