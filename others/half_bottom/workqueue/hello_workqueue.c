#include <linux/init.h>
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* everything\ldots{} */
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

static struct work_struct *my_work;

static void my_work_handler(void *data)
{
	printk(KERN_ALERT"hellow my_work!\n");
}

static int __init my_work_init(void)
{
	my_work = kmalloc(sizeof(my_work),0);
	if (!my_work) {
		printk(KERN_ALERT "kmalloc error!\n");
		return -1;
	}

	printk(KERN_ALERT"starting init my_work!\n");

	INIT_WORK(my_work, my_work_handler);
	printk(KERN_ALERT"flush before scheduling my work");
	flush_scheduled_work();
	schedule_work(my_work);
	return 0;
}


static void __exit my_work_exit(void)
{
	kfree(my_work);
	printk(KERN_ALERT"exiting my_work!\n");
}

MODULE_LICENSE("GPL");
module_init(my_work_init);
module_exit(my_work_exit);
