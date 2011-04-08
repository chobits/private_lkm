#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <asm/highmem.h> /* kmap */
#include <asm/pgtable_32_types.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");

#define MAX_SIZE (130 * 1024 * 1024)
#define MB1 (1 * 1024 * 1024)
#define MB2 (2 * 1024 * 1024)
#define MB4 (4 * 1024 * 1024)
#define MB8 (8 * 1024 * 1024)

void *p_8M[128];
void *p_2M[128];
void *p_4M[128];
void *p_1M[128];

static int __init hello_init(void)
{
	unsigned int size, total_min;
	int i8, i2, i4, i1;	
	void *p;

	total_min = 0;
	printk(KERN_ALERT "-----------vmalloc max MB---------\n");
	for (size = MAX_SIZE; size; size -= MB1) {
		p = vmalloc(size);
		if (p) {
			total_min += size;
			break;	
		}

	}
	for (i1 = 0; i1 < 128; i1++) {
		if ((p_1M[i1] = vmalloc(MB1)) == NULL) {
			i1--;
			break;
		}
		total_min += MB1;
	}
	if (p) 
		vfree(p);
	
	for (; i1 >= 0; i1--) {
		vfree(p_1M[i1]);
	}
	printk(KERN_ALERT "total_min: %ubytes = %dMB\n",
					total_min, total_min/MB1);

	printk(KERN_ALERT "-----------vmalloc 8MB/2MB---------\n");
	for (i8 = i2 = 0; i2 < 128; i2++, i8++) {
		if ((p_8M[i8] = vmalloc(MB8)) == NULL) {
			i8--;
			i2--;
			break;
		}

		total_min -= MB8;
		if ((p_2M[i2] = vmalloc(MB2)) == NULL) {
			i2--;
			break;
		}
		total_min -= MB2;
	}

	printk(KERN_ALERT "vmalloc: MB8*%d, MB2*%d\n", i8+1, i2+1);
	printk(KERN_ALERT "total_min: %ubytes = %dMB\n",
					total_min, total_min/MB1);
	printk(KERN_ALERT "-----------vfree all MB2-------------\n");
	for (; i2 >= 0; i2--) {
		vfree(p_2M[i2]);
		total_min += MB2;
	}
	printk(KERN_ALERT "total_min: %ubytes = %dMB\n",
			total_min, total_min/MB1);
	printk(KERN_ALERT "-------------vmalloc MB4---------------\n");
	for (i4 = 0; i4 < 128; i4++) {
		if ((p_4M[i4] = vmalloc(MB4)) == NULL) {
			i4--;
			break;
		}
		total_min -= MB4;
	}

	if (i4+1 == 0)
		printk(KERN_ALERT "cannot vmalloc MB4\n");
	else
		printk(KERN_ALERT "vmalloc: MB4*%d\n", i4+1);

	printk(KERN_ALERT "total_min: %ubytes = %dMB\n",
			total_min, total_min/MB1);
	printk(KERN_ALERT "------------vfree all-------------\n");
	for (; i8 >= 0; i8--) {
		vfree(p_8M[i8]);
		total_min += MB8;
	}
	for (; i4 >= 0; i4--) {
		vfree(p_4M[i4]);
		total_min += MB4;
	}
	printk(KERN_ALERT "total_min: %ubytes = %dMB\n",
			total_min, total_min/MB1);

	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
