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

asmlinkage int new_syscall(int a)
{
	printk(KERN_ALERT "test\n");
	return a;
}

static int search_code(unsigned int start, int slen, unsigned char *code,
						int len, unsigned int *retaddr)
{
	unsigned char *memaddr, *p;
	int i, j;

	memaddr = (unsigned char *)start;
	p = code;

	for (i = 0; i < slen - len + 1; i++, memaddr++) {
		for (j = 0; j < len && memaddr[j] == p[j]; j++)
			;
		if (j == len)
			break;
	}

	if (i == slen - len + 1) {
		*retaddr = 0;
		return -1;
	}
	else {
		*retaddr = (unsigned int)memaddr;
		return 0;
	}
}

/* 
 * clear WP bit of CR0, and return the original value
 */
unsigned int setcr0(void)
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
void setbackcr0(unsigned int val)
{
	asm volatile ("movl %%eax, %%cr0"
			:
			: "a"(val)
		     );
}

struct idt_desc {
        unsigned int low;
        unsigned int high;
} __attribute__ ((packed));


static unsigned long **q;
static unsigned int cr0;
static unsigned int old_nrcall;
static unsigned long old_338;
static unsigned char *old_nrcall_addr;

int hook_system_call(void)
{
	unsigned int system_call;
	unsigned int syscall_table;
	struct idt_desc *idt_table;
	unsigned int addr;
	unsigned char *p;
	unsigned char code[5] = "\x3d\x51\x01\x00\x00";
	

	/* set new addr */
	addr = (unsigned long)new_syscall;
	idt_table = (struct idt_desc *)0xc096f000;
	system_call = 
		(unsigned int) ((idt_table[0x80].low & 0xffff) |
				 (idt_table[0x80].high & 0xffff0000));
//	system_call = 0xc040340d;
	printk(KERN_ALERT "system_call %x\n", system_call);	

	/* search code cmp*/
	if (search_code(system_call, 100, code, 5, &addr) == 0)
		printk(KERN_ALERT "code at %x\n", addr);
	else {
		printk(KERN_ALERT "cannot find code\n");
		return -1;
	}

	old_nrcall_addr = p = (unsigned char *)addr;
	old_nrcall = *(unsigned int *)(p + 1);
	printk(KERN_ALERT "old_nrcall: %d\n", old_nrcall);
	if (old_nrcall != 337)
		return -1;

	cr0 = setcr0();
	*(unsigned int *)(p + 1) = 1;
	setbackcr0(cr0);
	printk(KERN_ALERT "old syscall number:%d\n", old_nrcall);
	printk(KERN_ALERT "new syscall number:339\n");

	/* search system_call */
	code[0] = 0xff;
	code[1] = 0x14;
	code[2] = 0x85;
	if (search_code(system_call, 100, code, 3, &addr) == 0)
		printk(KERN_ALERT "code at %x\n", addr);
	else {
		printk(KERN_ALERT "cannot find syscall_table\n");
		return -1;
	}
	
	p = (unsigned char *)addr;
	syscall_table = *(unsigned int *)(p + 3);
	printk(KERN_ALERT "syscall_table %x\n", syscall_table);

	q = (unsigned long **)syscall_table;
	cr0 = setcr0();
	old_338 = (unsigned long)(q[338]);
	q[338] = (unsigned long *)new_syscall;
	setbackcr0(cr0);

	return 0;
}


static int __init init(void)
{
	return hook_system_call();	
}

static void __exit exit(void)
{
	cr0 = setcr0();
	*(unsigned int *)(old_nrcall_addr + 1)= old_nrcall;
	q[338] = (unsigned long *)old_338;
	setbackcr0(cr0);
	printk(KERN_ALERT "goodbye\n");
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
