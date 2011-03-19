#include <linux/init.h>
#include <linux/module.h>

int returnzero(void)
{
	return 0;
}
EXPORT_SYMBOL(returnzero);

MODULE_LICENSE("GPL");
