#include <linux/init.h>
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* everything\ldots{} */
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/types.h>
#include <asm/segment.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

static void print_time(const char *s)
{
	int second,minite,hour;
	second = jiffies/HZ;
	minite = second / 60;
	hour = minite / 60;
	second %= 60;
	minite %= 60;
	printk(KERN_ALERT"%s:%d:%d:%d\n",s,hour,minite,second);
}

static int __init init(void)
{
	print_time("before hlt");
	__asm__ ("hlt\n\t");
	print_time("after hlt");
	return 0;
}


static void __exit exit(void)
{
	print_time("exit");
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
