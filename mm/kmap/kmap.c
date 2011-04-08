#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <asm/highmem.h> /* kmap */
#include <asm/pgtable_32_types.h>

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

static pte_t **ppkmap_page_table = (pte_t **)0xc0b067c0;
static pte_t **pkmap_pte = (pte_t **)0xc0a78000;
static struct page *pp;

static int __init hello_init(void)
{
	unsigned int i;
	unsigned long vaddr;
	void *ret;
	int index;
	unsigned long rett;
	unsigned int cr3;
	unsigned int *p, *q;

	cr3 = 0;
	asm volatile ("movl %%cr3, %%eax\n":"=a"(cr3));

	p = (unsigned int *)(cr3 + 0xc0000000);
	q = (unsigned int *)0xc0a64000;
	printk(KERN_ALERT "cr3:0x%x cr3[0]:0x%x swapper_pg_dir:0x%x\n", 
							cr3, p[0], q[0]);
	printk(KERN_ALERT "---------\n");
	printk(KERN_ALERT "cr3[%lu]:0x%x\n",
			PKMAP_BASE >> 22, p[PKMAP_BASE >> 22]);
	printk(KERN_ALERT "swapper_pg_dir[%lu]:0x%x\n",
			PKMAP_BASE >> 22, q[PKMAP_BASE >> 22]);

	printk(KERN_ALERT "pkmap_page_table:0x%x" \
			"[0]:0x%x\n",
				(unsigned int)*ppkmap_page_table,
				*(unsigned int *)*ppkmap_page_table);
	printk(KERN_ALERT "pkmap_pte:0x%x\n", (unsigned int)*pkmap_pte);
				
	printk(KERN_ALERT "---------\n");
	
	printk(KERN_ALERT "Highmemory: 0x%x\n", 
					(unsigned int)high_memory);

	printk(KERN_ALERT "PKMAP_BASE:0x%lx\n", PKMAP_BASE);

	for (i = 0; i < max_mapnr; i++) {
		if (PageHighMem(mem_map + i)) {	
			vaddr = (unsigned long)page_address(mem_map + i);
			if (!vaddr)
				break;
		}
	}
	if (i != max_mapnr)
		printk(KERN_ALERT "Find high page: %d == 0x%x\n", i, i);
	else {
		printk(KERN_ALERT "Not Found\n");
		return -1;
	}
	ret = kmap(mem_map + i);
	printk(KERN_ALERT "PA:0x%x\n", i * 4096);
	pp = mem_map + i;
	if (ret != NULL)
		printk(KERN_ALERT "kmap:0x%x\n", (unsigned int)ret);
	//aggregate value used where an integer was expected
	//struct / union cannot be exchanged to value
	index = ((unsigned int)ret - PKMAP_BASE) / 4096;
	rett = pte_val(ppkmap_page_table[0][index]);
	printk(KERN_ALERT "pte[%d]:0x%lx\n", index, rett);
	printk(KERN_ALERT "\n");

	return 0;
}

static void __exit hello_exit(void)
{
	kunmap(pp);	
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
