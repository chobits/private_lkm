#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

static struct kmem_cache *cachep;
struct x {
	int a;
	int b;
};
int __init hello_init(void)
{
	struct x *xp;
	cachep = kmem_cache_create("KKKKK", sizeof(struct x), 0, 0, NULL);
	if (cachep == NULL) {
		printk(KERN_ALERT "create err\n");
		return -1;
	}

	xp = kmem_cache_alloc(cachep, 0);
	if (xp == NULL) {
		printk(KERN_ALERT "create err\n");
		return -1;
	}
	xp->a = 1;
	xp->b = 2;
	printk(KERN_ALERT "%d %d\n", xp->a, xp->b);
	kmem_cache_free(cachep, xp);
	printk(KERN_ALERT "init ok\n");
	return 0;
}

void __exit hello_exit(void)
{
	kmem_cache_destroy(cachep);
	printk(KERN_ALERT "Goodbye kernel\n");
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

