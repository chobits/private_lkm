#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

static unsigned char pool[1024];
static int pool_r;
static int pool_w;
static int w = 1;
static int r = 0;
int pread(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	unsigned int room, first_room;

	room = r ? 1024:(pool_w - pool_r) & 0x3ff;
	first_room = (room > 1024 - pool_r) ? (1024 - pool_r) : room;
	room -= first_room;

	/* snprintf will add null to page, and we will alloc 1 size for it */
	pool[pool_r + first_room] = 0;
/* FIXME: the pool_r is changed strangely!!!!!!! */
//printk(KERN_ALERT "1pool_r :%d\n", pool_r);
	count = snprintf(page, first_room + 1, &pool[pool_r]);
//printk(KERN_ALERT "2pool_r :%d\n", pool_r);
	if (room) {
		pool[0 + room] = 0;
		count += snprintf(page + count, room + 1, &pool[0]);
	}

	pool_r = (pool_r + count) & 0x3ff;
	if (pool_r == pool_w)
		w = 1;
	r = 0;
	return count;
}

int pwrite(struct file *file, const char __user *buf,
		unsigned long count, void *data)
{
	unsigned int room, first_room;

	room = w ? 1024:(pool_r - pool_w)&0x3ff;
	
	if (room < count)
		return -ENOSPC;	
	first_room = (count > (1024 - pool_w)) ? (1024 - pool_w):count;
	count -= first_room;
	if (copy_from_user(&pool[pool_w], buf, first_room))
		return -EFAULT;
	if (room)
		if (copy_from_user(&pool[0], buf + first_room, count))
			return -EFAULT;
		
	pool_w = (pool_w + count + first_room) & 0x3ff;
	if (pool_w == pool_r)
		r = 1;
	w = 0;
//	printk(KERN_ALERT "-w---count:%d first_room:%d--\n", count, first_room);
//	printk(KERN_ALERT "-w-pool_r:%d pool_w:%d----\n", pool_r, pool_w);
	return count + first_room;	
}

int __init hello_init(void)
{
	struct proc_dir_entry *dproc;
/*
 *	 dproc = proc_create("kk", 0644, NULL, NULL);
 */
	dproc = create_proc_entry("kk", 0644, NULL);	
	if (!dproc)
		return -1;
	dproc->read_proc = pread;
	dproc->write_proc = pwrite;
	
	memset(pool, 0x0, 1024);
	pool_r = pool_w = 0;
	printk(KERN_ALERT "hello kernel\n");
	return 0;
}

void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");
	remove_proc_entry("kk", NULL);
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

