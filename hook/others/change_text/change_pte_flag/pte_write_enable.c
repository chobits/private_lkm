#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/mm.h>
#include <asm/fixmap.h>
#include <asm/processor.h>
#include <asm/tlbflush.h>
#include <asm/page.h>

unsigned int **tbl = (unsigned int **)0xc0798328 ;

asmlinkage int new_call(void)
{
	printk(KERN_ALERT "ooook\n");
	return 0;
}

unsigned int enable_write(unsigned int addr)
{
	unsigned int ps;
	unsigned int *pt;
	unsigned int *pte_base;
	unsigned int *pte;
	unsigned int ret;
	asm volatile ("movl %%cr3, %%eax":"=a"(ps):);

	pt = (unsigned int *)(0xc0000000 + (ps + ((addr >> 20) & 0xffc)));
	printk(KERN_ALERT "*pgd:0x%x\n", *pt);

	printk(KERN_ALERT "%x ?=%x\n",
		(unsigned int)pud_offset((pgd_t *)pt,addr), (unsigned int)pt);

	pte_base = (unsigned int *)(((*pt) & 0xfffff000) + 0xc0000000);
	pte = &pte_base[(addr >> 12) & 0x3ff];
	printk(KERN_ALERT "*pte:0x%x\n", *pte);

	ret = *pte;
	*pte = (*pte) | 2;
	__flush_tlb_one((unsigned long)addr);
	printk(KERN_ALERT "*pte:0x%x\n", *pte);
	
	return ret;	
}

void set_flag(unsigned int addr, unsigned int orig)
{
	unsigned int ps;
	unsigned int *pt;
	unsigned int *pte;
	unsigned int *pte_base;
	ps = *(unsigned int *)0xc0a64000;
	pt = (unsigned int *)(0xc0000000 + (ps + ((addr >> 20) & 0xffc)));
	pte_base = (unsigned int *)(((*pt) & 0xfffff000) + 0xc0000000);
	pte = &pte_base[(addr >> 12) & 0x3ff];
	*pte = orig;
	printk(KERN_ALERT "*pte:0x%x\n", *pte);
	__flush_tlb_one((unsigned long)addr);
}

/*
unsigned int enable_write(unsigned int addr)
{
	pgd_t *base = __va(read_cr3());
	pgd_t *pgd = &base[pgd_index(addr)];
	pmd_t *pmd;
	pte_t *pte;
	pmd = pmd_offset(pud_offset(pgd,addr), addr);
	printk(KERN_ALERT "*pde:0x%x\n", (unsigned int)pmd->pud.pgd.pgd);
	pte = pte_offset_kernel(pmd,addr);
	
	pte->pte_low = pte->pte_low | 2;
	printk(KERN_ALERT "*pte:0x%x\n", (unsigned int)pte->pte_low);
	local_flush_tlb();	
	sync_core();
	return 0;
}
*/

static int __init init(void)
{
	unsigned int addr;
	unsigned int flag;	
	addr = (unsigned int)&tbl[337];

	flag = enable_write(addr);	
	printk(KERN_ALERT"---------write before----------\n");
	tbl[337] = (unsigned int *)new_call;
	printk(KERN_ALERT"---------write after----------\n");
	set_flag(addr, flag);
	return 0;
}

static void __exit exit(void)
{
	printk(KERN_ALERT "bye!!\n");
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
