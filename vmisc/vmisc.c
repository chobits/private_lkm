#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/input.h>
#include <linux/platform_device.h>

static struct platform_device *vms_dev;

static struct input_dev *vms_input_dev;

static ssize_t write_vms(struct device *dev, struct device_attribute *attr,
			const char *buf, ssize_t count)
{
	int x,y;
	sscanf(buf, "%d %d 0", &x,&y);
	input_report_rel(vms_input_dev, REL_X, x);
	input_report_rel(vms_input_dev, REL_Y, y);
	input_sync(vms_input_dev);
	return count;
}

DEVICE_ATTR(coordinates, 0644, NULL, write_vms);

static struct attribute *vms_attrs[] = {
	&dev_attr_coordinates.attr,
	NULL,
};

static struct attribute_group vms_attr_group = {
	.attrs = vms_attrs
};

int __init vmisc_init(void)
{
	vms_dev = platform_device_register_simple("vms", -1, NULL, 0);
	if (IS_ERR(vms_dev)) {
		printk(KERN_ALERT "platform err\n");
		return -1;
	}
	
	sysfs_create_group(&vms_dev->dev.kobj, &vms_attr_group);

	vms_input_dev = input_allocate_device();
	if (vms_input_dev == NULL) {
		printk(KERN_ALERT "input alloc err\n");
		return -1;
	}

	vms_input_dev->name = "Virtual Mouse";
	set_bit(EV_REL, vms_input_dev->evbit);
	set_bit(REL_X, vms_input_dev->relbit);
	set_bit(REL_Y, vms_input_dev->relbit);

	input_register_device(vms_input_dev);
	printk(KERN_ALERT"INIT OK\n");
	return 0;
	
}

void __exit vmisc_cleanup(void)
{
	input_unregister_device(vms_input_dev);
	sysfs_remove_group(&vms_dev->dev.kobj, &vms_attr_group);
	platform_device_unregister(vms_dev);
//	input_free_device(vms_input_dev);
	printk(KERN_ALERT "EXIT OK\n");
}


module_init(vmisc_init);
module_exit(vmisc_cleanup);
MODULE_LICENSE("GPL");

