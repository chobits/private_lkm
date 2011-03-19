#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* everything\ldots{} */
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define BUF_SIZE 1024
#define MASK (BUF_SIZE - 1)
typedef struct buf_struct {
	int head;
	int tail;
	int full;
	char buf[BUF_SIZE];
} BS;

#define FREE(bsp) (((bsp)->head - (bsp)->tail) & MASK)
#define USED(bsp) (((bsp)->tail - (bsp)->head) & MASK)

static BS *bsp;

ssize_t 
cdev_write(struct file *file, const char __user *ubuf,
					 size_t count, loff_t *ppos)
{
	BS *bsp = (BS *)file->private_data;
	printk(KERN_ALERT "[w]\n");
	if (bsp == NULL)
		return -1;
	if (FREE(bsp) == 0 && bsp->full)
		return 0;
	if (count > FREE(bsp))
		count = FREE(bsp);

	if (bsp->tail > bsp->head && bsp->tail > (BUF_SIZE - count)) {
		copy_from_user((void *)(bsp->buf + bsp->tail),
					 (void *)ubuf, BUF_SIZE - bsp->tail);
		copy_from_user((void *)bsp->buf,
			 (void *)(ubuf + BUF_SIZE - bsp->tail), 
				count - (BUF_SIZE - bsp->tail));
		bsp->tail = (count - (BUF_SIZE - bsp->tail)) & MASK;
	}
	else {
		copy_from_user((void *)(bsp->buf + bsp->tail),
						(void *)ubuf, count);
		bsp->tail = (bsp->tail + count) & MASK;
	}
	return count;	

}

ssize_t 
cdev_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	BS *bsp = (BS *)file->private_data;
	printk(KERN_ALERT "[r]\n");
	if (bsp == NULL)
		return -1;

	if (USED(bsp) == 0)
		return 0;

	if (USED(bsp) < count)
		count = USED(bsp);

	if (bsp->tail < bsp->head && count > (BUF_SIZE - bsp->head)) {
		copy_to_user((void *)ubuf, (void *)(bsp->buf + bsp->head),
				BUF_SIZE - bsp->head);
		
		copy_to_user((void *)(ubuf + BUF_SIZE - bsp->head),
			 (void *)bsp->buf, count - (BUF_SIZE - bsp->head));
		bsp->head = count - (BUF_SIZE - bsp->head);
	}
	else{
		copy_to_user((void *)ubuf, (void *)(bsp->buf + bsp->head),
							 count);
		bsp->head = (bsp->head + count) & MASK;
	}	
	return count;
}

int cdev_open(struct inode *inode, struct file *file)
{
	printk(KERN_ALERT "[o]\n");
	file->private_data = (void *)bsp;
	return 0;
}

int cdev_close(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	printk(KERN_ALERT "[c]\n");
	return 0;
}

static dev_t cdev_num;
static struct cdev *cp;
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = cdev_open,
	.release = cdev_close,
	.read = cdev_read,
	.write = cdev_write,
//	.llseek = cdev_llseek,
};

static int __init cdev_start(void)
{
	int ret;
	ret = alloc_chrdev_region(&cdev_num, 0, 1, "char_dev");
	if (ret) {
		printk(KERN_ALERT "alloc_chrdev_region err\n");
		return ret;
	}
	
	cp = cdev_alloc();
	if (cp == NULL) {
		printk(KERN_ALERT "cdev_alloc err\n");
		return -1;
	}
	cdev_init(cp, &fops);
	cp->owner = THIS_MODULE;
	
	ret = cdev_add(cp, cdev_num, 1);
	if (ret) {
		printk(KERN_ALERT "cdev_add err\n");
		return ret;
	}

	bsp = (BS *)kmalloc(sizeof(BS), GFP_KERNEL);
	if (bsp == NULL) {
		printk(KERN_ALERT "kmalloc err\n");
		return -1;
	}
	bsp->head = 0;
	bsp->tail = 0;
	bsp->full = 0;
	printk(KERN_ALERT "init ok\n");
	return 0;
}

static void __exit cdev_exit(void)
{
	cdev_del(cp);	
	unregister_chrdev_region(cdev_num, 1);
	printk(KERN_ALERT "exit ok\n");
}

MODULE_LICENSE("GPL");

module_init(cdev_start);
module_exit(cdev_exit);
