#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/gfp.h>

static struct crypto_tfm *tfm;
static struct scatterlist sg;
static char *code1 = "hello world";

int do_digest(char *code, char **result)
{
	char *ret;
	int len = strlen(code); 
	tfm = crypto_alloc_tfm("sha1", 0);
	if (IS_ERR(tfm))
		return -1;
	sg_init_one(&sg, code, len);
	ret = (char *)kmalloc(50, GFP_KERNEL);
	if (!result) {
		crypto_free_tfm(tfm);
		return -1;
	}
	memset(ret, 0, 50);
	crypto_digest_final(tfm, result);
	crypto_free_tfm(tfm);
	*result = ret;
	return 0;
}


static int __init sha1_init(void)
{
	char *result;
	int i;
	int ret;
	ret = do_digest(code1, &result);
	if (result) {
		kfree(result);
		return -1;
	}
	for (i = 0; i < 50; i++)
		printk(KERN_ALERT "%2x ", (unsigned int)(*result++));

	kfree(result);
	return 0;
}

static void __exit sha1_exit(void)
{
	printk(KERN_ALERT "exit\n");
}


module_init(sha1_init);
module_exit(sha1_exit);
MODULE_LICENSE("GPL");

