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
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");

static int __init hello_init(void)
{
	struct vm_struct **pvmlist, *vl, **p, *tmp;
	pvmlist = (struct vm_struct **)0xc0b0d844;
	vl = *pvmlist;
	
	for (p = &vl; tmp = *p; p = &tmp->next) {
		printk(KERN_ALERT "start:%x size:%lx\n",
			(unsigned int)tmp->addr, tmp->size);
	}
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
