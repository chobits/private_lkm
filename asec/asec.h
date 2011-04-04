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

#define asecerr(fmt, arg...)\
	printk(KERN_ERR "%s "fmt"\n", __FUNCTION__, ##arg)

#ifdef DEBUG
#define asecdbg(fmt, arg...)\
	printk(KERN_ALERT "[k]%s "fmt"\n", __FUNCTION__, ##arg)
#else
#define asecdbg(fmt, arg...)
#endif

/* used for /sys/kernel/security/asec/querd read/write/poll */
struct query_entry {
	unsigned int syncnum;	/* syncronizing with userspace communication */
	unsigned int answer;	/* user-space answer for this query */
	char *query;		/* query string for user to read */
	int len;		/* string length of query */
	struct list_head list;	/* listed in query_list */
	char query_str[0];	/* query real addr */
};

extern int request_for_open(struct file *);

extern int asec_dentry_open(struct file *, const struct cred *);
extern void unregister_security(struct security_operations *);

extern void asecfs_exit(void);
extern int asecfs_init(void);

#endif
