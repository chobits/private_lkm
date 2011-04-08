#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/highmem.h> /* kmap */
#include <asm/pgtable_32_types.h>
#include <linux/gfp.h>

MODULE_LICENSE("GPL");

static int __init hello_init(void)
{
	struct page *page;
	int i;
	page = alloc_pages(GFP_KERNEL, 3);
	if (!page) {
		printk(KERN_ALERT "no page\n");
		return -1;
	}
	__free_pages(page, 2);
	__free_pages(page, 2);

	__free_pages(page + 4, 1);
	__free_pages(page + 4 + 2, 1);
	for (i = 0; i < 8; i++)
		printk(KERN_ALERT "%lu\n", (page + i)->private);

	printk(KERN_ALERT "align--\n");
	page = alloc_pages(GFP_KERNEL, 3);
	__free_pages(page, 3);
	for (i = 0; i < 8; i++)
		printk(KERN_ALERT "%lu\n", (page + i)->private);
	
	printk(KERN_ALERT "init ok\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
