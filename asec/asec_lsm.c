#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cred.h>
#include <linux/security.h>

#include "asec.h"

static struct security_operations asec_security_ops = {
	.name = "asec",
	.dentry_open = asec_dentry_open,
	.inode_permission = asec_inode_permission,
};

static int __init init(void)
{
	struct cred *cred = (struct cred *)current_cred();
	int err;

	if ((err = register_security(&asec_security_ops)))
		return err;
	printk(KERN_ALERT "register asec security successes\n");

	cred->security = &asec_security_ops;
	if ((err = asecfs_init()) != 0) {
		unregister_security(&asec_security_ops);
		return err;
	}

	printk(KERN_ALERT "asec security module installed ok\n");
	return 0;
}

static void __exit exit(void)
{
	unregister_security(&asec_security_ops);
	asecfs_exit();
	printk(KERN_ALERT "goodbye kernel\n");
}

MODULE_LICENSE("GPL");

module_init(init);
module_exit(exit);
