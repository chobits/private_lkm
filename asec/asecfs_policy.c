/*
 * for linux 2.6.36
 */
#include "asec.h"
#include "iostream.h"

#define PHASH_BITS 6
#define PHASH_SIZE (1 << PHASH_BITS)
#define PHASH_MASK (PHASH_SIZE - 1)
#define PTABLE_SIZE PHASH_SIZE

#define POLICY_DENY_READ	0x00000001
#define	POLICY_DENY_WRITE	0x00000002
#define POLICY_DENY_EXEC	0x00000004
#define POLICY_DENY_ACCESS	0x00000008
#define POLICY_HIDE_FILE	0x00000010
#define POLICY_HIDE_CONTENT	0x00000020

struct policy_entry {
	unsigned char *path;	/* absolute path name */
	unsigned int len;	/* path length */
	unsigned long ino;	/* file inode number */
	unsigned int hash;	/* path full name hash value */
	unsigned int mode;	/* core policy on path */
	unsigned int supermode;	/* superuser ignores this policy, setting 1 */
	unsigned int delete;	/* for lazy delete */
	struct list_head hlist;	/* use hash(full name hash value) as index */
	struct list_head list;
	void *private;		/* save f_op->open for hide content */
};

/* for quick searching */
static struct list_head policy_table[PTABLE_SIZE];
static DEFINE_SPINLOCK(ptable_lock);	/* protect policy_table */

static LIST_HEAD(policy_list);
static DEFINE_SPINLOCK(plist_lock);	/* protect policy_list */

static atomic_t policy_users = ATOMIC_INIT(0);


/* policy hash value */
static inline unsigned int phash(unsigned char *name, int len, unsigned long ino)
{
	/*
	   return full_name_hash(name, len) & PHASH_MASK;
	   */
	return (ino & PHASH_MASK);
}

struct policy_entry *hash_policy(char *path, unsigned long ino)
{
	struct policy_entry *pe, *found = NULL;
	struct list_head *head;

	spin_lock(&ptable_lock);
	/* init hash list head */
	head = &policy_table[phash(path, 0, ino)];
	list_for_each_entry(pe, head, hlist) {
		/* found it */
		if (pe->ino == ino) {
			found = pe;
			break;
		}
	}
	spin_unlock(&ptable_lock);

	return found;
}

#define FILE_ISDIR(file) (S_ISDIR(file->f_path.dentry->d_inode->i_mode))
#define FILE_ISREG(file) (S_ISREG(file->f_path.dentry->d_inode->i_mode))

static ssize_t hide_content_read(struct file *filp, char __user *buf,
		size_t count, loff_t *pos)
{
	/* read nothing(hide its content) */
	//asecinfo("read nothing");
	int total;
	total = i_size_read(filp->f_path.dentry->d_inode);
	total -= *pos;
	if (total > count)
		total = count;
	*pos += total;
	return total;
}

/* 
 * ugly & fucking trick! 
 * 1. cheat gcc skipping const checking
 * 2. clear WP bit of cr0: force writing the read only memory region
 */
void force_replace(void *old, void *new)
{
	unsigned int origcr0, tmp;
	/* get orignal cr0 */
	asm volatile ("movl %%cr0, %%eax": "=a"(origcr0));
	/* clear WP bit */
	tmp = origcr0 & 0xfffeffff;
	asm volatile ("movl %%eax, %%cr0":: "a"(tmp));

	/* replace */
	*(unsigned long *)old = (unsigned long)new;

	/* restore orignal cr0 */
	asm volatile ("movl %%eax, %%cr0":: "a"(origcr0));
}

static int hide_content_open(struct inode *inode, struct file *file)
{
	int (*open)(struct inode *, struct file *);
	int ret;
	/* 
	 * This dummy open is called when file try to open via its own
	 * open method.
	 * We let it open, and then replace its read method instead of
	 * `hide_content_read`
	 */
	if (!file->private_data) {
		asecerr("private (policy entry) of file disappears!");
		return -EIO;
	}
	open = (typeof(open))file->private_data;
	/* should this happen? */
	if (!open) {
		asecerr("original open method of file disappears!");
		return 0;
	}
	/* orignal open */
	ret = open(inode, file);
	/* error open, we return its errno directly */
	if (ret)
		return ret;
	/* replace hooking */
	if (!file->f_op) {
		asecerr("original open method of file disappears!");
		return 0;
	}
	force_replace(&file->f_op->read, hide_content_read);
	force_replace(&file->f_op->open, open);
	return 0;
}

void policy_hide_content(struct file *filp, struct policy_entry *pe)
{
	if (!filp->f_op)
		return;
	force_replace(&filp->f_op->read, hide_content_read);
	return;
	/* easy way: not private open method, just replace its read method */
	if (!filp->f_op->open) {
		/* regular file: hide its internal content */
		if (!FILE_ISDIR(filp)) {
			force_replace(&filp->f_op->read, hide_content_read);
		}
		/* dir: hide its internal files */

	} else {
		/* 
		 * hard way: filp has its own open method, which maybe set its own
		 * read method after dentry open hook. So we should replace open
		 * method now!
		 */
		if (!FILE_ISDIR(filp)) {
			/* save its original open method pointer */
			/* 
			 * in first dentry_open,
			 * we assert file->private is NULL
			 */
			filp->private_data = filp->f_op->open;
			force_replace(&filp->f_op->open, hide_content_open);
		}
	}
}

int request_for_chdir(struct inode *inode)
{
	struct policy_entry *pe;
	pe = hash_policy(NULL, inode->i_ino);
	if (!pe || pe->delete)
		return 0;
	if (pe->supermode && current_uid() == 0)
		return 0;
	if (pe->mode & POLICY_DENY_ACCESS)
		return -EPERM;
	return 0;
}

/* policy checking of file hook */
int request_for_policy(struct file *filp)
{
	struct policy_entry *pe;
	unsigned long ino;
	fmode_t mode;
	/* get inode number and mode */
	/* Are there some apis for getting inode number? */
	ino = filp->f_path.dentry->d_inode->i_ino;
	mode = filp->f_mode;
	/* get existed policy entry */
	pe = hash_policy(NULL, ino);
	/* no policy and deleted policy will permit file open */
	if (!pe || pe->delete)
		return 0;
	/* when supermode is set, permit super user open file */
	if (pe->supermode && current_uid() == 0)
		return 0;
	/* execve permittion checking */
	if (current->in_execve) {
		if (pe->mode & POLICY_DENY_EXEC)
			return -EPERM;
		return 0;
	}
	/* read permittion checking */
	if ((mode & FMODE_READ) && (pe->mode & POLICY_DENY_READ))
		return -EPERM;
	/* write permittion checking */
	if ((mode & FMODE_WRITE) && (pe->mode & POLICY_DENY_WRITE))
		return -EPERM;
	if (pe->mode & POLICY_HIDE_CONTENT)
		policy_hide_content(filp, pe);

	/* others for permitting file open */
	return 0;
}

/* 
 * donot lock p[list|table]_lock 
 * list traversing for safe
 */
void policy_delete_unlock(struct policy_entry *pe)
{
	list_del(&pe->list);
	list_del(&pe->hlist);
	kfree(pe->path);
	kfree(pe);
}

void policy_insert(struct policy_entry *pe)
{
	/* policy list */
	spin_lock(&plist_lock);
	list_add(&pe->list, &policy_list);
	spin_unlock(&plist_lock);
	/* policy hash table */
	spin_lock(&ptable_lock);
	list_add(&pe->hlist, &policy_table[pe->hash]);
	spin_unlock(&ptable_lock);
}

/* alloc and init policy entry and insert into list & hashtbl */
struct policy_entry *new_policy(char *path, unsigned int mode,
		unsigned supermode, unsigned long ino)
{
	struct policy_entry *pe;
	pe = kzalloc(sizeof(*pe), GFP_NOFS);
	if (!pe)
		return NULL;
	INIT_LIST_HEAD(&pe->list);
	INIT_LIST_HEAD(&pe->hlist);
	pe->mode = mode;
	pe->supermode = supermode;
	pe->ino = ino;
	pe->path = kstrdup(path, GFP_KERNEL);
	pe->len = strlen(path);
	pe->hash = phash(pe->path, pe->len, ino);
	policy_insert(pe);
	return pe;
}

void ior_policy_mode(struct iostream *fio, unsigned int mode)
{
	if (mode & POLICY_DENY_READ)
		ior_string(fio, "DENY_READ ");
	if (mode & POLICY_DENY_WRITE)
		ior_string(fio, "DENY_WRITE ");
	if (mode & POLICY_DENY_EXEC)
		ior_string(fio, "DENY_EXEC ");
	if (mode & POLICY_HIDE_FILE)
		ior_string(fio, "HIDE_FILE ");
	if (mode & POLICY_HIDE_CONTENT)
		ior_string(fio, "HIDE_CONTENT ");
	if (mode & POLICY_DENY_ACCESS)
		ior_string(fio, "DENY_ACCESS");
}

/* write policy to stream */
void policy_io_read(struct iostream *fio)
{
	struct policy_entry *pe = NULL;
	if (ior_eof(fio))
		return;
	if (fio->loglist == NULL)
		fio->loglist = policy_list.next;
	else if (ior_flush(fio))
		return;

	spin_lock(&plist_lock);
	while (ior_flush(fio) == 0 /* if user buf is full, break loop */
			&& fio->loglist != &policy_list) {
		pe = list_entry(fio->loglist, struct policy_entry, list);
		ior_string(fio, pe->path);
		ior_space(fio);
		ior_policy_mode(fio, pe->mode);
		if (pe->supermode)
			ior_string(fio, "supermode ");

		ior_newline(fio);
		fio->loglist = fio->loglist->next;
	}
	spin_unlock(&plist_lock);
	if (fio->loglist == &policy_list)
		ior_set_eof(fio);
}

static ssize_t policy_read(struct file *filp, char __user *buf,
		size_t count, loff_t *pos)
{
	struct iostream *fio = filp->private_data;
	/* init iostream */
	if (!fio || !fio->read_buf)
		return -EIO;
	fio->user_read_buf = buf;
	fio->user_read_len = count;

	/* real policy read method */
	policy_io_read(fio);

	return fio->user_read_buf - buf;
}

/*
 * match first strlen(match) string of *p with string match
 * if matched, return 1 and skip match string in *p,
 * otherwise for 0
 */
int string_start_from(char **p, char *match)
{
	char *str;
	/* invalid string? */
	if (!p || !*p || !match || !*match)
		return 0;
	str = *p;
	while (*str && *match && *str == *match) {
		match++;
		str++;
	}
	/* if matched, skip the matched string in (*p) */
	if (!*match) {
		*p = str;
		return 1;
	}
	return 0;
}

static inline int iswhitespace(char c)
{
	return (c == ' ') || (c == '\t');
}

/* low level auxiliary function: assert(p && *p) */
void skip_whitespace(char **p)
{
	char *str = *p;
	while (iswhitespace(*str))
		str++;
	*p = str;
}

unsigned int policy_is_delete(char **p)
{
	skip_whitespace(p);
	return string_start_from(p, "delete");
}

/* parsing policy mode string */
unsigned int policy_mode(char **p)
{
	unsigned int mode = 0;
	skip_whitespace(p);
	if (string_start_from(p, "DENY_READ"))
		mode = POLICY_DENY_READ;
	else if (string_start_from(p, "DENY_WRITE"))
		mode = POLICY_DENY_WRITE;
	else if (string_start_from(p, "DENY_EXEC"))
		mode = POLICY_DENY_EXEC;
	else if (string_start_from(p, "DENY_ACCESS"))
		mode = POLICY_DENY_ACCESS;
	else if (string_start_from(p, "HIDE_FILE"))
		mode = POLICY_HIDE_FILE;
	else if (string_start_from(p, "HIDE_CONTENT"))
		mode = POLICY_HIDE_CONTENT;
	return mode;
}

/* get inode number and check its validity */
int policy_ino(unsigned long *ino, char **p)
{
	char *str;
	unsigned int number = 0;
	skip_whitespace(p);
	str = *p;
	if (!*str)
		return -1;
	while (isdigit(*str)) {
		number = 10 * number + (*str - '0');
		str++;
	}
	/* no number string? */
	if (str == *p)
		return -1;
	*p = str;
	*ino = number;
	return 0;
}

/* get path and check its validity */
int policy_path(char *path, char **p)
{
	char *str;
	int lenlimit = 128;
	skip_whitespace(p);
	str = *p;
	/* invalid path or string *p is end */
	if (*str != '/')
		return -1;
	while (--lenlimit && *str && !iswhitespace(*str))
		*path++ = *str++;
	if (!lenlimit)
		return -1;
	*path = '\0';
	*p = str;
	return 0;
}

unsigned int policy_super_mode(char **p)
{
	skip_whitespace(p);
	return string_start_from(p, "supermode");
}

void debug_policy(char *policy, struct policy_entry *pe)
{
	printk(KERN_ALERT "policy:%s\n", policy);
	if (!pe) {
		printk(KERN_ALERT "----[debug NULL policy entry]----\n");
		return;
	}
	printk(KERN_ALERT "----[debug policy entry]------\n");
	printk(KERN_ALERT "path: %s len: %d\n", pe->path, pe->len);
	printk(KERN_ALERT "hash: %u\n", pe->hash);
	printk(KERN_ALERT "inode number: %ld\n", pe->ino);
	printk(KERN_ALERT "mode: %u\n", pe->mode);
	printk(KERN_ALERT "supermode: %u\n", pe->supermode);
	printk(KERN_ALERT "delete: %u\n", pe->delete);
	printk(KERN_ALERT "----[       end        ]------\n");

}

void policy_handle(char *policy, int len)
{
	char path[128];
	char *p = policy;
	struct policy_entry *pe = NULL;
	unsigned int tmp = 0, mode = 0, super = 0;
	unsigned long ino = 0;
	int delete = 0;

	/* parse the policy string */
	/* delete */
	delete = policy_is_delete(&p);
	/* path */
	if (policy_path(path, &p) == -1) {
		asecerr("invalid policy `%s`: path is error", policy);
		return;
	}
	/* inode number */
	if (policy_ino(&ino, &p) == -1) {
		asecerr("invalid policy `%s`: ino is error", policy);
		return;
	}
	/* mode */
	while ((tmp = policy_mode(&p)) != 0)
		mode |= tmp;
	/* super mode */
	super = policy_super_mode(&p);
	/* prepare finding the existed policy entry */
	pe = hash_policy(path, ino);
	/* real handle */
	if (delete) {
		if (!pe) {
			asecerr("delete nonexist policy");
			return;
		}
		pe->mode &= mode ? ~mode : 0;
		pe->supermode &= ~super;
	} else if (!pe) {
		/* alloc new policy and insert it int hashtbl & list */
		pe = new_policy(path, mode, super, ino);
	} else {
		pe->mode |= mode;
		pe->supermode |= super;
	}

	/* lazy delete */
	if (pe->mode || pe->supermode)
		pe->delete = 0;
	else
		pe->delete = 1;
	debug_policy(policy, pe);
}

static ssize_t policy_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *pos)
{
	struct iostream *fio = filp->private_data;
	int linelen;
	char *line;
	if (!fio || !fio->write_buf)
		return -EIO;
	fio->user_write_buf = buf;
	fio->user_write_len = count;

	while (fio->user_write_len) {
		linelen = iow_getline(fio, &line);
		/* error ? */
		if (linelen < 0)
			return linelen;
		else if (linelen == 0)
			break;
		policy_handle(line, linelen);
		iow_sync(fio);
	}
	return count - fio->user_write_len;
}

static int policy_open(struct inode *inode, struct file *file)
{
	int err = -EACCES;
	/* Only one user can handle policy */
	if (atomic_read(&policy_users) == 0 &&
			(err = init_iostream(file)) == 0)
		atomic_inc(&policy_users);
	return err;
}

static int policy_close(struct inode *inode, struct file *file)
{
	if (atomic_read(&policy_users))
		atomic_dec(&policy_users);
	else
		return -EACCES;
	fini_iostream(file);
	return 0;
}

const struct file_operations policy_ops = {
	.open = policy_open,
	.release = policy_close,
	.read = policy_read,
	.write = policy_write,
};

/* asec system auxiliary init method */
void init_policy(void)
{
	int i;
	for (i = 0; i < PTABLE_SIZE; i++)
		INIT_LIST_HEAD(&policy_table[i]);
	asecinfo("init policy table ok");
}

void exit_policy(void)
{
	/*
	 * donnot declare temp parameter as `__safe`, which conflicts with
	 * its same name macro in compiler.h 
	 */
	struct policy_entry *pe, *__tmp;

	spin_lock(&plist_lock);
	list_for_each_entry_safe(pe, __tmp, &policy_list, list)
		policy_delete_unlock(pe);
	spin_unlock(&plist_lock);
}
