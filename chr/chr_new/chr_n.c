#include <linux/module.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/jiffies.h>

#define CHR_NAME ("chr_new")


int copen(struct inode *inode, struct file *filep);
ssize_t cread(struct file *filep, char __user *buf, size_t count, loff_t *pos);
ssize_t cwrite(struct file *filep, const char __user *buf, size_t, loff_t *pos);
int crelease(struct inode *inode, struct file *filep);

static int used = 0;
static char *kbuf, *kbuf_p;
static dev_t dev_n;
static struct cdev *cdev_p;
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = copen,
	.read = cread,
	.write = cwrite,
	.release = crelease
};

int copen(struct inode *inode, struct file *filep)
{
	int second,minite,hour;

	try_module_get(THIS_MODULE);
	if(used)
		return -EBUSY;
	
	used++;

	kbuf_p = kbuf;
	return 0;
}

ssize_t cread(struct file *filep, char __user *buf, size_t count, loff_t *pos)
{
	int i = count;
	int len = 0;
	if (*kbuf_p == 0)
		return 0;
	for(; i && *kbuf_p; i--) {
		put_user(*(kbuf_p++), buf++);
		len++;
	}

	return len;

}

/*
 * here is a bug!
 * if you write more than 1024 bytes into it,it will oops x-X!
 */
ssize_t cwrite(struct file *filep,
		const char __user *buf,	size_t count, loff_t *pos)
{
	int i = count;
	int len = 0;
	char *buf_p = buf;
	if(i>1024)
		i = 1024;
	for(; i > 0; i--,len++)
		copy_from_user(kbuf_p++, buf_p++, 1);
	
	return len;
}

int crelease(struct inode *inode, struct file *filep)
{
	used--;
	module_put(THIS_MODULE);
	return 0;
}

static int __init chr_new_init()
{
	int result;
	int i;
	if(!(kbuf = (char *)kmalloc(1024*sizeof(char), GFP_KERNEL))) {
		printk(KERN_ALERT"kmalloc error\n");
		return -1;
	}
	for (i=0; i<1024; i++)
		kbuf[i] = '\0';
	result = alloc_chrdev_region(&dev_n, 0, 1, CHR_NAME);	
	if (result < 0) {
		printk(KERN_ALERT"alloc chrdev number error\n");
		return result;
	}

	printk(KERN_ALERT"dev_n MAJOR:%d MINOR:%d\n", 
						MAJOR(dev_n),MINOR(dev_n));

	if (!(cdev_p = cdev_alloc())) {
		printk(KERN_ALERT"cdev_alloc error");
		return -1;
	}
	
	cdev_init(cdev_p, &fops);
	cdev_p->owner = THIS_MODULE;
	
	result = cdev_add(cdev_p, dev_n, 1);
	if (result < 0) {
		printk(KERN_ALERT"add cdev error\n");
		return result;
	}
	
	return 0;
}

static void __exit chr_new_exit()
{
	cdev_del(cdev_p);
	unregister_chrdev_region(dev_n, 1);
}

module_init(chr_new_init);
module_exit(chr_new_exit);

MODULE_AUTHOR("Wang Bo");
MODULE_LICENSE("GPL");






