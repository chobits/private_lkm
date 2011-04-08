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
#include <linux/timer.h>
static struct timer_list my_timer;
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

static void my_handler(unsigned long data)
{
	print_time("delay:3s");
}

static int __init init(void)
{
	init_timer(&my_timer);
	my_timer.expires = jiffies+3*HZ;
	my_timer.data = 0;
	my_timer.function = my_handler;
	print_time("begin time");
	add_timer(&my_timer);
	return 0;
}

static void __exit exit(void)
{
	print_time("end time");
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
