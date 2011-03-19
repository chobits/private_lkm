#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/interrupt.h>
#include <linux/types.h>
#include <asm/segment.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/desc_defs.h>

struct desc_struct *idt_table = (struct desc_struct *)0xc096f000;
struct desc_struct old;



static int __init init(void)
{
	printk(KERN_ALERT "OK\n");
	old = idt_table[0];
	idt_table[0].base0 = 
	return 0;
}

static void __exit exit(void)
{
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wang xiao chen <wangxiaochen0@gmail.com>");

module_init(init);
module_exit(exit);
