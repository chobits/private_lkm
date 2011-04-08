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

static int __init hello_init(void)
{
	printk(KERN_ALERT "VMALLOC_START :0x%lx\n", VMALLOC_START);
	printk(KERN_ALERT "VMALLOC_END :0x%lx\n", VMALLOC_END);
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
