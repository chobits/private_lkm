#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/net.h>
#include <net/sock.h>


MODULE_LICENSE("GPL");

#define dbg(fmt, arg...) printk(KERN_ALERT "%s "fmt"\n", __FUNCTION__, ##arg)

#define PF_RAW raw_family
static int raw_family;
/* grep net_families /proc/kallsyms | cut -d ' ' -f 1 */
static struct net_proto_family **net_families = (struct net_proto_family **)0xc09a4760;

static struct proto raw_proto = {
	.name = "NET_RAW",
	.owner = THIS_MODULE,
	.obj_size = sizeof(struct sock),
};

/* hack to get an unused family number */
static int get_family(void)
{
	int family;
	for (family = 0; family < NPROTO; family++) {
		if (!net_families[family])
			return family;
	}
	return -1;
}

/* release (struct sock *)sk */
static int raw_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	if (!sk)
		return 0;
	/* sk_common_release need sk->sk_prot */
	/* sk_common_release(sk);             */
	sock_put(sk);
	sock->sk = NULL;
	return 0;
}

static int raw_sendmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t size)
{
	dbg("");
	return -ENOMEM;
}

static int raw_recvmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t size, int flags)
{
	dbg("");
	return -ENOMEM;
}

/*
 * reference to inet_sockraw_ops
 *
 * sys_sendto                                              fd
 *  -> sock_sendmsg                                        socket
 *   -> __sock_sendmsg  (sock->ops->sendmsg)               socket
 *    -> raw_sendmsg                                       socket
 *     (-> sk->sk_prot->sendmsg , this can be dropped!)    sock
 */
static struct proto_ops raw_ops = {
	.family = 0,
	.owner = THIS_MODULE,
	.release = raw_release,	/* real release */
	.bind = sock_no_bind,
	.connect = sock_no_connect,
	.socketpair = sock_no_socketpair,
	.accept = sock_no_accept,
	.getname = sock_no_getname,
	.poll = sock_no_poll,
	.ioctl = sock_no_ioctl,
	.listen = sock_no_listen,
	.shutdown = sock_no_shutdown,
	.setsockopt = sock_no_setsockopt,
	.getsockopt = sock_no_getsockopt,
	.sendmsg = raw_sendmsg,	/* real sendmsg */
	.recvmsg = raw_recvmsg,	/* real recvmsg */
	.mmap = sock_no_mmap,
	.sendpage = sock_no_sendpage,
};

/*
 * sys_socket -> sock_create -> __sock_create -> this
 */
static int raw_create(struct net *net, struct socket *sock, int protocol, int kern)
{
	struct sock *sk;
	int err;

	/* init sock struct */
	err = -ENOBUFS;
	sk = sk_alloc(net, PF_RAW, GFP_KERNEL, &raw_proto);
	if (sk == NULL)
		goto out;
	sock->ops = &raw_ops;
	sock_init_data(sock, sk);
	sk->sk_family = PF_RAW;
	err = 0;
out:
	return err;
}

static struct net_proto_family raw_family_ops = {
	.family = 0,
	.create = raw_create,
	.owner = THIS_MODULE,
};

static int __init skraw_init(void)
{
	int err;
	/* register proto: dummy proto */
	err = proto_register(&raw_proto, 0);
	if (err) {
		dbg("Register proto err");
		return -1;
	}

	/* register socket */
	PF_RAW = raw_family_ops.family = get_family();
	raw_ops.family = PF_RAW;
	if (raw_family_ops.family == -1) {
		proto_unregister(&raw_proto);
		dbg("No empty family can be used!");
		return -1;
	}

	err = sock_register(&raw_family_ops);
	if (err) {
		proto_unregister(&raw_proto);
		dbg("Register socket err");
		return -1;
	}

	/* usermode code: socket(PF_RAW, .., ..) */
	printk(KERN_ALERT "register socket family: %d\n", PF_RAW);
	return 0;
}

static void __exit skraw_exit(void)
{
	sock_unregister(raw_family_ops.family);
	printk(KERN_ALERT "exit\n");
}

module_init(skraw_init);
module_exit(skraw_exit);
