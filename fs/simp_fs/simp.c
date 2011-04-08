#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include <linux/mount.h>
#include <linux/slab.h>
#include "simp_fs.h"

static struct simp_dentry simp_root = {
	.ino = 0,
	.namelen = 5,
	.name = "/simp",
	.nlink = 2,
	.parent = &simp_root,
	.mode = S_IFDIR | S_IRUGO | S_IXUGO,
};

static struct kmem_cache *simp_inode_cachep;

/*
 * alloc simp_inode(embedded with inode)
 * and init simp_inode fields
 */
static struct inode *simp_alloc_inode(struct super_block *sb)
{
	struct simp_inode *sino;
	struct inode *inode;
	dfunc();
	sino = (struct simp_inode *)kmem_cache_alloc(simp_inode_cachep,
			GFP_KERNEL);
	if (!sino)
		return NULL;

	sino->sde = NULL;
	inode = &sino->vfs_inode;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	return inode;
}

/*
 * free inode
 */
static void simp_destroy_inode(struct inode *inode)
{
	dfunc();
	kmem_cache_free(simp_inode_cachep, SIMP_I(inode));
}

static int simp_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	struct super_block *sb = dentry->d_sb;
	dfunc();
	buf->f_type = sb->s_magic;
	buf->f_bsize = sb->s_blocksize;
	return 0;	
}

static struct super_operations simp_sops = {
	.alloc_inode = simp_alloc_inode,
	.destroy_inode = simp_destroy_inode,
	.statfs = simp_statfs,
};

/*
 * new alloced inode basic initialization
 */
static void sino_init_once(void *data)
{
	struct simp_inode *si;
	dfunc();
	si = (struct simp_inode *)data;
	inode_init_once(&si->vfs_inode);
}

static int simp_init_inodecache(void)	
{
	dfunc();
	simp_inode_cachep = kmem_cache_create("simp_inode_cache",
			sizeof(struct simp_inode),
			0, (SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD),
			sino_init_once);
	if (!simp_inode_cachep) {
		dprintk("cant create simp_inode cache");
		return -1;
	}
	return 0;
}

static void destroy_inodecache(void)
{
	dfunc();
	kmem_cache_destroy(simp_inode_cachep);
}

struct inode *simp_get_inode(struct super_block *sb, 
		struct simp_dentry *sde, u64 ino)
{
	struct inode *inode;
	dfunc();
	/* inode alloc and init(lists) */
	/* call sb->s_op->alloc_inode */
	if (!ino)
		inode = new_inode(sb);
	else
		inode = iget_locked(sb, ino);

	if (!inode)
		return NULL;	

	if ((inode->i_state & I_NEW) || !ino) {
		inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
		inode->i_mode = sde->mode;
		inode->i_nlink = sde->nlink;
		SIMP_I(inode)->sde = sde;
		if (!ino || !(sde->ino))
			sde->ino = inode->i_ino;

		if (inode->i_mapping)
			inode->i_mapping->a_ops = &simp_aops;
		else
			dprintk("inode i_mapping null");
		switch (inode->i_mode & S_IFMT) {
		case S_IFREG:
			inode->i_op = &simp_file_inode_operations;
			inode->i_fop = &simp_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &simp_dir_inode_operations;
			inode->i_fop = &simp_dir_operations;
			break;
		case S_IFLNK:
			inode->i_op = &simp_symlink_inode_operations;
			break;
		default:
			dprintk("warning inode->i_mode unkown");
			break;
		}	
		if (inode->i_state & I_NEW)
			unlock_new_inode(inode);
	}
	else {
		//put sde
	}

	return inode;
}

/* fill super's unchanged field */
/* we know data == NULL, silent == 0*/
static int simp_fill_super(struct super_block *sb, void *data, int silent)
{
	int err;
	struct inode *root_inode;
	dfunc();
	/*
	 * disallow update access time
	 * ignore suid and sgid bits
	 * disallow programe execution
	 */
	sb->s_flags |= MS_NODIRATIME | MS_NOSUID | MS_NOEXEC;
	sb->s_blocksize = 1024;
	sb->s_blocksize_bits = 10;
	sb->s_magic = SIMP_SUPER_MAGIC;
	sb->s_op = &simp_sops; /* will be used in simp_get_inode */
	sb->s_time_gran = 1;
	//sb->s_fs_info

	err = -1;
	/* alloc root inode */
	root_inode = simp_get_inode(sb, &simp_root, simp_root.ino);
	if (!root_inode)
		goto fail;
	/* ?? */
	root_inode->i_uid = 0;
	root_inode->i_gid = 0;

	/*
	 * alloc root dentry between sb and root inode 
	 * second mount will not run simp_fill_super 
	 */
	//d_alloc_root (alloc '/' ??)
	sb->s_root = d_alloc_root(root_inode);
	if (!sb->s_root)
		goto fail;

	return 0;

fail:
	dprintk("simpfs get root inode failed");
	iput(root_inode);
	return err;
}

/* test for sget() to select an old super block */
static int simp_test_super(struct super_block *sb, void *data)
{
	/* data is NULL */
	/* return sb->s_fs_info == data */
	/* for dev: data == sb->s_sbdev */

	dfunc();
	return 1;
}

/* for sget to init the alloced super block */
static int simp_set_super(struct super_block *sb, void *data)
{
	/* data is NULL */
	/* alloc dev id */
	/* 
	 * for dev: sb->s_bdev = data 
	 * sb->s_dev = s->s_bdev->bd_dev 
	 * sb->s_bdi = &s->s_bdev->bd_disk->queue->backing_dev_info
	 */
	dfunc();
	/*
	 * set_anon_super(x, data): data is *ignored* by func!
	 * s->s_dev = MKDEV(0, xx) 
	 * s->s_bdi = &noop_backing_dev_info
	 * Unnamed block devices are dummy device used by virtual
	 * filesystems which don't use real block-devices 
	 */
	return set_anon_super(sb, NULL);	
}

static int simp_get_sb(struct file_system_type *type, int flags,
		const char *dev_name, void *data, struct vfsmount *mnt)
{
	int err;
	struct super_block *sb;

	dfunc();
	/* alloc super_block */
	sb = sget(type, simp_test_super, simp_set_super, NULL);
	if (IS_ERR(sb))
		return PTR_ERR(sb);

	/* if second mount we dont run below */
	if (!sb->s_root) {
		/* init sb */
		sb->s_flags = flags;
		err = simp_fill_super(sb, data, 0);
		if (err) {
			deactivate_locked_super(sb);
			return err;
		}
		sb->s_flags |= MS_ACTIVE;
	}

	simple_set_mnt(mnt, sb);
	return 0;
}

void simp_kill_sb(struct super_block *sb)
{
	dfunc();
	kill_anon_super(sb);
}

static struct file_system_type simp_fs_type = {
	.owner = THIS_MODULE,
	.name = "simp",
	.get_sb = simp_get_sb,
	.kill_sb = simp_kill_sb,
};

//static struct vfsmount *simp_mnt;
int __init simp_fs_init(void)
{
	int err;
	err = simp_init_inodecache();
	if (err) {
		dprintk("cant alloc inode cache");
		return -1;
	}

	err = register_filesystem(&simp_fs_type);
	if (err) {
		dprintk("cant register fs");
		return -1;
	}
	/* 
	 * I dont find func like *un*kern_mount
	 * so dont use it, we can use userland program: mount/umount
	 *
	 simp_mnt = kern_mount(&simp_fs_type);
	 if (!simp_mnt) {
	 dprintk("cant mount");
	 return -1;
	 }
	 */

	dprintk("build simp filesystem ok");
	return 0;
}

void __exit simp_fs_exit(void)
{
	unregister_filesystem(&simp_fs_type);
	destroy_inodecache();
	dprintk("remove simpfs ok");
}

MODULE_LICENSE("GPL");
module_init(simp_fs_init);
module_exit(simp_fs_exit);
