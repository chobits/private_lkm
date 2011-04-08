#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>

static int mprotect_ret_handler(struct kretprobe_instance *ri,
				struct pt_regs *regs)
{
	printk(KERN_ALERT "original return address: 0x%lx\n",
						(unsigned long)ri->ret_addr);
	return 0;
}

static struct kretprobe mprotect_kretprobe = {
	.handler = mprotect_ret_handler,
	.maxactive = NR_CPUS,
};

int __init kinit(void)
{
	int ret;
	mprotect_kretprobe.kp.addr = 
			(kprobe_opcode_t *)kallsyms_lookup_name("sys_mprotect");
	if ((ret = register_kretprobe(&mprotect_kretprobe)) < 0) {
		printk(KERN_ALERT "kretprobe register err\n");
		return -1;
	}
	return 0;
}

void __exit kexit(void)
{
	unregister_kretprobe(&mprotect_kretprobe);
}

module_init(kinit);
module_exit(kexit);
MODULE_LICENSE("GPL");
