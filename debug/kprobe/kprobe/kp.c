#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

MODULE_LICENSE("GPL");

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	printk(KERN_ALERT "in pre:p->addr = 0x%p, ip = %lx,"
			" flags = 0x%lx\n",
		p->addr, regs->ip, regs->flags);
	return 0;

}

static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	printk(KERN_ALERT "in post:p->addr = 0x%p, ip = %lx,"
			" flags = 0x%lx [flags %lx]\n",
		p->addr, regs->ip, regs->flags, flags);

}
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(KERN_ALERT "in fault:p->addr = 0x%p, ip = %lx,"
			" flags = 0x%lx [trapnr %d]\n",
		p->addr, regs->ip, regs->flags, trapnr);
	return 0;

}
static struct kprobe kp = {
	.symbol_name = "do_fork",
	.pre_handler = handler_pre,
	.post_handler = handler_post,
	.fault_handler = handler_fault,
};

int __init kinit(void)
{
	return register_kprobe(&kp);
}

void __exit kexit(void)
{
	unregister_kprobe(&kp);
}

module_init(kinit);
module_exit(kexit);
