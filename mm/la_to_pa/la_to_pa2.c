#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* everything\ldots{} */
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <asm/page.h>
static int pid;
static unsigned int la;

module_param(pid,int,0644);
module_param(la,uint,0644);
struct task_struct *(*find_task_by_pid_n)(pid_t,struct pid_namespace *) = \
									  (struct task_struct *(*)(pid_t,struct pid_namespace *))0xc04588d1;


/* 
 *  * clear WP bit of CR0, and return the original value
 *   */
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
 *  *
 *   * @val : new value to set in cr0
 *    */
void setback_cr0(unsigned int val)
{
        asm volatile ("movl %%eax, %%cr0"
                        :
                        : "a"(val)
                     );
}
static unsigned int old_cr0;
static int __init init(void)
{
	pgd_t *tmp_pgd;
	pud_t *tmp_pud;
	pmd_t *tmp_pmd;
	pte_t *tmp_pte;
	unsigned int pa;
	struct task_struct *tmp;
	
	old_cr0 = clear_and_return_cr0();
	if (!(tmp = find_task_by_pid_n(pid,&init_pid_ns))) {
		printk(KERN_ALERT"cannot find task:%d\n",pid);
		goto exit;
	}
	printk(KERN_ALERT"pgd = 0x%p\n",tmp->mm->pgd);

	if (!find_vma(tmp->mm,la)) {
		printk(KERN_ALERT"va is unvalid\n");
		goto exit;
	}	

	tmp_pgd = pgd_offset(tmp->mm, la);
	if (pgd_none(*tmp_pgd)) {
		printk("not mapped int pgd\n");
		goto exit;
	}
	tmp_pud = pud_offset(tmp_pgd,la);
	if (pud_none(*tmp_pud)) {
		printk(KERN_ALERT"not mapped in pud\n");
	}
	tmp_pmd = pmd_offset(tmp_pud,la);
	if (pmd_none(*tmp_pmd)) {
		printk(KERN_ALERT"not mapped in pmd\n");
		goto exit;
	}

	tmp_pte = pte_offset_kernel(tmp_pmd,la);
	if (pte_none(*tmp_pte)) {
		printk(KERN_ALERT"not mapped in pte\n");
		goto exit;
	}
	if (!pte_present(*tmp_pte)) {
		printk(KERN_ALERT"pte not in RAM\n");
		goto exit;
	}
	pa = ((pte_val(*tmp_pte) & PAGE_MASK) | (la & (~PAGE_MASK)));
	printk(KERN_ALERT"la:0x%x-->pa:0x%x\n",la,pa);
	setback_cr0(old_cr0);	
	return 0;
exit:
	setback_cr0(old_cr0);
	return -1;
}


static void __exit kexit(void)
{
	printk(KERN_ALERT"k:exit!\n");
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(kexit);
