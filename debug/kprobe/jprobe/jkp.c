#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>

static int n_sys_mprotect(unsigned long start, size_t len, long prot)
{
	struct pt_regs *regs = task_pt_regs(current);
	start = regs->bx;
	len = regs->cx;
	prot = regs->dx;
	printk(KERN_ALERT "start: 0x%lx len: %u prot:0x%lx\n",
							start, len, prot);
	printk(KERN_ALERT "%%eax:%lx ?= 1\n", regs->ax);
	jprobe_return();
	printk(KERN_ALERT "jprobe ret\n");
	return 0;

}

static struct jprobe mprotect_jprobe = {
	.entry = (kprobe_opcode_t *)n_sys_mprotect,
};

int __init kinit(void)
{
	int ret;
	mprotect_jprobe.kp.addr = 
			(kprobe_opcode_t *)kallsyms_lookup_name("sys_mprotect");
	if ((ret = register_jprobe(&mprotect_jprobe)) < 0) {
		printk(KERN_ALERT "jprobe register err\n");
		return -1;
	}
	printk(KERN_ALERT "0x%x\n", (unsigned int)mprotect_jprobe.kp.flags);
	printk(KERN_ALERT "0x%x\n", (unsigned int)mprotect_jprobe.kp.ainsn.boostable);
	printk(KERN_ALERT "0x%x\n", (unsigned int)mprotect_jprobe.kp.opcode);
	printk(KERN_ALERT "0x%x\n", (unsigned int)mprotect_jprobe.kp.addr);
	printk(KERN_ALERT "0x%x\n", (unsigned int)mprotect_jprobe.kp.ainsn.insn);
	return 0;
}

void __exit kexit(void)
{
	unregister_jprobe(&mprotect_jprobe);
}

module_init(kinit);
module_exit(kexit);
MODULE_LICENSE("GPL");
