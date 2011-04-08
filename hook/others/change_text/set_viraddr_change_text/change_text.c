#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/mm.h>
#include <asm/fixmap.h>
#include <asm/processor.h>
#include <asm/tlbflush.h>

unsigned long **tbl = (unsigned long **)0xc0798328 ;

asmlinkage int new_call(void)
{
	printk(KERN_ALERT "ooook\n");
	return 0;
}

static int __init init(void)
{
	struct page *page;
	char *vaddr;
	unsigned long addr, saddr;
	addr = (unsigned long)&tbl[337];
	saddr = (unsigned long)new_call;
	printk(KERN_ALERT "0x%lx ?= 0x%lx\n", *(unsigned long *)addr, saddr);
	
	page = vmalloc_to_page((void *)addr);
	if (page == NULL) {
		printk(KERN_ALERT "no page\n");
		return -1;
	}

	set_fixmap(FIX_TEXT_POKE0, (unsigned int)page_to_pfn(page) << 12);
	vaddr = (char *)fix_to_virt(FIX_TEXT_POKE0);
	printk(KERN_ALERT "fix_text_poke0 start :0x%p\n", vaddr);
	memcpy(&vaddr[addr & ~PAGE_MASK], (void *)&saddr, 4);
	clear_fixmap(FIX_TEXT_POKE0);
	
	local_flush_tlb();	
	
	
	sync_core();	
	printk(KERN_ALERT "0x%lx ?= 0x%lx\n", *(unsigned long *)addr, saddr);
	return 0;
}

static void __exit exit(void)
{
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
