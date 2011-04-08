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

/*
static struct nf_hook_ops hook_ops = {
	.owner = THIS_MODULE,
	.hook = get_icmp,
	.pf = PF_INET,
	.hooknum = NF_IP_PRE_ROUTING,
	.priority = NF_IP_PRI_FILTER - 1,
}*/
static int pid;

void nlcall(struct sk_buff *__skb) 
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char data[128];
	data[119] = '\0';
	
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
		
		/*
		 * H E is from my own user program! 
		 */
		if (data[0] == 'H') {
			pid = nlh->nlmsg_pid;
			printk(KERN_ALERT "received message from process %d\n", pid);
			printk(KERN_ALERT "message:[%s]\n", data);
		}
		else if (data[0] == 'E') {
			pid = 0;
		}
		else {
			printk(KERN_ALERT "unknown protocol\n");
		}
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
