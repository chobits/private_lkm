#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/inet.h>
#include <linux/ip.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include "knd.h"

extern int set_bp(char *);
extern void unset_bp(char *);
extern void info_bp(void);

unsigned int size_inside_page(unsigned int start, unsigned int count)
{
	unsigned int sz;
	sz = 4096 - (start & 0xfff);
	return min(sz, count);	
}
void put_ptr(unsigned int vs, void *addr)
{
	unsigned long start;
	start = (unsigned long)((vs - 0xc0000000) & 0xfffff000);

	if (page_is_ram(start >> 12))
		return ;

	iounmap((void __iomem *)((unsigned long)addr & 0xfffff000));	
	return;
}

void *get_ptr(unsigned int vs)
{
	void *addr;
	unsigned long start;
	start = (unsigned long)((vs - 0xc0000000) & 0xfffff000);
	if (page_is_ram(start >> 12))
		return (void *)vs;
	addr = (void __force *)ioremap_cache(start, 4096);
	if (addr)
		addr = (void *)((unsigned long)addr | (vs & 0xfff));
	return addr;
	
}

int display_data(unsigned int start, unsigned int count)
{
	unsigned char *p;
	/*
	 * read:	all read data
	 * sz:  	one time read data
	 * */
	unsigned int read, sz, i, ps, s;	
	unsigned char buf[128];
	unsigned char *pb;
	
	
	if (start < 0xc0000000) {
		printk(KERN_ALERT "address 0x%x request err\n", start);
		return -1;
	}

	s = start;
	read = 0;
	while (count) {
		sz = size_inside_page(s, count);
		p = get_ptr(s);	
		if (!p) {
			printk(KERN_ALERT "cannot read\n");
			return -1;
		}
		for (i = 0, ps = s, pb = buf; i < sz / 4; i++, ps += 4) {
			if ((i & 0x3) == 0x0) {
				snprintf(pb, 16, "[k]0x%08x: ", ps);
				pb += 15;
			}
			snprintf(pb, 10, "%08x ", ((unsigned int *)p)[i]);
			pb += 9;
			if ((i & 0x3) == 0x3) {
				printk(KERN_ALERT "%s\n", buf);
				pb = buf;
			}
		}
		if (pb - buf)
			printk(KERN_ALERT "%s\n", buf);
		

		put_ptr(s, p);	

		s += sz;	
		read += sz;	
		count -= sz;
	}
	return read;
}

void do_handle_req(const struct raw_data *data)
{
	if (data->reqtype == REQ_DMEM) {
		struct dmem_data *p = (struct dmem_data *)data;
		display_data(p->start, p->len);
	}
	else if (data->reqtype == REQ_INFO_BP) {
		info_bp();
	}
	else if (data->reqtype == REQ_SET_BP) {
		struct bp_data *p = (struct bp_data *)data;
		set_bp(p->name);
	}
	else if	(data->reqtype == REQ_UNSET_BP) {
		struct bp_data *p = (struct bp_data *)data;
		unset_bp(p->name);	
	}
	else {
		printk(KERN_ALERT "unknown request type\n");
	}
}

void nlcall(struct sk_buff *__skb) 
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	unsigned char data[128];
	struct raw_data *p;
	
	
	printk(KERN_ALERT "[+]netlink start\n");
	/*
	 * get true skb
	 */
	skb = skb_get(__skb);

	/*
	 * length of actual data must be bigger than
	 * NLMSG_LENTH
	 */
	if (skb->len >= NLMSG_SPACE(0)) {
		/*
		 * skb->data
		 */
		nlh = nlmsg_hdr(skb);	
		/*
		 * nlh + NLMSG_LENTH -->data
		 */
		memcpy(data, NLMSG_DATA(nlh), sizeof(data));
		
		p = (struct raw_data *)data;	
		
		do_handle_req(p);

		kfree_skb(skb);
	}
	else {
		printk(KERN_ALERT "data len is short!\n");

	}
}

static struct sock *sp;

static int __init knd_init(void)
{
	/*
 	 * Don't know init_net:-(
 	 * And I set mutex to NULL
 	 */
	sp = netlink_kernel_create(&init_net, KNDLINK, 0,
					 nlcall, NULL, THIS_MODULE);
	if (sp == NULL)	{
		printk(KERN_ALERT "nlcreate error");
		return -1;
	}
		
	printk(KERN_ALERT "init ok!\n");
	return 0;
}

static void __exit knd_exit(void)
{
	/*
	 * sock_release (struct socket*)
	 */
	if (sp != NULL)	
		sock_release(sp->sk_socket);
	printk(KERN_ALERT "exit!\n");

}

module_init(knd_init);
module_exit(knd_exit);
MODULE_LICENSE("GPL");
