#include <linux/vfs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include "simp_fs.h"

/*
 * find a page, lock it, clear it
 */
int simp_write_begin(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned flags, struct page **pagep,
		void **fsdata)
{
	struct page *page;
	pgoff_t index;
	dfunc();

	/* pos to page nr */
	index = pos >> PAGE_CACHE_SHIFT;
	/* find(locked) or alloc page via index in radix tree */
	page = grab_cache_page_write_begin(mapping, index, flags);
	if (!page)
		return -ENOMEM;
	*pagep = page;

	/* not uptodate(clear page)? we do it own */
	if (!PageUptodate(page) && (len != PAGE_CACHE_SIZE)) {
		/* offset in page */
		unsigned from = pos & (PAGE_CACHE_SIZE - 1);
		/* 
		 * zero 0 to from
		 * zero from+len to 4KB 
		 */
		zero_user_segments(page, 0, from, from + len, PAGE_CACHE_SIZE);
	}
	return 0;
}

/*
 * clear page, unlock it, release it
 */
int simp_write_end(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned copied, struct page *page,
		void *fsdata)
{
	struct inode *inode = page->mapping->host;
	loff_t last_pos = pos + copied;
	dfunc();

	if (copied < len) {
		unsigned from = pos & (PAGE_CACHE_SIZE - 1);
		/*
		 * zero start:from+copied length:len-copied
		 * zero from+copied to from+len
		 */
		zero_user(page, from + copied, len - copied);
	}	
	/* after zero it we set Uptodate */
	if (!PageUptodate(page))
		SetPageUptodate(page);
	/* adjust inode->i_size(file size) */
	if (last_pos > inode->i_size)
		i_size_write(inode, last_pos);
	/* waiting to flush */	
	set_page_dirty(page);
	/* unlock the lock which is locked in write_begin */
	unlock_page(page);
	/* release it which is get from write_begin */
	page_cache_release(page);
	return copied;
}

/* clear page to prepare reading userbuf to page */
int simp_readpage(struct file *file, struct page *page)
{
	dfunc();
	/*
	 * kaddr = kmap_atomic(page) 
	 * memset(kaddr, 0x0, PAGE_SIZE)
	 */
	clear_highpage(page);
	/* nil */
	flush_dcache_page(page);
	/* uptodate flag after reading */
	SetPageUptodate(page);
	unlock_page(page);
	return 0;
}

/* dirty page */
int simp_set_page_dirty(struct page *page)
{
	dfunc();
	if (!PageDirty(page))
		SetPageDirty(page);
	return 0;
}

/*
 * here is just simple task
 * all complex task is left to f_op read or write
 */
struct address_space_operations simp_aops = {
	.readpage = simp_readpage,
	.set_page_dirty = simp_set_page_dirty,
	.write_begin = simp_write_begin,
	/* actually copy from user to page in do_sync_write */
	.write_end = simp_write_end,
};
