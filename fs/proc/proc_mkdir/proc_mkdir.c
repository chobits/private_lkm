#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

int __init hello_init(void)
{
	struct proc_dir_entry *dir;
	dir = proc_create("proc_create", 0555 | S_IFDIR, NULL, &p);
	if (!dir)
		return -1;
	return 0;
}

void __exit hello_exit(void)
{
	remove_proc_entry("kk", NULL);
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

