#ifndef MYNL
#define MYNL

#define NETLINK_MY 29
#define REQ_DMEM 0x00000001
#define REQ_SET_BP  0x00000002
#define REQ_UNSET_BP  0x00000003
#define REQ_INFO_BP  0x00000004

struct bp_data {
	unsigned int reqtype;
	unsigned int namelen;
	char name[0];
};

struct dmem_data {
	unsigned int reqtype;
	unsigned int start;
	unsigned int len;
};

struct raw_data {
	unsigned int reqtype;
};

struct nlreq {
	struct nlmsghdr nh;
	union {
		struct bp_data bp;
		struct dmem_data dmem;
		unsigned char pad[128];
	} data;
};

struct optarg_data {
	unsigned int options;
	const char *b_arg;		
	const char *u_arg;		
	const char *m_arg;
	const char *l_arg;
};

#define V_OPTION 0x00000001
#define H_OPTION 0x00000002
#define B_OPTION 0x00000004
#define U_OPTION 0x00000008
#define M_OPTION 0x00000010
#define L_OPTION 0x00000020
#define I_OPTION 0x00000040

#endif /* mynetlink.h */
