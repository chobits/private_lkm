#include <linux/slab.h>
#include <linux/fs.h>
#include "simp_fs.h"

static int simp_dir_getattr(struct vfsmount *mnt, struct dentry *dentry,
							struct kstat *stat)
{
	struct inode *inode;
	struct simp_dentry *sde;
	dfunc();
	inode = dentry->d_inode;
	sde = SDE(inode);

	generic_fillattr(inode, stat);
	return 0;
}


/*
 * __lookup_hash -.
 *                |->__d_lookup
 *                |->d_loopup
 *                `->inode->i_op->lookup(simp_lookup)
 *
 * If dentry exists in dir, the __d_lookup/d_lookup will found it then
 * our own func simp_lookup will not run!
 * So,if not err happen, it *must* return NULL whether dentry exists in dir
 */
static struct dentry *simp_lookup(struct inode *dir, struct dentry *dentry,
						struct nameidata *nd)
{
	struct inode *inode = NULL;
	dfunc();
	d_add(dentry, inode);
	return NULL;
}

static int simp_unlink(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	struct simp_dentry *p, **old;
	struct simp_dentry *dirsde = SIMP_I(dir)->sde;
	struct qstr *qname = &dentry->d_name;

	/* high-level dir unlink */
	for (old = &dirsde->subdir, p = dirsde->subdir; p; 
					old = &p->next, p = p->next) {
		if (p->namelen == qname->len &&
			strncmp(p->name, qname->name, qname->len) == 0)
			break;
	}
	if (!p)	{
		/* should happen? */ dprintk("dir not exits");
		return -1;
	}
	*old = p->next;
	dirsde->nlink--;

	/* normal unlink */
	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	drop_nlink(inode);
	dput(dentry);
	return 0;
}


static int simp_link(struct dentry *old_dentry, struct inode *dir,
						struct dentry *dentry)
{
	struct inode *inode = old_dentry->d_inode;
	struct simp_dentry *dirsde = SIMP_I(dir)->sde;
	struct qstr *qname = &dentry->d_name;
	int len;
	struct simp_dentry *oldsde;
	struct simp_dentry *sde;

	oldsde = SIMP_I(inode)->sde;
	len = sizeof(*sde) + qname->len + 1;
	sde = kzalloc(len, GFP_KERNEL);
	if (!sde) {
		dprintk("oop: kzalloc err");
		return -1;
	}
	memcpy(sde, oldsde, sizeof(*sde));
	sde->namelen = qname->len;
	sde->name = ((char *)sde) + sizeof(*sde);
	strncpy(sde->name, qname->name, qname->len);
	sde->name[qname->len] = '\0';
	sde->parent = dirsde;
	sde->next = dirsde->subdir;
	dirsde->subdir = sde;
	dirsde->nlink++;
	
	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	inc_nlink(inode);
	atomic_inc(&inode->i_count);
	d_instantiate(dentry, inode);
	dget(dentry);

	return 0;
}

static int simp_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	struct simp_dentry *sde = SIMP_I(inode)->sde;

	/* sde->nlink is entried attached to sde, larger than hard link */
	if (sde->nlink > 2) {
		dprintk("not empty dir");
		return -ENOTEMPTY;
	}

	/* unlink */
	if (simp_unlink(dir, dentry))
		return -1;
	kfree(sde);

	/* dir */
	drop_nlink(dir);
	return 0;
}

/*
 * @dentry: new dir's dentry
 */
static int simp_mknod(struct inode *dir, struct dentry *dentry, int mode)
{
	struct inode *inode;	
	struct simp_dentry *sde, *dirsde;
	struct super_block *sb = dir->i_sb;
	struct qstr *qname = &dentry->d_name;

	/* handle high-level dir */
	sde = kzalloc(sizeof(*sde) + qname->len + 1, GFP_KERNEL);
	if (!sde)
		panic("oops: kzalloc err\n");

	sde->namelen = qname->len;	
	sde->name = ((char *)sde) + sizeof(*sde);
	strncpy(sde->name, qname->name, qname->len);
	sde->name[sde->namelen] = '\0';
	sde->nlink = 1;
	if (S_ISDIR(mode))
		sde->nlink++;
	sde->mode = mode;

	inode = simp_get_inode(sb, sde, 0);
	if (!inode) {
		dprintk("simp_get_inode err");
		kfree(sde);
		return -1;
	}

	/* attach subdir to parent */
	dirsde = SIMP_I(dir)->sde;
	sde->next = dirsde->subdir;
	sde->parent = dirsde;
	dirsde->subdir = sde;
	dirsde->nlink++;

	/* attach inode to dentry */
	d_instantiate(dentry, inode);
	dget(dentry);
	dir->i_mtime = dir->i_ctime = CURRENT_TIME;

	return 0;
}

static int simp_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	int ret;
	ret = simp_mknod(dir, dentry, mode | S_IFDIR);
	if (!ret)
		inc_nlink(dir);	/* hard link, new dir add it, file not */
	return ret;
}

static int simp_create(struct inode *dir, struct dentry *dentry, int mode,
						struct nameidata *nd)
{
	return simp_mknod(dir, dentry, mode | S_IFREG);
}

struct inode_operations simp_file_inode_operations;
struct inode_operations simp_symlink_inode_operations;
struct inode_operations simp_dir_inode_operations = {
	.getattr = simp_dir_getattr,
	.lookup = simp_lookup,
	.create = simp_create,
	.mkdir = simp_mkdir,
	.rmdir = simp_rmdir,
	.link = simp_link,
	.unlink = simp_unlink,
};
