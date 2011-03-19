#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#define PROC_NAME ("helloworld")

struct proc_dir_entry *d_proc;
static char proc_buf[1024];
static int proc_buf_size = 0;

int p_read(char *buf,
	char **buf_location, off_t offset, int len, int *eof, void *data)
{
	int ret;
	if (offset > 0)
		ret = 0;
	else /*bug:(*/
		ret = snprintf(buf, proc_buf_size, proc_buf);

	return ret;
}

int p_write(struct file *filep, 
	const char *buf, unsigned long count, void *data)
{
	proc_buf_size = count;
	if (proc_buf_size > 1024)
		proc_buf_size = 1024;
	if (copy_from_user(proc_buf, buf, proc_buf_size)) 
		return -EFAULT;
	
	return proc_buf_size;

}

static int __init proc_init(void)
{
	d_proc = create_proc_entry(PROC_NAME, 0644, NULL);

	if (!d_proc) {
		remove_proc_entry(PROC_NAME, NULL);
		return -ENOMEM;
	}

	d_proc->read_proc = p_read;
	d_proc->write_proc = p_write;
	d_proc->mode = S_IFREG | S_IRUGO;
	d_proc->uid = 0;
	d_proc->gid = 0;
	d_proc->size = 37;

	return 0;
}

static void __exit proc_exit(void)
{
	remove_proc_entry(PROC_NAME, NULL);
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_AUTHOR("Wang Bo");
MODULE_LICENSE("GPL");
