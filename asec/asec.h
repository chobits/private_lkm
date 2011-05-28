#ifndef __ASEC_H
#define __ASEC_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/security.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/namei.h>
#include <linux/poll.h>
#include <linux/ctype.h>

#define asecerr(fmt, arg...)\
	printk(KERN_ERR "%s "fmt"\n", __FUNCTION__, ##arg)

#define asecinfo(fmt, arg...)\
	printk(KERN_ALERT "%s "fmt"\n", __FUNCTION__, ##arg)

#ifdef DEBUG
#define asecdbg(fmt, arg...)\
	printk(KERN_ALERT "[k]%s "fmt"\n", __FUNCTION__, ##arg)
#else
#define asecdbg(fmt, arg...)
#endif

#define dbg(fmt, arg...)\
	printk(KERN_ALERT "[k]%s "fmt"\n", __FUNCTION__, ##arg)



extern int request_for_chdir(struct inode *);
extern int request_for_policy(struct file *);
extern int request_for_query(struct file *);

/* hook */
extern int asec_dentry_open(struct file *, const struct cred *);
extern int asec_inode_permission(struct inode *, int);

extern void unregister_security(struct security_operations *);

extern void asecfs_exit(void);
extern int asecfs_init(void);


extern void exit_policy(void);
extern void init_policy(void);

#endif
