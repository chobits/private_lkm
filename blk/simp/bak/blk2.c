/*
 * a simple block device based on local memory
 * port from LWN.net
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock_types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>	/* blk_queue_logical_block_size */
#include <linux/hdreg.h>	/* hd_geometry */

#define KERNEL_SECTOR_SIZE 512

static struct md_device {
	unsigned int size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} md;


static struct request_queue *mdqueue;
static int blocksize;
static int blocks;
static int major_num;

/* 
 * HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which call this.
 * Tools , like fdisk, will call ioctl.
 */
static int md_getgeo(struct block_device *bdev, struct hd_geometry *hdgeo)
{
	struct md_device *md = bdev->bd_disk->private_data;
	int sectors;
	if (md == NULL) {
		printk(KERN_ALERT "error getgeo\n");
		return -1;
	}	
	sectors = md->size >> 9;
	hdgeo->cylinders = sectors / 4 / 16;
	hdgeo->heads = 4;
	hdgeo->sectors = 16;
	//hdgeo->start = 0;
	return 0;
}

struct block_device_operations md_ops = {
	.owner = THIS_MODULE,
	.getgeo = md_getgeo,
};

static void md_request(struct request_queue *q)
{
	struct request *req;
	unsigned int off;
	unsigned int size;

	req = blk_fetch_request(q);
	while (req != NULL) {
		if (req->cmd_type != REQ_TYPE_FS) {
			printk(KERN_ALERT "unknown request\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}

		/* handle I/O request */
		off = blk_rq_pos(req) << 9;	/* sector size is 512 bytes */
		size = blk_rq_cur_bytes(req);

		if (off + size > md.size) {
			printk(KERN_ALERT "I/O beyond size\n");
			return;
		}

		switch (rq_data_dir(req)) {
			/* write */
			case 1:
				memcpy(md.data + off, req->buffer, size);
				break;

				/* read */
			case 0:
				memcpy(req->buffer, md.data + off, size);
				break;

				/* we known it shouldn't be here */
			default:
				break;
		}

		/* 
		 * param 2--0--sucess end request 
		 * return value --0--request has been done
		 */
		if (__blk_end_request_cur(req, 0) == 0)
			req = blk_fetch_request(q);

	}

}

static int __init md_init(void)
{
	int ret;
	/* init parameters */
	blocksize = 512;
	blocks = 16 * 1024;	/* disk size is 16MB */
	major_num = 0;		/* dynamic alloc device major number */

	/* init memory disk device */
	printk(KERN_ALERT "init md\n");	
	spin_lock_init(&md.lock);
	md.size = blocks * blocksize;
	md.data = vmalloc(md.size);
	memset(md.data, 0x0, 512);
	if (md.data == NULL) {
		printk(KERN_ALERT "vmalloc err\n");
		return -ENOMEM;
	}

	/* init request queue */
	printk(KERN_ALERT "init request queue\n");	
	mdqueue = blk_init_queue(md_request, &md.lock);
	if (mdqueue == NULL)
		goto out;

	/* set I/O scheduler */
	if (mdqueue->elevator) {
		/* 
		 * blk_init_queue will set its orignal elevator:
		 * elevator_init(q, NULL)
		 */
		printk(KERN_ALERT "old elevator:%s\n", 
			mdqueue->elevator->elevator_type->elevator_name);
		/* remove orignal elevator */
		elevator_exit(mdqueue->elevator);
		/* 
		 * Setting it to NULL is important, otherwise
		 * next elevator_init return 0(ok) ,but it don't init, 
		 * in which case kernel will report bug!
		 */
		mdqueue->elevator = NULL;
	}
	/*
	 * see I/O scheduler for our block device:
	 * cat /sys/block/memd0/queue/scheduler 
	 */
	ret = elevator_init(mdqueue, "noop");
	printk(KERN_ALERT "  + elevator_init return %d\nnew elevator:%s\n", 
		ret, mdqueue->elevator->elevator_type->elevator_name);

	blk_queue_logical_block_size(mdqueue, blocksize);

	/* register block device */	
	printk(KERN_ALERT "register blk\n");
	major_num = register_blkdev(major_num, "memdisk");
	if (major_num <= 0) {
		printk(KERN_ALERT "md:unable to get major number\n");
		goto out;
	}

	/* setup generic disk */
	printk(KERN_ALERT "setup generic disk\n");
	md.gd = alloc_disk(16);	/* device will suport 15 partitions */
	if (md.gd == NULL)
		goto out_unregister;
	md.gd->major = major_num;
	md.gd->first_minor = 0;
	md.gd->fops = &md_ops;
	md.gd->private_data = &md;
	strcpy(md.gd->disk_name, "memd0");
	set_capacity(md.gd, blocks * (blocksize / KERNEL_SECTOR_SIZE));
	md.gd->queue = mdqueue;

	/* add disk when it can handle requests*/
	printk(KERN_ALERT "add disk\n");
	add_disk(md.gd);

	printk(KERN_ALERT "memory disk init ok\n");
	return 0;

out_unregister:
	unregister_blkdev(major_num, "memdisk");
out:
	vfree(md.data);
	return -ENOMEM;
}

static void __exit md_exit(void)
{
	del_gendisk(md.gd);
	put_disk(md.gd);
	unregister_blkdev(major_num, "memdisk");
	blk_cleanup_queue(mdqueue);
	vfree(md.data);
	printk(KERN_ALERT "goodbye kernel\n");
}

MODULE_LICENSE("GPL");

module_init(md_init);
module_exit(md_exit);
