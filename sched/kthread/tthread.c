#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/delay.h>

static void print_time(const char *s)
{
	int second,minite,hour;
	second = jiffies/HZ;
	minite = second / 60;
	hour = minite / 60;
	second %= 60;
	minite %= 60;
	printk(KERN_ALERT"%s:%d:%d:%d\n",s,hour,minite,second);
}

int mythread_func(void *data)
{
	int i = 0;
	while (!kthread_should_stop()) {
		print_time("Hello I'm new kthread");
		mdelay(3000);
		i++;
		if (i == 10)
			break;	
	}
	return 0;
}

static struct task_struct * mythread;
static int __init mythread_init(void)
{
	int ret = 0;
	printk(KERN_ALERT"init\n");
	mythread = kthread_create(mythread_func, NULL, "%s", "Hellod");
	if (!IS_ERR(mythread))
		wake_up_process(mythread);
	else {
		printk(KERN_ALERT"ERR:kthread_create\n");
		ret = -1;
	}
	return ret;
}


static void __exit mythread_exit(void)
{
	kthread_stop(mythread);
	printk(KERN_ALERT"k:exit!\n");
}

MODULE_LICENSE("GPL");
module_init(mythread_init);
module_exit(mythread_exit);
