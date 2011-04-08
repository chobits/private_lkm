#ifndef __KND_H 
#define __KND_H

#define KNDLINK 29

#define REQ_DMEM	0x00000001
#define REQ_SET_BP	0x00000002
#define REQ_UNSET_BP	0x00000003
#define REQ_INFO_BP	0x00000004

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


#endif /* knd.h */
