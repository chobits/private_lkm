#include <stdio.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "knd.h"

void err_quit(const char *str)
{
	if (str)
		fprintf(stderr, "(u-knd)%s\n", str);
	else
		fprintf(stderr, "(u-knd)unkown err\n");
	exit(EXIT_FAILURE);
}

unsigned int atou(const char *str)
{
	unsigned int ret, add;
	int i, j, mul;
	const char *p;

	ret = 0;
	p = str;
	if (str[0] == '0' && str[1] == 'x' && strlen(str) <= 10) {
		p += 2;
		mul = 16;
	}
	else if (strlen(str) <= 10) {
		mul = 10;
	}
	else {
		err_quit("start address too long");
	}
	

	for (ret = i = 0; p[i]; i++) {
		if (mul == 16) {
			if (p[i] >= 'a' && p[i] <= 'f')
				add = p[i] - 'a' + 10;	
			else if (p[i] >= 'A' && p[i] <= 'F')
				add = p[i] - 'A' + 10;
			else if (p[i] >= '0' && p[i] <= '9')
				add = p[i] - '0';
			else 
				err_quit("invalid start address");
			
		}
		else if (mul == 10) {
			if (p[i] >= '0' && p[i] <= '9')
				add = p[i] - '0';
			else 
				err_quit("invalid start address");
		}
		ret = ret * mul + add;
	}
	return ret;
}

void version(void)
{
	fprintf(stderr,
		"version: 0.00\n"
		"Knd is not debugger, but it can help you develop kernel!\n\n"
		);
	exit(0);
}

void usage(void)
{
	fprintf(stderr, 
		"Knd is not debugger, but it can help you develop kernel!\n\n"
		"Usage: knd [OPTION] ...\n"
		"\t-b funcname      Set breakpoint in func's beginning.\n"
		"\t                 And when func start, it can dump strace.\n"
		"\t-u funcname      Unset breakpoint.\n"
		"\t-i               Info of all breakpoint\n"
		"\t-m start         Disasm the memory from start."
					"(default len is 4)\n"
		"\t-l len           Set len of disasmmed memory.\n\n"
		"Report bugs to <wangxiaochen0@gmail.com>\n"
		);
	exit(1);
}

static struct nlreq nlreq;

void send_req(void)
{
	int nlsock;

	/*
	 * socket create
	 */
	nlsock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_MY);
	if (nlsock < 0)
		err_quit("err socket");

	nlreq.nh.nlmsg_len = NLMSG_LENGTH(sizeof(nlreq.data));
	nlreq.nh.nlmsg_pid = getpid();

	if (send(nlsock, &nlreq, nlreq.nh.nlmsg_len, 0) < 0)
		err_quit("err send");
}

void debug_dmem(struct dmem_data *p)
{
	if (p->start + p->len < p->start)
		err_quit("debug_dmem:integer overflow!");

	if (0xffffffff - p->start <= 1 + p->len)
		err_quit("debug_dmem:range too long(beyond 0xffffffff)");

	fprintf(stderr, "(u)start address: 0x%x len:%x\n",
							p->start, p->len);
}

int handle_arg(int argc, char **argv)
{
	int c, quik_out;
	struct optarg_data oa;

	/* arguments collect */
	memset(&oa, 0x0, sizeof(oa));
	quik_out = 0;	
	opterr = 0;
	while ((c = getopt(argc, argv, "ib:u:m:l:vh")) != -1) {
		switch (c) {
		case 'b':
			oa.options |= B_OPTION;	
			oa.b_arg = optarg;
			break;
		case 'u':
			oa.options |= U_OPTION;	
			oa.u_arg = optarg;
			break;
		case 'i':
			oa.options |= I_OPTION;
			break;
		case 'm':
			oa.options |= M_OPTION;	
			oa.m_arg = optarg;
			break;
		case 'l':
			oa.options |= L_OPTION;	
			oa.l_arg = optarg;
			break;

		case 'v':
			oa.options |= V_OPTION;	
			quik_out = 1;
			break;
		case 'h':
		case '?':
			oa.options |= H_OPTION;	
			quik_out = 1;
			break;
		default:
			/* You wont actually get here. */
			break;
		}
		if (quik_out)
			break;
	}
	if (optind > argc)
		err_quit("Expected argument after options\n");

	/* arguments handle */
	if (oa.options & V_OPTION)
		version();
	if (oa.options & H_OPTION)
		usage();
	
	memset(&nlreq, 0x0, sizeof(nlreq));		
	if (oa.options & I_OPTION) {
		struct raw_data *p;
		p = (struct raw_data *)nlreq.data.pad;	
		p->reqtype = REQ_INFO_BP;
	}
	else if (oa.options & B_OPTION) {
		struct bp_data *p;
		p = &nlreq.data.bp;
		p->reqtype = REQ_SET_BP;
		p->namelen = strlen(oa.b_arg);
		strncpy(p->name, oa.b_arg, p->namelen);
		p->name[p->namelen] = '\0';
	}
	else if (oa.options & U_OPTION) {
		struct bp_data *p;
		p = &nlreq.data.bp;
		p->reqtype = REQ_UNSET_BP;
		p->namelen = strlen(oa.u_arg);
		strncpy(p->name, oa.u_arg, p->namelen);
		p->name[p->namelen] = '\0';
	}
	else if (oa.options & M_OPTION) {
		struct dmem_data *p;
		p = &nlreq.data.dmem;
		p->reqtype = REQ_DMEM;
		p->start = atou(oa.m_arg);
		if (oa.options & L_OPTION) 
			p->len = atoi(oa.l_arg);
		else
			p->len = 4;
		/* Exit, if err! */
		debug_dmem(p);		
	}
	else
		err_quit("unknown or error arg options");

	send_req();
	return 0;
}

int main(int argc, char **argv)
{

	if (argc < 2) 
		usage();
	/* 
	 * success return 
	 * err handle before returning
	 */
	return handle_arg(argc, argv);	
}
