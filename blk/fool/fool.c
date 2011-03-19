#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <asm/segment.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/bio.h>

#include "fool.h"

#define dbg(fmt, arg...) printk(KERN_ALERT "%s:"fmt"\n", __FUNCTION__, ##arg)


#define FOOL_MAJOR 250

static struct fool_device fo;

/* blk-core.c bio_check_eod is not exported */
static inline int bio_check_eod(struct bio *bio, unsigned int nr_sectors)
{
	sector_t max;
	if (!nr_sectors)
		return 0;
	max = bio->bi_bdev->bd_inode->i_size >> 9;
	if (max) {
		sector_t sector = bio->bi_sector;
		if (max < nr_sectors || max - nr_sectors < sector)
			return 1;
	}
	return 0;
}

static void fool_add_list(struct fool_device *fo, struct bio *bio)
{
	bio_list_add(&fo->bio_list, bio);
}

static struct bio *fool_get_bio(struct fool_device *fo)
{
	return bio_list_pop(&fo->bio_list);
}

static int file_rw(struct file *file, void *iomem,
				int len, loff_t pos, int write)
{
	int size;
	mm_segment_t old_fs = get_fs();
	set_fs(get_ds());

	if (write)
		size = file->f_op->write(file, iomem, len, &pos);
	else
		size = file->f_op->read(file, iomem, len, &pos);
	set_fs(old_fs);		
/*	if (write)
		cond_resched();
		*/
	return size;
}

static int do_bio_file(struct fool_device *fo, struct bio *bio)
{
	loff_t pos;
	int n, i, w, ret;
	struct file *file;
	struct bio_vec *bv;
	void *iomem;

	ret = 0;
	pos = ((loff_t)bio->bi_sector << 9);
	file = fo->bound_file;
	w = (bio_rw(bio) == WRITE) ? 1 : 0;
	
	bio_for_each_segment(bv, bio, i) {
		iomem = kmap(bv->bv_page);
		if (!iomem) {
			dbg("kmap err");
			ret = -EIO;
			goto end;
		}
		iomem += bv->bv_offset;
		n = file_rw(file, iomem, bv->bv_len, pos, w);
		if (n != bv->bv_len) {
			dbg("rw err: pos=%llu, %s", pos, w ? "write":"read");
			ret = -EIO;
			kunmap(bv->bv_page);
			goto end;
		}
		pos += bv->bv_len;
		kunmap(bv->bv_page);
	}

end:
	return ret;
}

static void fool_handle_bio(struct fool_device *fo, struct bio *bio)
{
	int ret;
	ret = do_bio_file(fo, bio);
	bio_endio(bio, ret);
}

static int fool_thread(void *data)
{
	struct fool_device *fo = data;
	struct bio *bio;

	while (!kthread_should_stop() || !bio_list_empty(&fo->bio_list)) {

		wait_event_interruptible(fo->wait, 
			!bio_list_empty(&fo->bio_list) ||
			kthread_should_stop());
		
		if (bio_list_empty(&fo->bio_list))
			continue;
	
		spin_lock_irq(&fo->lock);
		bio = fool_get_bio(fo);
		spin_unlock_irq(&fo->lock);
		fool_handle_bio(fo, bio);
	}

	return 0;
}

static int fool_make_request(struct request_queue *q, struct bio *bio)
{
	struct fool_device *fo = q->queuedata;
	int rw = bio_rw(bio);

	if (rw == READA)
		rw = READ;
	if (!fo || (rw != READ && rw != WRITE) ||
		bio_check_eod(bio, bio_sectors(bio)))
		goto out;
	

	/* lock fool device */
	spin_lock_irq(&fo->lock);
	if (fo->state != FOOL_BOUND) {
		spin_unlock_irq(&fo->lock);
		goto out;
	}
	if (rw == WRITE && (fo->flags & FOOL_FLAGS_READ_ONLY)) {
		spin_unlock_irq(&fo->lock);
		goto out;
	}
	fool_add_list(fo, bio);
	/* real bio handle is left to fool kthread */
	wake_up(&fo->wait);
	spin_unlock_irq(&fo->lock);
	return 0;

out:
	bio_io_error(bio);
	return 0;
}

static loff_t get_fool_size(struct fool_device *fo, struct file *file)
{
	loff_t size;
	size = i_size_read(file->f_mapping->host);
	/* the extra size in [0-511] is dropped!! */
	return size >> 9;	
}

static int fool_clr_fd(struct fool_device *fo, struct block_device *bdev)
{
	struct file *file = fo->bound_file;
	int ret;

	ret = -ENXIO;
	if (fo->state != FOOL_BOUND)
		goto out;

	ret = -EINVAL;
	if (!file)
		goto out;

	kthread_stop(fo->thread);
	fo->bound_file = NULL;
	fo->blkdev = NULL;
	fo->flags = 0;
	fo->thread = NULL;
	if (bdev)
		invalidate_bdev(bdev);
	set_capacity(fo->gd, 0);
	if (bdev) {
		bd_set_size(bdev, 0);
		kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);
	}
	/* ignore fool block size */

	fput(file);
	fo->state = FOOL_UNBOUND;
	ret = 0;
out:
	return ret;

}

static int is_fool_device(struct file *file)
{
	struct inode *i = file->f_mapping->host;
	return i && S_ISBLK(i->i_mode) && MAJOR(i->i_rdev) == FOOL_MAJOR;
}

static int fool_set_fd(struct fool_device *fo, fmode_t mode,
		struct block_device *bdev, unsigned int fd)
{
	loff_t sectors;
	struct file *file;
	struct inode *inode;
	int ret;

	ret = -EBUSY;
	if (fo->state != FOOL_UNBOUND)
		goto out;

	ret = -EBADF;
	file = fget(fd);
	if (!file)
		goto out;

	ret = -EINVAL;
	/* donnot bind fool file */
	if (is_fool_device(file))
		goto out_put_file;

	inode = file->f_mapping->host;
	/* only bind block and regular file */
	if (!S_ISREG(inode->i_mode) && !S_ISBLK(inode->i_mode))
		goto out_put_file;

	if (!(file->f_mode & FMODE_WRITE) || !(mode & FMODE_WRITE))
		fo->flags |= FOOL_FLAGS_READ_ONLY;

	/* get size of bound file */
	sectors = get_fool_size(fo, file);
	/* maybe loff_t overflows sector_t */
	if ((loff_t)(sector_t)sectors != sectors) {
		ret = -EFBIG;
		goto out_put_file;
	}

	set_device_ro(bdev, (fo->flags & FOOL_FLAGS_READ_ONLY) != 0);
	fo->blkdev = bdev;
	fo->bound_file = file;
	bio_list_init(&fo->bio_list);
	blk_queue_make_request(fo->q, fool_make_request);
	fo->q->queuedata = fo;

	/* set size */
	fo->blocksize = S_ISBLK(inode->i_mode) ?
			inode->i_bdev->bd_block_size : PAGE_SIZE;
	set_capacity(fo->gd, sectors);
	bd_set_size(bdev, sectors << 9);
	kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);
	set_blocksize(bdev, fo->blocksize);
	
	fo->thread = kthread_create(fool_thread, fo, "kfool");
	
	if (IS_ERR(fo->thread)) {
		ret = PTR_ERR(fo->thread);
		goto out_clear;
	}

	fo->state = FOOL_BOUND;
	/* let kloop run */
	wake_up_process(fo->thread);

	return 0;

out_clear:
	fo->thread = NULL;
	fo->blkdev = NULL;
	fo->bound_file = NULL;
	fo->flags = 0;	
	set_capacity(fo->gd, 0);
	invalidate_bdev(bdev);
	bd_set_size(bdev, 0);
	kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);
	fo->state = FOOL_UNBOUND;

out_put_file:
	fput(file);

out:
	return ret;	
}

static int fool_ioctl(struct block_device *bdev, fmode_t mode,
		unsigned int cmd, unsigned long arg)
{
	struct fool_device *fo = bdev->bd_disk->private_data;
	int err;

	BUG_ON(!fo);
	
	switch (cmd) {
	case FOOL_SET_FD:
		err = fool_set_fd(fo, mode, bdev, arg);
		break;
	case FOOL_CLR_FD:
		err = fool_clr_fd(fo, bdev);
		break;
	/* just ignore it */
	case FOOL_SET_STATUS:
		dbg("ignore FOOL_SET_STATUS");
		err = 0;
		break;
	case FOOL_SET_STATUS64:
		dbg("ignore FOOL_SET_STATUS64");
		err = 0;
		break;
	default:
		err = -EINVAL;
		break;
	}
	return err;
}

static struct block_device_operations fool_fops = {
	.owner = THIS_MODULE,
	.ioctl = fool_ioctl,
};

/* init fool_device internal parameters */
static void init_fool_device(struct fool_device *fo)
{
	fo->blkdev = NULL;
	fo->state = FOOL_UNBOUND;
	fo->flags = 0;
	fo->thread = NULL;
	fo->bound_file = NULL;
	init_waitqueue_head(&fo->wait);
}

static int __init fool_init(void)
{
	int ret;
	memset(&fo, 0x0, sizeof(fo));

	ret = -EIO;
	if (register_blkdev(FOOL_MAJOR, "FOOL"))
		goto out;


	ret = -ENOMEM;
	fo.q = blk_alloc_queue(GFP_KERNEL);
	if (!fo.q)
		goto out;
	
	fo.gd = alloc_disk(16);
	if (!fo.gd)

		goto out_free_queue;
	
	/* init disk and fo */
	init_fool_device(&fo);
	spin_lock_init(&fo.lock);	
	fo.gd->major = FOOL_MAJOR;	
	fo.gd->first_minor = 0;
	fo.gd->fops = &fool_fops;
	fo.gd->queue = fo.q;
	fo.gd->private_data = &fo;
	sprintf(fo.gd->disk_name, "fool");

	/* add disk */
	add_disk(fo.gd);
	printk(KERN_ALERT "fool init\n");
	return 0;

out_free_queue:
	blk_cleanup_queue(fo.q);	
out:
	return ret;
}

static void __exit fool_exit(void)
{
	if (fo.state == FOOL_BOUND) {
		dbg("unclear fool bound file: clear it aotumatically");
		fool_clr_fd(&fo, NULL);
		
	}

	del_gendisk(fo.gd);
	put_disk(fo.gd);

	unregister_blkdev(FOOL_MAJOR, "FOOL");

	blk_cleanup_queue(fo.q);
	printk(KERN_ALERT "fool exit\n");
}

MODULE_LICENSE("GPL");

module_init(fool_init);
module_exit(fool_exit);
