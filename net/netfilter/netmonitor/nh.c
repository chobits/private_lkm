#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h> /* ip_hdr */
//#include <linux/socket.h> /* PF_INET */

MODULE_LICENSE("GPL");
#define IP_FORMAT "%u.%u.%u.%u"
#define IP_NUM(addr)\
	((unsigned char *)&addr)[0],\
	((unsigned char *)&addr)[1],\
	((unsigned char *)&addr)[2],\
	((unsigned char *)&addr)[3]

unsigned int pre_routing_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;

	printk(KERN_ALERT "pre routing:" IP_FORMAT "->" IP_FORMAT"\n",
				IP_NUM(ip->saddr), IP_NUM(ip->daddr));

	return NF_ACCEPT;
}

unsigned int local_in_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;

	printk(KERN_ALERT "local in:" IP_FORMAT "->" IP_FORMAT"\n",
				IP_NUM(ip->saddr),IP_NUM(ip->daddr));
	return NF_ACCEPT;
}

unsigned int forward_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;

	printk(KERN_ALERT "forward:" IP_FORMAT "->" IP_FORMAT "\n",
				IP_NUM(ip->saddr),IP_NUM(ip->daddr));
	return NF_ACCEPT;
}

unsigned int local_out_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;
	printk(KERN_ALERT "local out:" IP_FORMAT "->" IP_FORMAT"\n",
				IP_NUM(ip->saddr),IP_NUM(ip->daddr));

	return NF_ACCEPT;
}

unsigned int post_routing_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;
	printk(KERN_ALERT"post routing:"IP_FORMAT"->"IP_FORMAT"\n",
				IP_NUM(ip->saddr),IP_NUM(ip->daddr));

	return NF_ACCEPT;
}

static struct nf_hook_ops nfho_array[] = {
	{
		.hook = pre_routing_hook,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_INET_PRE_ROUTING,
		.priority = NF_IP_PRI_FIRST,
	},
	{
		.hook = local_in_hook,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP_PRI_FIRST,
	},
	{
		.hook = forward_hook,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_INET_FORWARD,
		.priority = NF_IP_PRI_FIRST,
	},
	{
		.hook = local_out_hook,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_INET_LOCAL_OUT,
		.priority = NF_IP_PRI_FIRST,
	},
	{
		.hook = post_routing_hook,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_INET_POST_ROUTING,
		.priority = NF_IP_PRI_FIRST,
	}
};


static int __init nh_init(void)
{
	int i;
	for (i = 0; i < 5; i++)
		nf_register_hook(nfho_array + i);
	printk(KERN_ALERT "init ok\n");
	return 0;
}

static void __exit nh_exit(void)
{
	int i;
	for (i = 0; i < 5; i++)
		nf_unregister_hook(nfho_array + i);

	printk(KERN_ALERT "exit\n");
}

module_init(nh_init);
module_exit(nh_exit);
