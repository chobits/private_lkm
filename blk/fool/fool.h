#ifndef MNT_FOOL_H
#define MNT_FOOL_H

#include <linux/posix_types.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>

#define FOOL_MAJOR 250

struct fool_device {
	spinlock_t lock;
	struct gendisk *gd;
	struct request_queue *q;

	struct bio_list bio_list;

	/* internal */
	unsigned int blocksize;
	struct block_device *blkdev;
	unsigned int state;
	unsigned int flags;
	wait_queue_head_t wait;
	struct task_struct *thread;
	struct file *bound_file;
};

enum {
	FOOL_UNBOUND,
	FOOL_BOUND,
};

#define FOOL_FLAGS_READ_ONLY 1

#define FOOL_SET_FD		0x4C00
#define FOOL_CLR_FD		0x4C01
#define FOOL_SET_STATUS		0x4C02
#define FOOL_SET_STATUS64	0x4C05

enum {
	LO_FLAGS_READ_ONLY  = 1,
	LO_FLAGS_USE_AOPS   = 2,
	LO_FLAGS_AUTOCLEAR  = 4, /* New in 2.6.25 */
};

#endif /* MNT_FOOL_H */
