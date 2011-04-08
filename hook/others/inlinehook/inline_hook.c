#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");
unsigned int clearcr0(void)
{
	unsigned int cr0 = 0;
	unsigned int ret;
	asm volatile ("movl %%cr0, %%eax"
			: "=a"(cr0));
	ret = cr0;
	
	cr0 &= 0xfffeffff;
	
	asm volatile ("movl %%eax, %%cr0"
			:: "a"(cr0));
	return ret;

}

void resumecr0(unsigned int value)
{
	asm volatile ("movl %%eax, %%cr0"
			:: "a"(value));
}

ssize_t (*old_vfs_read)(struct file *file, char __user *buf, size_t count,
	loff_t *pos);

ssize_t new_vfs_read (struct file *file, char __user *buf, size_t count,
	loff_t *pos)
{
	printk(KERN_ALERT "*");
	return old_vfs_read(file, buf, count, pos);
}

static unsigned orig_cr0;
unsigned int ih(unsigned int start, unsigned int old, unsigned int new)
{
	unsigned char *p;
	unsigned int *offset, save;
	int i;

	printk(KERN_ALERT "search form 0x%x\n", start);
	for (p = (unsigned char *)start, i = 0; i < 512; p++, i++) {
		if (p[0] == 0xe8) {
			offset = (unsigned int *)&p[1];
			if (*offset + 5 + (unsigned int)p == old) {
				printk(KERN_ALERT "find orig offset\n");
				save = (unsigned int)offset;

				orig_cr0 = clearcr0();
				*offset = new - (unsigned int)p - 5;
				resumecr0(orig_cr0);

				return save;
			}	
		}
	}
	return 0;
}
static unsigned int offset, start;
static int __init inline_init(void)
{
	start = 0xc04d2099;			//sys_read
	old_vfs_read = (void *)0xc04d1f5a;	//vfs_read
	offset = ih(start, (unsigned int)old_vfs_read,
				(unsigned int)new_vfs_read);
	if (offset)
		return 0;

	return -1;
}

static void __exit inline_exit(void)
{
/*	unsigned int orig_cr0;
	if (offset) {
		orig_cr0 = clearcr0();
		*(unsigned int *)offset = \
				(unsigned int)old_vfs_read - (offset - 1 + 5);
		resumecr0(orig_cr0);
	}*/
	ih(start, (unsigned int)new_vfs_read, (unsigned int)old_vfs_read);
	
}

module_init(inline_init);
module_exit(inline_exit);
