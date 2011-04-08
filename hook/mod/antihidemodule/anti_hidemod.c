#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/notifier.h>

MODULE_LICENSE("GPL");
static int (*orig_module_init)(void);
static struct module *mod;
static struct list_head orig_list;

static int anti_hd_init(void)
{
	int ret;
	ret = orig_module_init();
	if (mod->list.prev != orig_list.prev ||
		orig_list.prev->next != &mod->list) {
		mod->list.prev = orig_list.prev;
		orig_list.prev->next = &mod->list;
		printk(KERN_ALERT "[ahd]mod:%s wants to hide"\
				" (lish->prev changed)\n", mod->name);
	}
	if (mod->list.next != orig_list.next ||
		orig_list.next->prev != &mod->list) {
		mod->list.next = orig_list.next;
		orig_list.next->prev = &mod->list;
		printk(KERN_ALERT "[ahd]mod:%s wants to hide"\
				" (lish->next changed)\n", mod->name);
	}
	return ret;
}

static int ahd_call(struct notifier_block *self, unsigned long val, void *pmod)
{
	if (val == MODULE_STATE_COMING) {
		/*
		 * the order is:
		 * 	load_module --> list_add
		 * 	block_notifier_call_chain --> ahd_call
		 * 	do_one_initcall -X-> mod->init (may hide via list_del)
		 * 	|		    ./  \.
		 * 	+-> mod->ahd_init -/      \--> ahd_init detect hide
		 *
		 */
		mod = (struct module *)pmod;
		orig_module_init = mod->init;
		mod->init = anti_hd_init;
		orig_list.prev = mod->list.prev;
		orig_list.next = mod->list.next;
		printk(KERN_ALERT "[ahd]anti_mod_hide start\n");
	}
	return NOTIFY_OK;
}
static struct notifier_block ahd_nb = {
	.notifier_call = ahd_call,
};
static int __init hello_init(void)
{
	return register_module_notifier(&ahd_nb);
}

static void __exit hello_exit(void)
{
	unregister_module_notifier(&ahd_nb);
}

module_init(hello_init);
module_exit(hello_exit);
