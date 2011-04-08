#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

int __init hello_init(void)
{
	struct proc_dir_entry *dir;
	dir = proc_symlink("link", NULL, "/no");
	if (!dir)
		return -1;
	printk(KERN_ALERT "init ok\n");
	return 0;
}

void __exit hello_exit(void)
{
	remove_proc_entry("link", NULL);
	printk(KERN_ALERT "exit ok\n");
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

