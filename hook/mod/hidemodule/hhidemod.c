#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
static int flag;
static int __init hello_init(void)
{
	list_del(&__this_module.list);
//	list_del_rcu(&__this_module.list);
	/*__this_module.list.prev->next = __this_module.list.next;
	__this_module.list.next->prev = __this_module.list.prev;
	__this_module.list.prev = LIST_POISON1;
	__this_module.list.prev = LIST_POISON2;*/
	printk(KERN_ALERT "flag: 0x%x\n", (unsigned int)&flag);
	
	return 0;
}

static void __exit hello_exit(void)
{
}

module_init(hello_init);
module_exit(hello_exit);
