#include <linux/module.h>
#include <linux/kernel.h>
#include <asm-generic/sections.h>

#define pk(str,...) printk(KERN_ALERT str, __VA_ARGS__)

int __init hello_init(void)
{
	printk(KERN_ALERT "hello kernel\n");
	printk(KERN_ALERT "_text: 0x%p\n", _text);
	printk(KERN_ALERT "_stext: 0x%p\n", _stext);
	printk(KERN_ALERT "_etext: 0x%p\n", _etext);
	printk(KERN_ALERT "_data: 0x%p\n", _data);
	printk(KERN_ALERT "_sdata: 0x%p\n", _sdata);
	printk(KERN_ALERT "_edata: 0x%p\n", _edata);
	return 0;
}
void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye kernel\n");

}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

