#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include "tcmos.h"

struct cmos_struct *cmosp[NUM_CMOS_BANKS];
static dev_t cmos_dev_num;
struct class *cmos_class;

unsigned char addrports[NUM_CMOS_BANKS] = {CMOS_BANK0_ADDR_PORT,
					CMOS_BANK1_ADDR_PORT};
unsigned char dataports[NUM_CMOS_BANKS] = {CMOS_BANK0_DATA_PORT,
					CMOS_BANK1_DATA_PORT};

loff_t
cmos_llseek (struct file *file, loff_t offset, int orig)
{
	return -1;
}

unsigned char port_read(unsigned char offset, int bank)
{
	unsigned char data;
	if (bank > NUM_CMOS_BANKS) {
		printk(KERN_ALERT "cmos_read err\n");
		return 0;
	}
	outb(offset, addrports[bank]);
	data = inb(dataports[bank]);
	return data;
	
}

ssize_t 
cmos_read (struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned char data[CMOS_BANK_SIZE];
	int rbit,start_bit,start_byte;
	int l,i;
	unsigned char mask;
	struct cmos_struct *cp = file->private_data;
	start_byte = cp->current_pointer / 8;
	start_bit = cp->current_pointer % 8; 

	if (cp->current_pointer >= cp->size) {
		printk(KERN_ALERT "FULL\n");	
		return 0;
	}
	if (cp->current_pointer + count > cp->size) 
		count = cp->size - cp->current_pointer;

	i = 0;
	while (rbit < count) {
		data[i] = port_read(start_byte, cp->bank_num) >> start_bit;
		start_byte++;
		rbit += 8 - start_bit;
		if (start_bit > 0 && start_bit + count > 8) {
			data[i] |= 
			port_read(start_byte, cp->bank_num) << (8 - start_bit);
			rbit += start_bit;
		}
		i++;
	}	

	if (rbit > count) {
		mask = 1 << (8 - (rbit - count));	
		for (l = 0; l < rbit - count; l++) {
			data[i - 1] &= ~mask;
			mask <<= 1;
		}
	}

	rbit = count;
	if (rbit == 0) 
		return -EIO;
	if (copy_to_user(buf, (void *)data, (rbit / 8 + 1)) != 0) {
		return -EIO;
	}
	cp->current_pointer += rbit;
	return rbit;
			
}

ssize_t 
cmos_write (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return -1;
}

int 
cmos_ioctl (struct inode *inode, struct file *file, unsigned int a, unsigned long b)
{
	return -1;
}

int cmos_open (struct inode *inode, struct file *file)
{
	struct cmos_struct *cp;
	cp = container_of(inode->i_cdev, struct cmos_struct, cmos_cdev);
	file->private_data = cp;
	cp->size = CMOS_BANK_SIZE;
	cp->current_pointer = 0;
	return 0;
}

int cmos_release (struct inode *inode, struct file *file)
{
	struct cmos_struct *cp;
	cp = file->private_data;
	cp->current_pointer = 0;
	return 0;
}

static struct file_operations cmos_fops = {
   .owner	= THIS_MODULE,
   .open	= cmos_open,
   .read	= cmos_read,
   .write	= cmos_write,
   .release	= cmos_release,
   .llseek 	= cmos_llseek,
   .ioctl	= cmos_ioctl, 
};

int __init cmos_init(void)
{
	int i,ret;
	if (alloc_chrdev_region(&cmos_dev_num, 0,
		 NUM_CMOS_BANKS, DEVICE_NAME) < 0) {
		printk(KERN_ALERT "alloc_chrdev_region err\n");
		return -1;
	}

	/* sysfs entries */
	cmos_class = class_create(THIS_MODULE, DEVICE_NAME);

	for (i = 0; i < NUM_CMOS_BANKS; i++) {
		cmosp[i] = kmalloc(sizeof(struct cmos_struct), GFP_KERNEL);
		if (cmosp[i] == NULL) {
			printk(KERN_ALERT "kmalloc err\n");
			return -1;
		}

		sprintf(cmosp[i]->name, "cmos%d", i);
		if (!(request_region(addrports[i], 2, cmosp[i]->name))) {
			printk(KERN_ALERT "request_region err\n");
			return -1;
		}

		cmosp[i]->bank_num = i;
	
		cdev_init(&cmosp[i]->cmos_cdev, &cmos_fops);
		cmosp[i]->cmos_cdev.owner = THIS_MODULE;


		ret = cdev_add(&cmosp[i]->cmos_cdev, cmos_dev_num + i, 1);
		if (ret) {
			printk(KERN_ALERT "cdev_add\n");
			return ret;
		}

		/* /dev nodes */
		device_create(cmos_class, NULL,
			 MKDEV(MAJOR(cmos_dev_num), i),
			 NULL, "cmos%d", i);
	}
	printk(KERN_ALERT "cmos initialized\n");
	return 0;
}

void __exit cmos_exit(void)
{
	int i;
	unregister_chrdev_region(cmos_dev_num, NUM_CMOS_BANKS);

	for (i= 0; i < NUM_CMOS_BANKS; i++) {
		/* Release I/O region */
		device_destroy (cmos_class, MKDEV(MAJOR(cmos_dev_num), i));
		release_region(addrports[i], 2);
		cdev_del(&cmosp[i]->cmos_cdev);
		kfree(cmosp[i]);
	}

	/* sysfs */
	class_destroy(cmos_class);
}

module_init(cmos_init);
module_exit(cmos_exit);

MODULE_AUTHOR("Wang Bo");
MODULE_LICENSE("GPL");
