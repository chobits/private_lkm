#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/input.h>

	static int 
hook_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id)
{
	return 0;
}

static const struct input_device_id hook_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},

	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_SND) },
	},

	{ },    /* Terminating entry */
};

static struct input_handler hook_handler = {
	.fops = NULL,
	.name = "hook_ih",
	.connect = hook_connect,
	.blacklist = NULL,	
	.id_table = hook_ids,
};

static int __init hook_init(void)
{
	int error;
	error = input_register_handler(&hook_handler);
	if (error)
		return error;
	return 0;
}

static void __exit hook_cleanup(void)
{
	input_unregister_handler(&hook_handler);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wang xiao chen <wangxiaochen0@gmail.com>");

module_init(hook_init);
module_exit(hook_cleanup);

