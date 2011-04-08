#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/types.h>
#include <asm/io.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");

/* 
 * You'd better not to use this kind method of registering char dev
 * Please try New method!
 * Here, I just want to do it easy!
 */
#define DEVMEM_MAJOR 123
#define DEVMEM_NAME "devmem"

static inline int valid_phys_addr_range(unsigned long addr, size_t count)
{
	if (addr + count > __pa(high_memory))
		return 0;
	return 1;
}

unsigned int size_inside_page(unsigned int start, unsigned int count)
{
	unsigned int sz;
	sz = 4096 - (start & 0xfff);
	return min(sz, count);	
}

void put_ptr(unsigned long vs, void *addr)
{
	unsigned long start;
	start = vs & 0xfffff000;

	if (page_is_ram(start >> 12))
		return ;

	iounmap((void __iomem *)((unsigned long)addr & 0xfffff000));	
	return;
}

void *get_ptr(unsigned long vs)
{
	void *addr;
	unsigned long start;
	start = vs & 0xfffff000;
	if (page_is_ram(start >> 12))
		return __va(vs);
	addr = (void __force *)ioremap_cache(start, 4096);
	if (addr)
		addr = (void *)((unsigned long)addr | (vs & 0xfff));
	return addr;
	
}

static ssize_t 
write_devmem(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	/*
	 * read:	all read data
	 * sz:  	one time read data
	 * */
	size_t write, sz;
	unsigned long start, remaining;
	char *p;	
	start = *ppos;

	if (!valid_phys_addr_range(start, count))
		return -EFAULT;

	remaining = 0;
	write = 0;
	
	while (count > 0) {
		sz = size_inside_page(start, count);

		p = get_ptr(start);	
		if (!p) {
			printk(KERN_ALERT "cannot read\n");
			return -1;
		}
		remaining = copy_from_user(p, buf, (unsigned long)sz);	
		put_ptr(start, p);	
		if (remaining) {
			printk(KERN_ALERT "cannot copy\n");
			return -1;
		}

		start += sz;
		buf += sz;
		write += sz;	
		count -= sz;
	}

	*ppos += write;
	return write;
}


static ssize_t 
read_devmem(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	/*
	 * read:	all read data
	 * sz:  	one time read data
	 */
	size_t read, sz;
	unsigned long start, remaining;
	char *p;	

	start = *ppos;
	if (!valid_phys_addr_range(start, count))
		return -EFAULT;

	remaining = 0;
	read = 0;
	
	while (count > 0) {
		sz = size_inside_page(start, count);
		p = get_ptr(start);
		if (!p) {
			printk(KERN_ALERT "cannot read\n");
			return -1;
		}
		remaining = copy_to_user(buf, p, (unsigned long)sz);	
		put_ptr(start, p);	
		if (remaining) {
			printk(KERN_ALERT "cannot copy\n");
			return -1;
		}

		start += sz;
		buf += sz;
		read += sz;	
		count -= sz;
	}

	*ppos += read;
	return read;
}

static loff_t llseek_devmem(struct file *file, loff_t pos, int whence)
{
	loff_t off;

	off = pos;
	mutex_lock(&file->f_path.dentry->d_inode->i_mutex);
	switch (whence) {
	case SEEK_CUR:
		off += file->f_pos;	
	case SEEK_SET:
		if ((unsigned long long)off >= ~0xFFFULL) {
			off = -EOVERFLOW;
			break;
		}
		file->f_pos = off;
		break;
	case SEEK_END:
		off = -1;
		break;
	default:
		off = -1;
		break;
	}	
	mutex_unlock(&file->f_path.dentry->d_inode->i_mutex);
	return off;
}

static int open_devmem(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations devmem_fops = {
	.llseek	= llseek_devmem,
	.read	= read_devmem,
	.write	= write_devmem,
	.open	= open_devmem,
};

static int __init hello_init(void)
{
	if (register_chrdev(DEVMEM_MAJOR, DEVMEM_NAME, &devmem_fops))
		printk(KERN_ALERT "unable to get major %d for memory devs\n",
								DEVMEM_MAJOR);
	printk(KERN_ALERT "Devmem char init ok!\n");
	return 0;
}

static void __exit hello_exit(void)
{
	unregister_chrdev(DEVMEM_MAJOR, DEVMEM_NAME);
	printk(KERN_ALERT "exit!\n");
}

module_init(hello_init);
module_exit(hello_exit);
