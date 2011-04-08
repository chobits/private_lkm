#include <linux/init.h>
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* everything\ldots{} */
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
static struct tasklet_struct my_tasklet;

static void my_handler(unsigned long data)
{
	printk(KERN_ALERT"hellow my_tasklet! data:%ld\n",data);

}

static int __init my_tasklet_init(void)
{
	printk(KERN_ALERT"starting init my_tasklet!\n");
	tasklet_init(&my_tasklet,my_handler,19900722);
	tasklet_schedule(&my_tasklet);
	return 0;
}

static void __exit my_tasklet_exit(void)
{
	tasklet_kill(&my_tasklet);
	printk(KERN_ALERT"exiting my_tasklet!\n");
}

MODULE_LICENSE("GPL");
module_init(my_tasklet_init);
module_exit(my_tasklet_exit);
