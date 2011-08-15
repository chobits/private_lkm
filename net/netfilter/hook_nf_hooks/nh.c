#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h> /* ip_hdr */

MODULE_LICENSE("GPL");
static struct nf_hook_ops nfho;
/* little endian:
 * 0:127
 * 1:0
 * 2:0
 * 3:1 */
unsigned int nfhook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	return NF_ACCEPT;
}

#define nf_reg(nfhops, hn, nf, pfnum)		\
	nfhops.hook = nf;			\
	nfhops.hooknum = hn;/*position*/	\
	nfhops.pf = pfnum; /*family*/		\
	nfhops.priority = NF_IP_PRI_FIRST;	\
	nf_register_hook(&nfhops)
	


static int __init nh_init(void)
{
	struct list_head **nf_hookss;
	struct list_head *p;
	int x,y;
	y = NF_INET_PRE_ROUTING;
	x = PF_INET;
	nf_reg(nfho, y, nfhook, x);
	p = nfho.list.prev;
	
	nf_hookss = (struct list_head **)\
			(p-(x * NF_MAX_HOOKS + y));
	
	printk(KERN_ALERT "0x%x\n", (unsigned int)nf_hookss);
	printk(KERN_ALERT "0x%x\n", (unsigned int)nf_hooks);
	return 0;
}

static void __exit nh_exit(void)
{
	nf_unregister_hook(&nfho);

	printk(KERN_ALERT "exit\n");
}

module_init(nh_init);
module_exit(nh_exit);
