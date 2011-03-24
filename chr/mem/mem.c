#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm-generic/uaccess.h>

#define memdbg(fmt, arg...)\
	printk(KERN_ALERT "%s " fmt "\n", __FUNCTION__, ##arg)

/* bottomless pit, you can write anyth*/
static ssize_t null_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *pos)
{
	return count;
}

static ssize_t null_read(struct file *filp, char __user *buf,
		size_t count, loff_t *pos)
{
	return 0;
}

static loff_t null_llseek(struct file *filp, loff_t offset, int orig)
{
	return filp->f_pos = 0;
}

static struct file_operations null_fops = {
	/*
	 * not set THIS_MODULE here,or you,opening the file,will get_module:
	 * in sys_open->dentry_open: get_module(filp->fops:memdev_fops->owner);
	 * then in memdev_open: change filp->fops to zero_fops
	 * in sys_close --> put_module(filp->fops:zero_fops ===> BUG
	 */
	/* .owner = THIS_MODULE, */
	.llseek = null_llseek,
	.read = null_read,
	.write = null_write,
};

static ssize_t zero_read(struct file *filp, char __user *buf,
						size_t count, loff_t *pos)
{
	int chunk, unwrited;
	size_t rsize;


	if (!count)
		return 0;

	rsize = 0;
	/* why not read the whole zero data? */
	while (count) {
		chunk = PAGE_SIZE;
		if (chunk > count)
			chunk = count;
		unwrited = __clear_user(buf, chunk);
		rsize += chunk - unwrited;
		if (unwrited)
			break;
		count -= chunk;		
	}

	return rsize ? rsize : -EFAULT;
}

#define zero_llseek null_llseek
#define zero_write null_write

static struct file_operations zero_fops = {
	/*
	 * not set THIS_MODULE here,or you,opening the file,will get_module:
	 * in sys_open->dentry_open: get_module(filp->fops:memdev_fops->owner);
	 * then in memdev_open: change filp->fops to zero_fops
	 * in sys_close --> put_module(filp->fops:zero_fops ===> BUG
	 */
	/* .owner = THIS_MODULE, */
	.llseek = zero_llseek,
	.read = zero_read,
	.write = zero_write,
};

static dev_t mem_devnum;
struct memdev {
	const char *name;
	const struct file_operations *fops;	
	mode_t mode;
} mem_dev_list[] = {
	[0] = {"zero", &zero_fops, 0666},
	[1] = {"null", &null_fops, 0666},
};


static int memdev_open(struct inode *inode, struct file *filp)
{
	int minor;
	const struct memdev *dev;
	
	minor = iminor(inode);
	if (minor >= ARRAY_SIZE(mem_dev_list))
		return -ENXIO;

	dev = &mem_dev_list[minor];
	if (!dev->fops)
		return -ENXIO;

	filp->f_op = dev->fops;
	if (filp->f_op->open)
		return filp->f_op->open(inode, filp);
	return 0;
}



static const struct file_operations memdev_fops = {
	/*
	 * not set THIS_MODULE here,or you,opening the file,will get_module:
	 * in sys_open->dentry_open: get_module(filp->fops:memdev_fops->owner);
	 * then in memdev_open: change filp->fops to zero_fops
	 * in sys_close --> put_module(filp->fops:zero_fops ===> BUG
	 */
	/* .owner = THIS_MODULE, */
	.open = memdev_open,
};

static struct cdev *mem_dev;

static int __init init(void)
{
	int err;
	err = alloc_chrdev_region(&mem_devnum, 0, ARRAY_SIZE(mem_dev_list),
								"xmem");
	if (err < 0) {
		memdbg("alloc_chrdev_region err");
		goto err_alloc_chrdev_region;
	}

	mem_dev = cdev_alloc();
	if (!mem_dev) {
		memdbg("alloc cdev err");
		err = -ENOMEM;
		goto err_alloc_cdev;
	}

	cdev_init(mem_dev, &memdev_fops);
	mem_dev->owner = THIS_MODULE;
	err = cdev_add(mem_dev, mem_devnum, ARRAY_SIZE(mem_dev_list));
	if (err < 0) {
		memdbg("add cdev err");
		goto err_alloc_cdev;
	}

	memdbg("mem dev init ok");
	return 0;

err_alloc_cdev:
	unregister_chrdev_region(mem_devnum, ARRAY_SIZE(mem_dev_list));
err_alloc_chrdev_region:
	return err;
}

static void __exit exit(void)
{
	unregister_chrdev_region(mem_devnum, ARRAY_SIZE(mem_dev_list));
	if (mem_dev)
		cdev_del(mem_dev);	
	memdbg("mem dev removed!");
}

MODULE_LICENSE("GPL");

module_init(init);
module_exit(exit);
