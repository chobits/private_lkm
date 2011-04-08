#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/notifier.h> /* used func or data structure */
#include <linux/reboot.h> /* register_reboot_notifier */

BLOCKING_NOTIFIER_HEAD(my_notifier_list);
int register_my_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&my_notifier_list, nb);
}

int unregister_my_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&my_notifier_list, nb);
}

static int my_block_call(struct notifier_block *this, unsigned long val,
								void *p)
{
	
	printk(KERN_ALERT"*[MY_b_CALL:%s]\n", (p == NULL) ? "<null>":(char *)p);
	/* %lu -- unsigned long
	 * other format to see `Documentation/printk-formats.txt`
	 * */
	printk(KERN_ALERT "*[event:%lu]\n", val);	
	printk(KERN_ALERT "*[priority:%d]\n", this->priority);	
	if (this->next == NULL)
		printk(KERN_ALERT "* I'm last one\n");
	if (val == (unsigned long)10) {
		printk(KERN_ALERT "*[event:%lu stops!!]\n", val);	
		return NOTIFY_STOP;
	}
	printk(KERN_ALERT "\n");
	
	return NOTIFY_OK;

}

/* we should note here:
 *	.next/.priority is 0 defaultly!
 * */
static struct notifier_block my_block1 = {
	.notifier_call = my_block_call,
	.priority = 2,
};

static struct notifier_block my_block2 = {
	.notifier_call = my_block_call,
};

static struct notifier_block my_block3 = {
	.notifier_call = my_block_call,
};

int __init notifier_init(void)
{
	printk(KERN_ALERT "#1.priority is %d defaultly\n",
						my_block1.priority);
	printk(KERN_ALERT "#2.priority is %d defaultly\n",
						my_block2.priority);
	printk(KERN_ALERT "#3.priority is %d defaultly\n",
						my_block3.priority);
	if (register_my_notifier(&my_block1) == 0)
		printk(KERN_ALERT "#block 1 registered\n");
	if (register_my_notifier(&my_block3) == 0)
		printk(KERN_ALERT "#block 3 registered\n");
	if (register_my_notifier(&my_block2) == 0)
		printk(KERN_ALERT "#block 2 registered\n");
	blocking_notifier_call_chain(&my_notifier_list, 2, "in init");
	
	return 0;
}
void __exit notifier_exit(void)
{
	blocking_notifier_call_chain(&my_notifier_list, 10, "in exit");
	if (unregister_my_notifier(&my_block1) == 0)
		printk(KERN_ALERT "[e]block 1 unregistered\n");
	if (unregister_my_notifier(&my_block3) == 0)
		printk(KERN_ALERT "[e]block 3 unregistered\n");
	if (unregister_my_notifier(&my_block2) == 0)
		printk(KERN_ALERT "[e]block 2 unregistered\n");
	printk(KERN_ALERT "[e]exit\n");
}

MODULE_LICENSE("GPL");
module_init(notifier_init);
module_exit(notifier_exit);

