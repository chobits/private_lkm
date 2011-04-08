#include "simp_fs.h"

/*
 * return >0 ok. return 0 err.
 */
static int simp_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
	int ret, i;
	struct inode *inode;
	struct simp_dentry *sde;
	u64 ino;
	dfunc();

	/* 0 for error */
	ret = 0;
	/* dir inode */
	inode = filp->f_dentry->d_inode;
	/* dir sde */
	sde = SDE(inode);
	
	i = filp->f_pos;
	ino = inode->i_ino;

	/* handle f_pos(i) need some tricks */	
	switch (i) {
	case 0:
		if (filldir(dirent, ".", 1, i, ino, DT_DIR) < 0)
			goto out;
		i++;
		filp->f_pos++;
	case 1:
		if (filldir(dirent, "..", 2, i, parent_ino(filp->f_dentry),
				DT_DIR) < 0)
			goto out;
		i++;
		filp->f_pos++;
	default:
		/* subdir */
		sde = sde->subdir;
		i -= 2;
		/* jump handled dir*/
		while (1) {
			if (!sde)
				goto outok;
			if (!i)
				break;
			sde = sde->next;
			i--;
		}
		/* read other dirs */
		while (1) {
			if (!sde)
				break;
			if (filldir(dirent, sde->name, sde->namelen,
				 filp->f_pos, sde->ino, sde->mode >> 12) < 0)
				goto out;	
			filp->f_pos++;
			/* FIXME:handle sde->count */
			sde = sde->next;	
		}
		break;
	}	
outok:
	ret = 1;
out:
	if (!ret)
		dprintk("filldir err");
	return ret;
}

int simp_sync_file(struct file *file, struct dentry *dentry, int datasync)
{
	dfunc();
	return 0;
}

struct file_operations simp_file_operations = {
	.llseek = generic_file_llseek,
	.read = do_sync_read,
	.aio_read = generic_file_aio_read,
	.write = do_sync_write,
	.aio_write = generic_file_aio_write,
	.fsync = simple_sync_file,
	.mmap = generic_file_mmap,
	.splice_read = generic_file_splice_read,
	.splice_write = generic_file_splice_write,
};

struct file_operations simp_dir_operations = {
	.read = generic_read_dir,
	.readdir = simp_readdir,
};
