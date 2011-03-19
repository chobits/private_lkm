/*
 * a simple block device based on local memory
 *
 * dynamic alloc memory to disk
 *   alloc memory to disk when first WRITE
 *   return random data(here, we use zero data) to first READ operation
 *
 * + add mutex to prevent races
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock_types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>	/* blk_queue_logical_block_size */
#include <linux/hdreg.h>	/* hd_geometry */
#include <linux/bio.h>		/* bio_for_each_segment */
#include <linux/radix-tree.h>

#define SECTOR_SHIFT 9
#define SECTOR_SIZE (1 << SECTOR_SHIFT)
#define SEGMENT_ORDER 2
#define SEGMENT_SHIFT (SEGMENT_ORDER + PAGE_SHIFT)
#define SEGMENT_SIZE (1 << SEGMENT_SHIFT)
#define SEGMENT_MASK (~(SEGMENT_SIZE - 1))


static struct radix_tree_root __diskmem_root;
static DEFINE_MUTEX(root_mutex); /* protect the __diskmem_root */

static struct md_device {
	unsigned long long size;
	spinlock_t lock;
	struct radix_tree_root *data;
	struct gendisk *gd;
} md;

static struct request_queue *mdqueue;
static unsigned int blocksize;
static unsigned long long blocks;
static char *disk_size_str = "16M";
static int major_num;

#define MD_MAX_SIZE (blocksize * blocks)
#define MD_NAME "memdisk"

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
	sectors = md->size >> SECTOR_SHIFT;
	hdgeo->cylinders = sectors / 4 / 16;
	hdgeo->heads = 4;
	hdgeo->sectors = 16;
	return 0;
}

struct block_device_operations md_ops = {
	.owner = THIS_MODULE,
	.getgeo = md_getgeo,
};

static int blkdev_trans_seg(void *iomem, unsigned long long diskaddr,
						unsigned int size, int write)
{
	unsigned int count, offset, len, segaddr;
	struct page *page;
	void *diskmem;

	count = 0;
	segaddr = diskaddr & ~SEGMENT_MASK;

	mutex_lock(&root_mutex);
	page = radix_tree_lookup(&__diskmem_root, diskaddr >> SEGMENT_SHIFT);
	/* first read: copy zero data to iomem */
	if (!page && !write) {		
		count = size;
		memset(iomem, 0x0, count);
		goto out;
	}
	/* first write: alloc zero pages */
	else if (!page) {
		page = alloc_pages(GFP_KERNEL | __GFP_ZERO | __GFP_HIGHMEM,
								SEGMENT_ORDER);
		if (!page) {
			printk(KERN_ALERT "cannot alloc pages\n");
			goto out;
		}
		if (radix_tree_insert(&__diskmem_root,
			diskaddr >> SEGMENT_SHIFT, (void *)page) != 0) {
			printk(KERN_ALERT "radix tree insert err\n");
			goto out;	
		}
	}

	page += (segaddr >> PAGE_SHIFT);
	/* other situations */
	while (count < size) {
		diskmem = kmap(page);
		if (!diskmem) {
			printk(KERN_ALERT "kmap err\n");
			goto out;
		}

		offset = (segaddr + count) & ~PAGE_MASK;
		len = min(size - count, (unsigned int)PAGE_SIZE - offset);

		/* handle real I/O */
		if (write)
			memcpy(diskmem + offset, iomem, len);
		else
			memcpy(iomem, diskmem + offset, len);

		count += len;
		iomem += len;

		kunmap(page);
		page++;
	}

out:
	mutex_unlock(&root_mutex);
	return count;
}

static int blkdev_trans(void *iomem, unsigned long long diskaddr,
						unsigned int size, int rw)
{
	unsigned int count, offset, len;

	count = 0;

	if (rw == WRITE)
		rw = 1;
	else if (rw == READ || rw == READA)
		rw = 0;
	else {
		printk(KERN_ALERT "unknown bio rw value\n");
		goto out;
	}

	while (count < size) {
		offset = (diskaddr + count) & ~SEGMENT_MASK;
		len = min(size - count, (unsigned int)SEGMENT_SIZE - offset);

		/* blkdev_trans_seg will handle NULL page */
		if (blkdev_trans_seg(iomem, diskaddr + count, len, rw)
								!= len) {
			printk(KERN_ALERT "bllkdev_trans_seg err\n");
			goto out;
		}

		count += len;
		iomem += len;
	}
out:
	return count;
}

/*
 * handle I/O operations via bio, not using request_queue
 */
static int fool_make_request(struct request_queue *q, struct bio *bio)
{
	struct bio_vec *bv;
	unsigned long long disk_addr;
	int i, error, n;
	void *iovec_mem;


	error = 0;
	if ((bio->bi_sector << 9) + bio->bi_size > MD_MAX_SIZE) {
		printk(KERN_ERR MD_NAME ":bas request: block=%llu, count=%u\n",
			(unsigned long long)bio->bi_sector ,bio->bi_size);	
		error = -EIO;
		goto end;
	}

	disk_addr = bio->bi_sector << 9;
	bio_for_each_segment(bv, bio, i) {
		iovec_mem = kmap(bv->bv_page);
		if (!iovec_mem) {
			error = -EIO;	
			printk(KERN_ALERT "kmap iovec_mem error\n");
			goto end;
		}
		iovec_mem += bv->bv_offset;

		n = blkdev_trans(iovec_mem, disk_addr, bv->bv_len, bio_rw(bio));	
		if (n != bv->bv_len) {
			kunmap(bv->bv_page);
			error = -EIO;	
			printk(KERN_ALERT "blkdev_trans error\n");
			goto end;
		}

		kunmap(bv->bv_page);
		disk_addr += bv->bv_len;
	}

end:
	bio_endio(bio, error);
	return 0;
}

static void free_diskmem(unsigned long long size) 
{
	struct radix_tree_root *root;
	struct page *page;
	unsigned long long segs, i;

	/* init radix tree root*/
	root = &__diskmem_root;
	segs = (size + SEGMENT_SIZE - 1) / SEGMENT_SIZE;

	for (i = 0; i < segs; i++) {
		page = radix_tree_lookup(root, i);
		if (page)
			__free_pages(page, SEGMENT_ORDER);
		radix_tree_delete(root, i);
	}
}

static unsigned long long get_blocks(char *str)
{
	char unit;
	unsigned long long value;
	if (sscanf(str, "%llu%c", &value, &unit) != 2) {
		printk(KERN_ALERT "cannot analyze 'size' param\n");	
		return 0;	
	}
	switch (unit) {
		case 'k':
		case 'K':
			value <<= (10 - SECTOR_SHIFT);
			break;

		case 'm':
		case 'M':
			value <<= (20 - SECTOR_SHIFT);
			break;

		case 'g':
		case 'G':
			value <<= (30 - SECTOR_SHIFT);
			break;
		default:
			printk(KERN_ALERT "size donnot support unit:%c\n", unit);
			value = 0;
			break;
	}

	return (unsigned long long)value;
}

static int __init md_init(void)
{
	/* init parameters */
	blocksize = 512;
	blocks = get_blocks(disk_size_str);	/* default: disk size is 16MB */
	if (blocks == 0)
		goto out;
	md.size = blocks * blocksize;

	/* init radix tree root*/
	INIT_RADIX_TREE(&__diskmem_root, GFP_KERNEL);
	md.data = &__diskmem_root;

	major_num = 0;		/* dynamic alloc device major number */

	/* init memory disk device */
	printk(KERN_ALERT "init md\n");	
	spin_lock_init(&md.lock);

	/* init request queue */
	printk(KERN_ALERT "init request queue\n");	
	mdqueue = blk_alloc_queue(GFP_KERNEL);
	if (mdqueue == NULL)
		goto out;
	mdqueue->node = -1;
	/* 
	 * use our own q->make_request_fn 
	 * normally: bio->request->elevator
	 * here we skip request and elevator layer and handle I/O in our own
	 * fool_make_request func directly.
	 */
	blk_queue_make_request(mdqueue, fool_make_request);
	/*
	 * this two funcs must be called after blk_queue_make_request
	 * because blk_queue_make_request will call them defaultly
	 * set hw/soft max limit sectors
	 */
	blk_queue_max_hw_sectors(mdqueue, 1024);
	/* no need bounce, BLK_BOUNCE_HIGH is also OK */
	blk_queue_bounce_limit(mdqueue, BLK_BOUNCE_ANY);

	blk_queue_logical_block_size(mdqueue, blocksize);
	

	/*
	 * register block device 
	 * see major name: grep -w MD_NAME /proc/devices
	 */
	printk(KERN_ALERT "register blk\n");
	major_num = register_blkdev(major_num, MD_NAME);
	if (major_num <= 0) {
		printk(KERN_ALERT "md:unable to get major number\n");
		goto out_cleanup_queue;
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
	/* block device total size is determined */
	set_capacity(md.gd, blocks * (blocksize >> SECTOR_SHIFT));
	md.gd->queue = mdqueue;

	/* add disk when it can handle requests*/
	printk(KERN_ALERT "add disk\n");
	/* gendisk->queue != NULL */
	add_disk(md.gd);

	printk(KERN_ALERT "memory disk init ok\n");
	return 0;

out_unregister:
	unregister_blkdev(major_num, MD_NAME);
out_cleanup_queue:
	blk_cleanup_queue(mdqueue);
out:
	return -ENOMEM;
}

static void __exit md_exit(void)
{
	del_gendisk(md.gd);
	put_disk(md.gd);
	unregister_blkdev(major_num, MD_NAME);
	blk_cleanup_queue(mdqueue);

	/* free alloced disk memory */
	free_diskmem(md.size);
	printk(KERN_ALERT "goodbye kernel\n");
}

MODULE_LICENSE("GPL");

/* insmod size=?? ==> disk_size=??*/
module_param_named(size, disk_size_str, charp, S_IRUGO);

module_init(md_init);
module_exit(md_exit);
