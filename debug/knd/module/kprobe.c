#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>

extern void dump_stack_only_task(void);

struct jp_node {
	struct list_head node;
	struct jprobe jp;
	unsigned int symlen;
	char sym[0];
};

static LIST_HEAD(jph);

static int jhook(void)
{
	dump_stack_only_task();
	jprobe_return();
	/*
	 * not run here, return from jprobe_return();
	 */
	return 0;
}

static struct jp_node *get_jpn(char *name)
{
	int namelen;
	struct jp_node *jpn;
	namelen = strlen(name);
	list_for_each_entry(jpn, &jph, node) {
		if (jpn->symlen == namelen &&
				strncmp(name, jpn->sym, namelen) == 0)
			goto found;
	}
	return NULL;
found:
	return jpn;
}

/*
 * set break point
 * @name :func sym name
 */
int set_bp(char *name)
{
	int ret, len;
	struct jp_node *jpn;
	ret = 0;

	if ((jpn = get_jpn(name)) != NULL) {
		printk(KERN_ALERT "err:%s has been set\n", name);
		ret = -1;
		goto out;
	}

	len = sizeof(*jpn) + strlen(name) + 1;
	jpn = (struct jp_node *)kmalloc(len, GFP_KERNEL);	
	if (jpn == NULL) {
		printk(KERN_ALERT "cant alloc jprobe\n");
		ret = -1;
		goto out;
	}

	jpn->jp.entry = (kprobe_opcode_t *)jhook;
	jpn->jp.kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name(name);
	if (jpn->jp.kp.addr == NULL) {
		printk(KERN_ALERT "%s cant be found\n", name);	
		ret = -1;
		goto out_jp;
	}
	if ((ret = register_jprobe(&jpn->jp)) < 0) {
		printk(KERN_ALERT "jprobe register err\n");
		ret = -1;
		goto out_jp;
	}

	jpn->symlen = strlen(name);
	strncpy(jpn->sym, name, jpn->symlen);
	jpn->sym[jpn->symlen] = '\0';	

	INIT_LIST_HEAD(&jpn->node);
	list_add(&jpn->node, &jph);

	printk(KERN_ALERT "%s set ok\n", name);
	return ret;
out_jp:
	kfree(jpn);
out:
	return ret;
}

void unset_bp(char *name)
{
	struct jp_node *jpn;

	if ((jpn = get_jpn(name)) != NULL) {
		unregister_jprobe(&jpn->jp);
		list_del(&jpn->node);
		kfree(jpn);
		printk(KERN_ALERT "%s unset ok\n", name);
		return;
	}

	printk(KERN_ALERT "err:%s has not been set\n", name);
}

void info_bp(void)
{
	struct jp_node *jpn;

	if (list_empty(&jph)) {
		printk(KERN_ALERT "No break points set\n");
		return;
	}
	list_for_each_entry(jpn, &jph, node) {
		printk(KERN_ALERT "BP:%s at 0x%x\n",
				jpn->sym, (unsigned int)jpn->jp.kp.addr);
	}
}

MODULE_LICENSE("GPL");
