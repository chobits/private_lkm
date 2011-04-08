#include <stdio.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mynetlink.h"

int main()
{
	struct {
		struct nlmsghdr nh;
		char data[128];
	} nlreq;
	struct sockaddr_nl nladdr;

	int nlsock;
	/*
	 * socket create
	 */
	nlsock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_MY);
	if (nlsock < 0) {
		printf("err socket\n");
		return -1;
	}

	/*
	 * bind 
	 */ /*
	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_pad = 0;
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();
	nladdr.nl_groups = 0;
	if (bind(nlsock, (struct sockaddr *)&nladdr, sizeof(nladdr)) < 0) {
		printf("err bind\n");
		return -1;
	}*/


	memset(&nlreq, 0, sizeof(nlreq));
	nlreq.nh.nlmsg_len = NLMSG_LENGTH(sizeof(nlreq.data));
	nlreq.nh.nlmsg_pid = getpid();

	memcpy(nlreq.data, "Hello\0", 6);
	if (send(nlsock, &nlreq, nlreq.nh.nlmsg_len, 0) < 0) {
//	if (sendto(nlsock, &nlreq, nlreq.nh.nlmsg_len, 0,
//		(struct sockaddr *)&nladdr, sizeof(nladdr)) < 0) {
		printf("err send\n");
		return -1;
	}
	printf("user pid: %d\n", getpid());
	return 0;
}
