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
#include "mynetlink.h"

MODULE_LICENSE("GPL");

struct data_t {
	unsigned int start;
	unsigned int len;
};

void display_data(unsigned int start, unsigned int len)
{
	unsigned int *p;
	unsigned int i, l;

	if (start < 0xc0000000) {
		printk(KERN_ALERT "address 0x%x request err\n", start);
		return ;
	}
	p = (unsigned int *)(start & 0xfffffffc);
	printk(KERN_ALERT "[+]start:%x len:%x \n", start, len);
	l = ((start & 0x3) + len + 3) / 4;
	for (i = 0; i < l; i++, p += 4) {
		printk(KERN_ALERT "0x%08x : %08x %08x %08x %08x\n",
				(unsigned int)p, p[0], p[1], p[2], p[3]);
	}
}


void nlcall(struct sk_buff *__skb) 
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char data[128];
	struct data_t *p;
	
	
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
		
		p = (struct data_t *)data;	
		
		display_data(p->start, p->len);


		kfree_skb(skb);

	}
	else {
		printk(KERN_ALERT "data len is short!\n");

	}
}

static struct sock *sp;

static int __init hello_init(void)
{
	/*
 	 * Don't know init_net:-(
 	 * And I set mutex to NULL
 	 */
	sp = netlink_kernel_create(&init_net, NETLINK_MY, 0,
					 nlcall, NULL, THIS_MODULE);
	if (sp == NULL)	{
		printk(KERN_ALERT "nlcreate error");
		return -1;
	}
		

	printk(KERN_ALERT "init ok!\n");
	return 0;
}

static void __exit hello_exit(void)
{
	/*
	 * sock_release (struct socket*)
	 */
	if (sp != NULL)	
		sock_release(sp->sk_socket);
	printk(KERN_ALERT "exit!\n");

}

module_init(hello_init);
module_exit(hello_exit);
