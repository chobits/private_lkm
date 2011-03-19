#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>

/* 
 * change the value to real addr:
 * grep -w kernel_text_address /proc/kallsyms
 */
static int (*kernel_text_addressp)(unsigned int) =
					(int (*)(unsigned int))0xc044f107;

void my_dump_stack(void)
{
	unsigned int stack;
	unsigned int top;
	unsigned int addr;
	printk("Pid: %d, comm: %.20s %s %s %.*s\n",
			current->pid, current->comm, print_tainted(),
			init_utsname()->release,
			(int)strcspn(init_utsname()->version, " "),
			init_utsname()->version); 

	printk("Call Trace:\n");
	/* get stack point */
	stack = (unsigned int)&stack;
	/* get stack top point */
	top = (unsigned int)current_thread_info() + THREAD_SIZE;

	for (; stack < top; stack += 4) {
		addr = *(unsigned int *)stack;
		if (kernel_text_addressp(addr))
			printk(" [<%p>] %pS\n", (void *)addr, (void *)addr);
	}
}

static int __init init(void)
{
	/* test */
	my_dump_stack();

	return 0;
}

static void __exit exit(void)
{
	dump_stack();
	/* test */
	my_dump_stack();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wang xiao chen <wangxiaochen0@gmail.com>");

module_init(init);
module_exit(exit);
