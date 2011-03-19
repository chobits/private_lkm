#include <stdio.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nlproto.h"

int nlswitch(int switchon)
{
	struct {
		struct nlmsghdr nh;
		int data;
	} nlreq;
	struct sockaddr_nl nladdr;
	int nlsock;

	/*
	 * socket create
	 * use our defined net protocol:NETLINK_IKEY
	 */
	nlsock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_IKEY);
	if (nlsock < 0) {
		printf("err socket\n");
		return -1;
	}

	memset(&nlreq, 0, sizeof(nlreq));
	nlreq.nh.nlmsg_len = NLMSG_LENGTH(sizeof(nlreq.data));
	nlreq.nh.nlmsg_pid = getpid();

	nlreq.data = switchon;
	if (send(nlsock, &nlreq, nlreq.nh.nlmsg_len, 0) < 0) {
		printf("err send\n");
		return -1;
	}

	return 0;
}
