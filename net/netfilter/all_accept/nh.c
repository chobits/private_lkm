#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/hardirq.h>

MODULE_LICENSE("GPL");
static struct nf_hook_ops nfho4;
static struct nf_hook_ops nfho3;
static struct nf_hook_ops nfho2;
static struct nf_hook_ops nfho1;
static struct nf_hook_ops nfho0;

unsigned int nfhook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{

	printk(KERN_ALERT "%d\n", hooknum);
	return NF_ACCEPT;
}

#define nf_reg(nfhops, hn, nf, pfnum)	\
	nfhops.hook = nf;		\
	nfhops.hooknum = hn;		\
	nfhops.pf = pfnum;			\
	nf_register_hook(&nfhops)




static int __init nh_init(void)
{
	nf_reg(nfho0, NF_INET_PRE_ROUTING, nfhook, PF_INET);
	nf_reg(nfho1, NF_INET_LOCAL_IN, nfhook,PF_INET);
	nf_reg(nfho2, NF_INET_FORWARD,nfhook ,PF_INET);
	nf_reg(nfho3, NF_INET_LOCAL_OUT ,nfhook,PF_INET);
	nf_reg(nfho4, NF_INET_POST_ROUTING ,nfhook,PF_INET);

	printk(KERN_ALERT "init ok\n");
	return 0;
}

static void __exit nh_exit(void)
{
	nf_unregister_hook(&nfho4);
	nf_unregister_hook(&nfho3);
	nf_unregister_hook(&nfho2);
	nf_unregister_hook(&nfho1);
	nf_unregister_hook(&nfho0);

	printk(KERN_ALERT "exit\n");
}

module_init(nh_init);
module_exit(nh_exit);
