/*
 * for linux 2.6.36
 */
#include "asec.h"

extern const struct file_operations query_ops;
extern const struct file_operations policy_ops;

static struct dentry *asec_dir;

static void asecfs_remove(const char *filename)
{
	struct dentry *dentry;
	dentry = lookup_one_len(filename, asec_dir, strlen(filename));
	if (!IS_ERR(dentry)) {
		securityfs_remove(dentry);
		dput(dentry);
	}
}

void asecfs_exit(void)
{
	if (asec_dir) {
		asecfs_remove("policy");
		asecfs_remove("query");
		securityfs_remove(asec_dir);
		asec_dir = NULL;
	}
}

static int asec_create(const char *name, int mask,
					const struct file_operations *fops)
{
	struct dentry *dentry;
	dentry = securityfs_create_file(name, S_IFREG | mask, asec_dir,
								NULL, fops);
	return IS_ERR(dentry) ? PTR_ERR(dentry) : 0;
}

int asecfs_init(void)
{
	int error;

	asec_dir = securityfs_create_dir("asec", NULL);
	if (IS_ERR(asec_dir)) {
		error = PTR_ERR(asec_dir);
		asec_dir = NULL;
		asecerr("securityfs_create_dir error");
		goto err_free_fs;
	}

	/* create file interface: query */
	error = asec_create("query", 0600, &query_ops);
	if (error) {
		asecerr("create query error");
		goto err_free_fs;
	}

	/* create file interface: policy */
	error = asec_create("policy", 0600, &policy_ops);
	if (error) {
		asecerr("create query error");
		goto err_free_fs;
	}
	/* init policy internal structure */
	init_policy();

	printk(KERN_ALERT "asec filesystem init ok\n");
	return 0;

err_free_fs:
	asecfs_exit();
	printk(KERN_ALERT "Error init asec securityfs\n");
	return error;
}
