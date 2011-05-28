#include "asec.h"

int request_for_open(struct file *f)
{
	int ret;
	ret = request_for_policy(f);
	if (ret)
		return ret;
	return request_for_query(f);
}

int asec_dentry_open(struct file *f, const struct cred *cred)
{
	return request_for_open(f);
}

int asec_inode_permission(struct inode *inode, int mask)
{
	if (mask & MAY_CHDIR)
		return request_for_chdir(inode);
	return 0;
}
