#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

#define PROC_FILE "/proc/kallsyms"
static struct file *file = NULL;

int find_sym(char *symname, unsigned int *value)
{
	char buf[512];
	char *find;
	int n, seek, symlen, ret;
	mm_segment_t old_fs;

	ret = -1;
	file = filp_open(PROC_FILE, O_RDONLY, 0);
	if (IS_ERR(file)) {
		printk(KERN_ALERT "Cannot open %s\n", PROC_FILE);
		goto out0;
	}
	
	if (!file->f_op || !file->f_op->read) {
		goto out1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	symlen = strlen(symname);
	find = NULL;
	seek = 0;

	while ((n = file->f_op->read(file, buf, 512, &file->f_pos)) > 0) {
		if ((find = strstr(buf, symname)) != NULL &&
					find[-1] == ' ' &&
					find[symlen] == '\n')
			break;

//		while (n - 1 - seek >= 0 && buf[n - 1 - seek] != '\n') 
//			seek++;

//		if (seek) {
//			file->f_pos -= seek;
//			seek = 0;
//		}
		find = NULL;
	}
	if (n == -1) {
		printk(KERN_ALERT "read err\n");
		goto out;
	}

	if (find != NULL) {
		find = find - 11;	
		find[8] = 0;
		*value = (unsigned int)simple_strtoul(find, NULL, 16);
		printk(KERN_ALERT "%s: 0x%s\n", symname, find);
		ret = 0;
	}
	else {
		printk(KERN_ALERT "cannot find %s\n", symname);
		goto out;
	}

out:
	set_fs(old_fs);
out1:
	filp_close(file, NULL);
out0:
	return ret;
}

static int __init hello_init(void)
{
	unsigned int addr;
	if (find_sym("system_call", &addr) == 0)
		printk(KERN_ALERT "0x%x\n", addr);

	if (find_sym("sys_call_table", &addr) == 0)
		printk(KERN_ALERT "0x%x\n", addr);

	if (find_sym("c", &addr) == 0)
		printk(KERN_ALERT "0x%x\n", addr);
	return 0;
}

static void __exit hello_exit(void)
{
	
	printk(KERN_ALERT "Goodbye kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
