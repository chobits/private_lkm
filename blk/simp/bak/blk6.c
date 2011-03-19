/*
 * a simple block device based on local memory
 * change alloc_diskmem & free_diskmem
 * sub traverses, using radix_tree_gang_lookup instead of radix_tree_lookup
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

#define KERNEL_SECTOR_SIZE 512
#define SECTOR_SHIFT 9
#define SEGMENT_ORDER 2
#define SEGMENT_SHIFT (SEGMENT_ORDER + PAGE_SHIFT)
#define SEGMENT_SIZE (1 << SEGMENT_SHIFT)
#define SEGMENT_MASK (~(SEGMENT_SIZE - 1))

static struct radix_tree_root __diskmem_root;

static struct md_device {
	unsigned long long size;
	spinlock_t lock;
	struct radix_tree_root *data;
	struct gendisk *gd;
} md;

static struct request_queue *mdqueue;
static unsigned int blocksize;
static unsigned int blocks;
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
	sectors = md->size >> 9;
	hdgeo->cylinders = sectors / 4 / 16;
	hdgeo->heads = 4;
	hdgeo->sectors = 16;
	return 0;
}

struct block_device_operations md_ops = {
	.owner = THIS_MODULE,
	.getgeo = md_getgeo,
};

void copy_disk_mem(void *iomem, void *diskmem, int len, int write)
{
	if (write) {
		printk(KERN_INFO MD_NAME":write %d\n", len);
		memcpy(diskmem, iomem, len);
	}
	else {
		printk(KERN_INFO MD_NAME":read %d\n", len);
		memcpy(iomem, diskmem, len);
	}
}

static void *get_disk_mem(unsigned int size, struct page **p)
{
	struct page *start_page;
	void *ret;
	ret = NULL;
	*p = NULL;
	start_page = radix_tree_lookup(&__diskmem_root, size / SEGMENT_SIZE);
	if (start_page) {
		*p = start_page + (size & ~SEGMENT_MASK) / PAGE_SIZE;
		ret = kmap(*p);
	}
	return ret;
}

static void put_disk_mem(struct page *page)
{
	kunmap(page);
}

static int blkdev_trans(void *iomem, unsigned int diskaddr, unsigned int size,
								int rw)
{
	unsigned int count, offset, len;
	void *diskmem;
	struct page *page;

	count = 0;

	if (rw == WRITE)
		rw = 1;
	else if (rw == READ || rw == READA)
		rw = 0;
	else
		goto out;

	while (count < size) {
		offset = (diskaddr + count) & ~PAGE_MASK;
		len = min(size - count, (unsigned int)PAGE_SIZE - offset);

		diskmem = get_disk_mem(diskaddr + count, &page);
		/* if get_disk_mem failed, we donot need to put_disk_mem */
		if (!diskmem) {
			goto out;
		}
		copy_disk_mem(iomem, diskmem + offset, len, rw);
		put_disk_mem(page);

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
	unsigned int disk_addr;
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

static void free_diskmem(unsigned int size) 
{
	struct radix_tree_root *root;
	struct page *page;
	struct page *pages[64];
	unsigned int segs;
	int i, k, n;

	/* init radix tree root*/
	root = &__diskmem_root;
	segs = (size + SEGMENT_SIZE - 1) / SEGMENT_SIZE;

	for (i = 0; i < segs; i += 64) {
		/* n traverses 
		   page = radix_tree_lookup(root, i);
		   */

		/* change n traverses to n/64 */
		n = radix_tree_gang_lookup(root, (void **)pages, i, 64);
		for (k = 0; k < n; k++) {
			page = pages[k];
			if (page) {
				__free_pages(page, SEGMENT_ORDER);
			}
			radix_tree_delete(root, i + k);
		}
	}
}

/*
   static void debug_page(struct page *page)
   {
   void *p;
   static int i = 0;

   p = kmap(page);
   ((unsigned int *)p)[0] = i++;	
   kunmap(page);
   }
   */

static struct radix_tree_root *alloc_diskmem(unsigned int size)
{
	struct radix_tree_root *root;
	struct page *page;
	unsigned int segs;	
	int i;

	/* init radix tree root*/
	root = &__diskmem_root;
	INIT_RADIX_TREE(root, GFP_KERNEL);

	/* alloc disk memory */
	segs = (size + SEGMENT_SIZE - 1) / SEGMENT_SIZE;
	for (i = 0; i < segs; i++) {
		page = alloc_pages(GFP_KERNEL | __GFP_ZERO | __GFP_HIGHMEM, SEGMENT_ORDER);
		if (!page)	{
			printk(KERN_ALERT "alloc pages err\n");
			goto err_gzhp;
		}
		if (radix_tree_insert(root, i, page) != 0) {
			printk(KERN_ALERT "radix tree insert err\n");
			goto err_insert;	

		}
		/* debug_page(page); */
	}
	return root;

err_insert:
	__free_pages(page, SEGMENT_ORDER);
	i++;
err_gzhp:
	if (i)
		free_diskmem(i * SEGMENT_SIZE);
	return NULL;
}

static unsigned int get_blocks(char *str)
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

	return (unsigned int)value;
}

static int __init md_init(void)
{
	/* init parameters */
	blocksize = 512;
	blocks = get_blocks(disk_size_str);	/* default: disk size is 16MB */
	if (blocks == 0)
		goto out;

	major_num = 0;		/* dynamic alloc device major number */

	/* init memory disk device */
	printk(KERN_ALERT "init md\n");	
	spin_lock_init(&md.lock);
	md.size = blocks * blocksize;
	/* alloc virtual disk memory */
	md.data = alloc_diskmem(md.size);
	if (md.data == NULL) {
		printk(KERN_ALERT MD_NAME": alloc disk mem err\n");
		goto out;
	}

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

	blk_queue_logical_block_size(mdqueue, blocksize);

	/* register block device */	
	printk(KERN_ALERT "register blk\n");
	major_num = register_blkdev(major_num, MD_NAME);
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
	return -ENOMEM;
}

static void __exit md_exit(void)
{
	del_gendisk(md.gd);
	put_disk(md.gd);
	unregister_blkdev(major_num, "memdisk");
	blk_cleanup_queue(mdqueue);
	free_diskmem(md.size);
	printk(KERN_ALERT "goodbye kernel\n");
}

MODULE_LICENSE("GPL");

/* insmod size=?? ==> disk_size=??*/
module_param_named(size, disk_size_str, charp, S_IRUGO);

module_init(md_init);
module_exit(md_exit);
