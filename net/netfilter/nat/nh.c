#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h> /* ip_hdr */
#include <net/ip.h> /*ip_send_check*/
//#include <linux/socket.h> /* PF_INET */

MODULE_LICENSE("GPL");

void int_to_str(__be32 addr, unsigned char *str)
{
	str[0] = (unsigned int)(addr & 0xff);
	str[1] = (unsigned int)((addr >> 8) & 0xff);
	str[2] = (unsigned int)((addr >> 16) & 0xff);
	str[3] = (unsigned int)((addr >> 24) & 0xff);
}

unsigned int pre_routing_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	unsigned char s[4],d[4];
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;
	//ip->saddr = *(unsigned int *)"\x7f\x0\x0\x2";
	ip->saddr = *(unsigned int *)"\xa\x0\x10\x2";
	ip_send_check(ip);
	sk->local_df = 1;

	int_to_str(ip->saddr, s);
	int_to_str(ip->daddr, d);
	printk(KERN_ALERT"pre routing:%u.%u.%u.%u:->%u.%u.%u.%u",
				s[0], s[1], s[2], s[3],
				d[0], d[1], d[2], d[3]);	

	return NF_ACCEPT;
}

unsigned int local_in_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	unsigned char s[4],d[4];
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;

	int_to_str(ip->saddr, s);
	int_to_str(ip->daddr, d);
	printk(KERN_ALERT"local in:%u.%u.%u.%u:->%u.%u.%u.%u",
				s[0], s[1], s[2], s[3],
				d[0], d[1], d[2], d[3]);	
	return NF_ACCEPT;
}

unsigned int forward_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	unsigned char s[4],d[4];
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;

	int_to_str(ip->saddr, s);
	int_to_str(ip->daddr, d);
	printk(KERN_ALERT"forward:%u.%u.%u.%u:->%u.%u.%u.%u",
				s[0], s[1], s[2], s[3],
				d[0], d[1], d[2], d[3]);	
	return NF_ACCEPT;
}

unsigned int local_out_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	unsigned char s[4],d[4];
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;

	int_to_str(ip->saddr, s);
	int_to_str(ip->daddr, d);
	printk(KERN_ALERT"local out:%u.%u.%u.%u:->%u.%u.%u.%u",
				s[0], s[1], s[2], s[3],
				d[0], d[1], d[2], d[3]);	
	return NF_ACCEPT;
}

unsigned int post_routing_hook(unsigned hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sk = skb;	
	struct iphdr *ip;
	unsigned char s[4],d[4];
	if (sk == NULL || (ip = ip_hdr(sk)) == NULL)
		return NF_ACCEPT;

	int_to_str(ip->saddr, s);
	int_to_str(ip->daddr, d);
	printk(KERN_ALERT"post routing:%u.%u.%u.%u:->%u.%u.%u.%u",
				s[0], s[1], s[2], s[3],
				d[0], d[1], d[2], d[3]);	
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
