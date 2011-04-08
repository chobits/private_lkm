#ifndef __SIMP_FS_H
#define __SIMP_FS_H
#include <linux/fs.h>
#include <linux/types.h>

#define DEBUG 1
#if DEBUG
#define dprintk(fmt, arg...) \
	printk(KERN_ALERT "[simp] %s "fmt"\n",__FUNCTION__, ##arg)
#else
#define dprintk(fmt, arg...) 
#endif

#define dfunc() dprintk("")

#define SIMP_SUPER_MAGIC 0xdeadbeef

struct simp_dentry {
	u64 ino;
	char *name;
	unsigned int namelen;
	unsigned int count;
	nlink_t nlink;
	mode_t mode;
	struct simp_dentry *parent, *next, *subdir;
	/* (namelen + 1) chars for name */
};

struct simp_inode {
	struct inode vfs_inode;
	struct simp_dentry *sde;
};

#define SIMP_I(inode) ((struct simp_inode *)(inode))
#define SDE(inode) (SIMP_I(inode)->sde)

extern struct address_space_operations simp_aops;
extern struct file_operations simp_file_operations;
extern struct file_operations simp_dir_operations;
extern struct inode_operations simp_file_inode_operations;
extern struct inode_operations simp_dir_inode_operations;
extern struct inode_operations simp_symlink_inode_operations;
extern struct inode *simp_get_inode(struct super_block *, struct simp_dentry *,
								u64);
extern int simp_sync_file(struct file *, struct dentry *, int);
extern int simp_write_end(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned copied, struct page *page,
		void *fsdata);
extern int simp_write_begin(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned flags, struct page **pagep,
		void **fsdata);
extern int simp_readpage(struct file *file, struct page *page);
extern int simp_set_page_dirty(struct page *page);

#endif
