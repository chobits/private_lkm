#include <linux/init.h>
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* everything\ldots{} */
#include <linux/jiffies.h>
#include <linux/types.h>
#include <asm/segment.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <linux/types.h>
#include <linux/dirent.h>
#include <linux/string.h>
#include <linux/syscalls.h>
unsigned long **sys_call_table;

struct idt_descr {
        unsigned short limit;
        unsigned int base;
} __attribute__ ((packed));

struct idt_desc {
        unsigned int low;
        unsigned int high;
} __attribute__ ((packed));

unsigned long **get_sys_call_table(void)
{
        struct idt_descr idt;
        struct idt_desc *idt_table;
        unsigned char *p;
        unsigned int ret;
        int i;

        __asm__ ("sidt %0\n\t":"=m"(idt)::);
        printk(KERN_ALERT "idt_table:0x%x\n", idt.base);

        idt_table =(struct idt_desc *)idt.base;
        p = (unsigned char *)((idt_table[0x80].low&0xffff) +
                        (idt_table[0x80].high&0xffff0000));
        printk(KERN_ALERT "system_call:%p\n",p);

        for(ret=0,i=0;  i<100; i++) {
                if(p[i]==0xff && p[i+1]==0x14 && p[i+2]==0x85)
                        ret = *(unsigned int *)(p+i+3);
        }
        if(ret) {
                printk(KERN_ALERT "sys_call_table:0x%x\n", ret);
                return (unsigned long **)ret;
        }
        return NULL;
}

/* 
 * clear WP bit of CR0, and return the original value
 */
unsigned int clear_and_return_cr0(void)
{
	unsigned int cr0 = 0;
	unsigned int ret;

	asm volatile ("movl %%cr0, %%eax"
			: "=a"(cr0)
		     );
	ret = cr0;

	/* clear the bit16 of CR0, a.k.a WP bit */
	cr0 &= 0xfffeffff;

	asm volatile ("movl %%eax, %%cr0"
			:
			: "a"(cr0)
		     );
	return ret;
}

/* set CR0 with new value
 *
 * @val : new value to set in cr0
 */
void setback_cr0(unsigned int val)
{
	asm volatile ("movl %%eax, %%cr0"
			:
			: "a"(val)
		     );
}

asmlinkage int (*old_mkdir)(const char __user *,int);

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

asmlinkage int new_mkdir(const char __user *pathname,int mode)
{
	int ret;
	print_time("hijack mkdir:");
	ret = (*old_mkdir)(pathname,mode);
	return ret;
}

static unsigned int old_cr0;

int __init init(void)
{
	sys_call_table = get_sys_call_table();
	if (	sys_call_table ==NULL ||
		sys_call_table[__NR_close]!=(unsigned long *)sys_close) {
		printk(KERN_ALERT"ERROR!\n");
		return -1;
	}
	old_mkdir = sys_call_table[__NR_mkdir];
	
	old_cr0 = clear_and_return_cr0();
	sys_call_table[__NR_mkdir] = new_mkdir;
	setback_cr0(old_cr0);
	
	return 0;
}

void __exit exit(void)
{
	old_cr0 = clear_and_return_cr0();
	sys_call_table[__NR_mkdir] = old_mkdir;
	setback_cr0(old_cr0);
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
