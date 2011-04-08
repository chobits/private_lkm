#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/notifier.h> /* used func or data structure */
#include <linux/reboot.h> /* register_reboot_notifier */

static int my_block_call(struct notifier_block *this, unsigned long val,
								void *p)
{
	
	printk(KERN_ALERT"************************************************\n");
	printk(KERN_ALERT"*[MY_b_CALL:%s]\n", (p == NULL) ? "<null>":(char *)p);
	/* %lu -- unsigned long
	 * other format to see `Documentation/printk-formats.txt`
	 * */
	printk(KERN_ALERT "*[event:%lu]\n", val);	
	printk(KERN_ALERT "*[priority:%d]\n", this->priority);	
	if (this->next == NULL) {
		printk(KERN_ALERT"-----------------------------------------\n");
		printk(KERN_ALERT "I'm the last one\n");
		while (1) ;
	}
	if (this->priority == 0) {
		printk(KERN_ALERT "priority is 0\n");
		while (1) ;
	}
	
	
	return NOTIFY_OK;

}

/* we should note here:
 *	.next/.priority is 0 defaultly!
 * */
static struct notifier_block my_block1 = {
	.notifier_call = my_block_call,
	.priority = 1,
};

static struct notifier_block my_block2 = {
	.notifier_call = my_block_call,
};

int __init notifier_init(void)
{
	printk(KERN_ALERT "#1.priority is %d defaultly\n",
						my_block1.priority);
	printk(KERN_ALERT "#2.priority is %d defaultly\n",
						my_block2.priority);
	if (register_reboot_notifier(&my_block1) == 0)
		printk(KERN_ALERT "#block 1 registered\n");
	if (register_reboot_notifier(&my_block2) == 0)
		printk(KERN_ALERT "#block 2 registered\n");
	
	return 0;
}
void __exit notifier_exit(void)
{
	if (unregister_reboot_notifier(&my_block1) == 0)
		printk(KERN_ALERT "[e]block 1 unregistered\n");
	if (unregister_reboot_notifier(&my_block2) == 0)
		printk(KERN_ALERT "[e]block 2 unregistered\n");
	unregister_reboot_notifier(&my_block2);
	printk(KERN_ALERT "[e]exit\n");
}

MODULE_LICENSE("GPL");
module_init(notifier_init);
module_exit(notifier_exit);

