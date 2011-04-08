#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <net/protocol.h>
#include <linux/mm.h>
#include <asm/highmem.h> /* kmap */
#include <asm/fixmap.h> /*pkmap_page_table*/
#include <asm/pgtable_32.h>

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

static int __init hello_init(void)
{
	//orig = clearcr0();
	//resumecr0(orig);
	unsigned int cr3;
	unsigned int *p,**q;
	int i;
	cr3 = 0;
	asm volatile ("movl %%cr3, %%eax\n":"=a"(cr3));
	printk(KERN_ALERT "cr3:0x%x\n", cr3);
	//pa -> va
	//kernel can only use va
	p = (unsigned int *)(cr3 + 0xc0000000);
	q = (unsigned int **)0xc0a64000;
	printk(KERN_ALERT "%x ?= %x\n", (unsigned int)p, (unsigned int)*q);
	for (i = 0; i < 10; i ++) {
		printk(KERN_ALERT "cr3[%d]:0x%x swapper_pg_dir[%d]:0x%x\n", 
						i, p[i], i,
			((unsigned int*)((unsigned int)q[0] + 0xc0000000))[i]);
	}
	printk(KERN_ALERT "*pkmap_page_table: 0x%x\n", 
					*(unsigned int *)0xc0b067c0);
	
	printk(KERN_ALERT "Highmemory: 0x%x\n", 
					(unsigned int)high_memory);

	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
