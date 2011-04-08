#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

static const struct seq_operations pp = {
	.start = s_start,
	.next = s_next,
	.stop = s_stop,
	.show = s_show,
};

int popen(struct inode *inode, struct file *file)
{
	printk(KERN_ALERT "in open\n");
	dump_stack();
	return seq_open(file, &pp);
}
EXPORT_SYMBOL(popen);

static const struct file_operations p = {
	.open = popen,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release_private,
}

int __init hello_init(void)
{
	proc_create("proc_create", S_IRUSR, NULL, &p);
	return 0;
}

void __exit hello_exit(void)
{
	remove_proc_entry("kk", NULL);
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

