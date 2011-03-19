#include <linux/init.h>
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* everything\ldots{} */
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/jiffies.h>
#include <linux/types.h>
#include <asm/segment.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

static char *str;
static int number = 0;
static void print_time(const char *s);
static ssize_t read_p(struct file *filp, char *buf, size_t count,loff_t *p)
{
	static int i = 0;
	static int len = 0;
	if (*str == 0)
		return 0;
	for (i = count;i && *str;i++) {
		put_user(*str++, buf++);
		len++;
	}
//	printk(KERN_ALERT"*\n");	
	return len;
}

static ssize_t write_p(struct file *filp, char *buf, size_t count,loff_t *p)
{
	print_time("write error:");	
	return count;
}

static int open_p(struct inode *inode, struct file *filp)
{
	if(number)
		return -EBUSY;
	number++;
	str = "hello!";
//	printk(KERN_ALERT"k:open_p\n");
	printk(KERN_ALERT"k:inode->i_cdev->dev:major:%d,minor:%d\n",
			MAJOR(inode->i_cdev->dev),
			MINOR(inode->i_cdev->dev));
//	printk(KERN_ALERT"k:inode->i_cdev->ops->owner->name:%s\n",
//			inode->i_cdev->ops->owner->name);
//	printk(KERN_ALERT"k:inode->i_cdev->owner->name:%s\n",
//			inode->i_cdev->owner->name);
	try_module_get(THIS_MODULE);
	return 0;
}

static void release_p(struct inode *inode, struct file *filp)
{
	number--;
	module_put(THIS_MODULE);
//	print_time("release:");
}

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

static struct file_operations my_f_op = {
	.owner	 = THIS_MODULE,
	.read	 = read_p,
	.write	 = write_p,
	.open	 = open_p,
	.release = release_p,

static dev_t dev_n;
static int __init init(void)
{
	int result;
	printk(KERN_ALERT"k:start!\n");
	result = register_chrdev(0, "test_q", &my_f_op);
	if (result < 0) {
		printk(KERN_INFO"ERROR!\n");
		return result;	
	}
	dev_n = result;
	printk(KERN_ALERT"k:dev_n:%d\n",dev_n);
	return 0;
}


static void __exit exit(void)
{
	print_time("exit!");
	unregister_chrdev(dev_n, "test");
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
