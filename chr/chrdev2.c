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


static void __exit exit(void)
{
	printk(KERN_ALERT"k:exit!\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wang xiao chen <wangxiaochen0@gmail.com>");
//module_param(mi, int, 0);
//MODULE_PARM_DESC(mi,"An integer");

//module_init(init);
module_exit(exit);
