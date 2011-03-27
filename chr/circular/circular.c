/*
 * for linux 2.6.36
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define dbg(fmt, arg...) printk(KERN_ALERT"%s "fmt"\n", __FUNCTION__, ##arg)

#define DEBUG 0
#if DEBUG
#define ddbg(fmt, arg...) printk(KERN_ALERT"%s "fmt"\n", __FUNCTION__, ##arg)
#else
#define ddbg(fmt, arg...)
#endif

#define MAX_CIRCULAR_SIZE ((1 << 16) - 1)
struct circular_buf {
	int front;		/* head pointer */
	int rail;		/* tail pointer */
	int free;		/* free char size */
	int capacity;		/* total char size */
	char *buf;		/* circular buf pointer */
};

static struct circular_buf *cirbuf;

struct circular_buf *alloc_circular_buf(int size)
{
	struct circular_buf *cbuf;
	
	if (size > MAX_CIRCULAR_SIZE) {
		dbg("allocing circular buf oversize");
		return NULL;
	}
	
	cbuf = (struct circular_buf *)kzalloc(GFP_KERNEL, sizeof(*cbuf));
	if (!cbuf) {
		dbg("cannot alloc memory for circular buf structure");
		return NULL;
	}

	/* the tail char is for empty rail point */
	cbuf->buf = (char *)kzalloc(GFP_KERNEL, size + 1);
	if (!cbuf->buf) {
		dbg("cannot alloc memory for circular buf structure");
		kfree(cbuf);
		return NULL;
	}

	cbuf->free = size;
	cbuf->capacity = size;
	cbuf->front = 0;
	cbuf->rail = 0;

	return cbuf;	
}

void free_circular_buf(struct circular_buf *cbuf)
{
	/* kfree will hande NULL pointer */
	kfree(cbuf->buf);
	kfree(cbuf);
}

/*
static loff_t circular_llseek(struct file *filp, loff_t off, int orign)
{
	return -1;
}
*/

static ssize_t circular_read(struct file *filp, char __user *buf,
			size_t count, loff_t *pos)
{
	int size, ret;
	size = cirbuf->capacity - cirbuf->free;

	if (size > count)
		size = count;

	if (!size)
		return 0;
	ret = size;
	if (size + cirbuf->front > cirbuf->capacity) {
		if (copy_to_user(buf, cirbuf->buf + cirbuf->front, 
					cirbuf->capacity - cirbuf->front))
			return -EFAULT;
		size -= cirbuf->capacity - cirbuf->front;
		cirbuf->free += cirbuf->capacity - cirbuf->front;
		cirbuf->front = 0;
	}
	
	if (copy_to_user(buf + ret - size, cirbuf->buf + cirbuf->front, size))
		return -EFAULT;

	cirbuf->front = (cirbuf->front + size) % cirbuf->capacity;
	cirbuf->free += size;
	ddbg("\n\n[r - %d - f: %d r:%d free:%d]\n\n",
			ret, cirbuf->front, cirbuf->rail, cirbuf->free);
	return ret;
}

static ssize_t circular_write(struct file *filp, char __user *buf,
			size_t count, loff_t *pos)
{
	int size, ret;
	size = cirbuf->free;
	if (size > count)
		size = count;
	if (!size)
		return -ENOSPC;

	ret = size;
	if (size + cirbuf->rail > cirbuf->capacity) {
		if (copy_from_user(cirbuf->buf + cirbuf->rail, buf,
					cirbuf->capacity - cirbuf->rail))
			return -EFAULT;
		cirbuf->free -= cirbuf->capacity - cirbuf->rail;
		size -= cirbuf->capacity - cirbuf->rail;
		cirbuf->rail = 0;
	}
	
	if (copy_from_user(cirbuf->buf + cirbuf->rail, buf + ret - size, size))
		return -EFAULT;

	cirbuf->free -= size;
	cirbuf->rail = (cirbuf->rail + size) % cirbuf->capacity;
	ddbg("\n\n[w - %d - f: %d r:%d free:%d]\n\n",
			ret, cirbuf->front, cirbuf->rail, cirbuf->free);

	return ret;
}

static struct file_operations circular_ops = {
	.read = circular_read,
	.write = circular_write,
//	.llseek = circular_llseek,
};

#define CHAR_NAME "circular"
#define CHAR_BASEMINOR 0
#define CHAR_COUNT 1
static int circular_major;

static int __init cir_init(void)
{

	/* alloc major, baseminor = 0, minor counts = 255 */
	circular_major = __register_chrdev(0, CHAR_BASEMINOR, CHAR_COUNT,
						CHAR_NAME, &circular_ops);
	if (circular_major < 0) {
		dbg("register char device err\n");
		return circular_major;
	}

	cirbuf = alloc_circular_buf(4096);
	if (!cirbuf) {
		dbg("register char device err\n");
		__unregister_chrdev(circular_major, CHAR_BASEMINOR, CHAR_COUNT,
								CHAR_NAME);
		return -ENOMEM;
	}

	printk(KERN_ALERT "init ok\n");

	return 0;
}

static void __exit cir_exit(void)
{
	free_circular_buf(cirbuf);
	__unregister_chrdev(circular_major, CHAR_BASEMINOR, CHAR_COUNT,
								CHAR_NAME);
	printk(KERN_ALERT "goodbye kernel\n");
}

MODULE_LICENSE("GPL");
module_init(cir_init);
module_exit(cir_exit);
