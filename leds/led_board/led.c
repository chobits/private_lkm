#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>

#define DEVICE_NAME     "ledddd"

static struct class *led_class;    /* Class to which this device belongs */
static struct kobject kobj;
static dev_t dev_number;

struct led_attr {
	struct attribute attr;
	ssize_t (*show)(char *);
	ssize_t (*store)(const char *, size_t count);
};

static ssize_t led_store(const char *buf, size_t b)
{
	printk(KERN_ALERT "%s|%d\n",buf,b);
	printk(KERN_ALERT "led store\n");
	return b;
}

static ssize_t led_show(char *buf)
{
	printk(KERN_ALERT "led show\n");
	return 0;
}

static ssize_t l_store(struct kobject *kobj,struct attribute *a,
					const char *user_buf, size_t b)
{
	struct led_attr *lattr = container_of(a, struct led_attr, attr);
	printk(KERN_ALERT "store\n");
	if (lattr) {
		lattr->store(user_buf, b);
	}
	return b;
}

static ssize_t l_show(struct kobject *kobj,struct attribute *a, char *buf)
{
	printk(KERN_ALERT "show\n");
	return 1;
}

static struct led_attr led_attr = __ATTR(llleddd, 0644, led_show, led_store);
static struct attribute *lp[] = {&led_attr.attr, NULL};

static struct sysfs_ops sysfs_ops = {
	.show = l_show,
	.store = l_store,
};

static struct kobj_type ktype_led = {
	.sysfs_ops = &sysfs_ops,
	.default_attrs = lp,
};

int __init led_init(void)
{
	int ret;
	struct device *dp;
	if (alloc_chrdev_region (&dev_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_ALERT "Can’t register new device\n");
		return -1;
	}

	/* Create the led class */
	led_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(led_class))
		printk("Bad class create\n");

	dp = device_create(led_class, NULL, dev_number, NULL, DEVICE_NAME);

	ret = kobject_init_and_add(&kobj, &ktype_led, &dp->kobj, "sonnnn");
	if (ret) {
		printk(KERN_ALERT "ERR\n");
		return -1;
	}
	printk(KERN_ALERT "LED Driver Initialized.\n");
	return 0;
}

void __exit led_cleanup(void)
{
	unregister_chrdev_region(dev_number, 1);
	kobject_del(&kobj);
	device_destroy(led_class, dev_number);
	class_destroy(led_class);
	return;
}
module_init(led_init);
module_exit(led_cleanup);
MODULE_LICENSE("GPL");

