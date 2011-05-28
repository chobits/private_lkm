#ifndef __IOSTREAM_H
#define __IOSTREAM_H

#define INTERNAL_BUFSIZE 4096	/* internal i/o buffer size */

#define IO_READ_EOF	1
#define IO_WRITE_EOF	2

struct iostream {
	/* user buf */
	const char __user *user_write_buf;
	int user_write_len;
	char __user *user_read_buf;
	int user_read_len;
	/* internal buf */
	char *read_buf;		/* internal read buffer */
	int rhead;
	int rtail;
	char *write_buf;	/* internal write buffer */
	int whead;
	int wtail;
	/* read log */
	unsigned int eof;
	struct list_head *loglist;	/* for policy list traversing */
};

extern int init_iostream(struct file *file);
extern void fini_iostream(struct file *file);
extern void ior_set_eof(struct iostream *io);
extern int ior_eof(struct iostream *io);
extern int ior_flush(struct iostream *io);
extern void ior_string(struct iostream *io, char *p);
extern void ior_space(struct iostream *io);
extern void ior_newline(struct iostream *io);
extern int iow_getline(struct iostream *io, char **linp);
extern void iow_sync(struct iostream *io);

#endif	/* iostream.h */
