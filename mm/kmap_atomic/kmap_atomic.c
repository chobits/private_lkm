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

static pte_t **pkmap_pte = (pte_t **)0xc0a78000;

/*
 * only used in single cpu
 */
int none_pte(void)
{
	int i;
	pte_t *p = (pte_t *)*pkmap_pte;
	printk(KERN_ALERT "cpu:%d\n", smp_processor_id());
	for (i = 0; i < KM_TYPE_NR; i++)
		if (pte_none(*(p-i)))
			break;

	if (i == KM_TYPE_NR)
		i = -1;
	else 
		printk(KERN_ALERT "find none pte.type:%d\n", i);

	return i;	
}

unsigned int highmem_page(void)
{
	unsigned int i;
	unsigned long vaddr;

	for (i = 0; i < max_mapnr; i++) {
		if (PageHighMem(mem_map + i)) {	
			vaddr = (unsigned long)page_address(mem_map + i);
			if (!vaddr)
				break;
		}
	}

	if (i != max_mapnr)
		printk(KERN_ALERT "Find high page: %d == 0x%x\n", i, i);
	else 
		printk(KERN_ALERT "Not Found\n");
	
	return i;
}

static void *vaddr;
static void *vaddr2;
static int k;
static int __init hello_init(void)
{
	unsigned int i;


	if ((i = highmem_page()) == max_mapnr)
		return -1;
	if ((k = none_pte()) == -1)
		return -1;

	/*1st*/
	vaddr = kmap_atomic(mem_map + i, k);
	printk(KERN_ALERT "[1]PA:0x%x\n", i * 4096);
	if (vaddr != NULL)
		printk(KERN_ALERT "[1]kmap_atomic:0x%x\n", (unsigned int)vaddr);
	printk(KERN_ALERT "[1]%s\n",
		pte_none(*((*pkmap_pte) - k))?"none":"value");

	/*2nd:BUG pte_none(*(kmap_pte-k)) == 0 */
	/*
	vaddr2 = kmap_atomic(mem_map + i, k);
	printk(KERN_ALERT "[2]PA:0x%x\n", i * 4096);
	if (vaddr != NULL)
		printk(KERN_ALERT "[2]kmap_atomic:0x%x\n",(unsigned int)vaddr2);
	printk(KERN_ALERT "[2]%s\n",
		pte_none(*((*pkmap_pte) - k))?"none":"value");
	*/

	
	kunmap_atomic(vaddr, k);	
	printk(KERN_ALERT "kunmap[1]\n");
/*
	kunmap_atomic(vaddr, k);	
	printk(KERN_ALERT "kunmap[2]\n");
*/

	printk(KERN_ALERT "%s\n",
		pte_none(*((*pkmap_pte) - k))?"none":"value");
	printk(KERN_ALERT "FIX_KMAP_BEGIN :0x%x\n", FIX_KMAP_BEGIN);
	printk(KERN_ALERT "FIXADDR_TOP: 0x%lx\n", FIXADDR_TOP);
	printk(KERN_ALERT "FIXADDR_USER_START: 0x%lx\n", FIXADDR_USER_START);
	printk(KERN_ALERT "FIXADDR_USER_END: 0x%lx\n", FIXADDR_USER_END);

	return 0;
}

static void __exit hello_exit(void)
{
/*	FIXME:PANIC!!!!	
 *	printk(KERN_ALERT "0x%x %d", (unsigned int)vaddr, k);
 

	kunmap_atomic(vaddr, k);	
	printk(KERN_ALERT "%s\n",
		pte_none(*((*pkmap_pte) - k))?"none":"value");
*/
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
