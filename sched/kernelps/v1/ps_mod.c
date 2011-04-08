#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/rwlock.h>
#include <linux/kallsyms.h>

#define pk(fmt, arg...) printk(KERN_ALERT "[ps mod]" fmt "\n", ##arg)

static rwlock_t *tasklist_lockp;

static int ps_proc_show(struct seq_file *m, void *v)
{
	struct task_struct *task;
	read_lock(tasklist_lockp);
	seq_printf(m, "process             state    pid\n");
	for_each_process(task) {
		seq_printf(m, "%-20s%-10ld%-5d\n",
			task->comm, task->state, task->pid);
	}
	read_unlock(tasklist_lockp);
	return 0;
}

static int ps_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ps_proc_show, NULL);
}


static const struct file_operations ps_proc_fops = {
	.open	= ps_proc_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
};

static int __init psmod_init(void)
{
	tasklist_lockp = 
		(rwlock_t *)kallsyms_lookup_name("tasklist_lock");
	if (!tasklist_lockp) {
		pk("cannot find tasklist_lock");
		return -1;
	}
	proc_create("_ps", 0, NULL, &ps_proc_fops);
	pk("init ok");
	return 0;
}

static void __exit psmod_exit(void)
{
	pk("goodbye kernel");
}

MODULE_LICENSE("GPL");
module_init(psmod_init);
module_exit(psmod_exit);

