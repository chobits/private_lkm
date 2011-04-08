#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");

asmlinkage long hook_337_syscall(void)
{
	/* I will change code */
	printk("wait changing");
	return 0;
}

unsigned int set_cr0(void)
{
	unsigned int cr0, ret;
	asm volatile ("movl %%cr0, %%eax":"=a"(ret):);
	cr0 = ret & 0xfffeffff;
	asm volatile ("movl %%eax, %%cr0"::"a"(cr0));
	return ret;
}

void set_back_cr0(unsigned int cr0)
{
	asm volatile ("movl %%eax, %%cr0"::"a"(cr0));
}

/*
 * It is fool! I need a better func.
 */
int text_len(void *start)
{
	int len;
	unsigned char *p;

	/*
	 * find:
	 * c9 leave
	 * c3 ret
	 */
	for (len = 0, p = start; !(p[len] == 0xc9 && p[len + 1] == 0xc3); len++)
		;

	/* from 0 count and 0xc3's index is len + 1*/
	return len + 2;
}


static int __init hidemod_init(void)
{
	unsigned long **sys_call_table;
	unsigned int cr0, call_offset;
	int len;
	char *page_vaddr, *p;
	char *str = "<1>The module has been removed!\nJust a memory leak\n";

	sys_call_table = (unsigned long **)0xc0798328;	

	/*
	 * I learn it from kernel func: sysenter_setup
	 * (The code of memory management in 2.6 is hard :0)
	 */ 
	page_vaddr = (char *)get_zeroed_page(GFP_ATOMIC);
	if (page_vaddr == NULL) {
		printk(KERN_ALERT "Unable get zero page \n");
		return -1;
	}

	/*copy string(printk()'s argument) to my page*/
	len = strlen(str);
	memcpy(page_vaddr, str, len);

	/*copy code to my page*/
	p = page_vaddr + len + 1;
	len = text_len(hook_337_syscall);
	memcpy(p, (void *)hook_337_syscall, len);


	/*
	 * change the offset 
	 * number 9/14 is counted from the assembled code
	 */
	call_offset = (unsigned int)printk - (unsigned int)(&p[13] + 5);
	*(unsigned int *)&p[9] = (unsigned int)page_vaddr;
	*(unsigned int *)&p[14] = call_offset;

	cr0 = set_cr0();
	sys_call_table[337] = (unsigned long *)p;
	set_back_cr0(cr0);

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
