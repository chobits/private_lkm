#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/sched.h>
MODULE_LICENSE("Dual BSD/GPL");

#define _DEBUG
#ifdef _DEBUG
#define kprintk(fmt,args...) printk(KERN_DEBUG fmt,##args)
#define kprintf(fmt,args...) printf(fmt,##args)
#define kperror(str) perror(str)
#else
#define kprintk
#define kprintf
#define kperror
#endif

struct _idtr{
	unsigned short  limit;
	unsigned int    base;
}; __attribute__ ( ( packed ) );

// 中断描述符表结构
struct _idt_descriptor
{
	unsigned short offset_low;
	unsigned short sel;
	unsigned char  none, flags;
	unsigned short offset_high;
} __attribute__((packed));


unsigned int get_sys_call_table(void)
{

	struct _idt_descriptor * idt;
	struct _idtr idtr;
	unsigned int sys_call_off;
	int sys_call_table=0;
	unsigned char* p;
	int i;

	asm("sidt %0":"=m"(idtr));
	printk("addr of idtr: 0x%x\n", (unsigned int)&idtr);
	idt=(struct _idt_descriptor *)(idtr.base+8*0x80);
	sys_call_off=((unsigned int )(idt->offset_high<<16)|(unsigned int )idt->offset_low);
	printk("addr of idt 0x80: 0x%x\n", sys_call_off);
	p=(unsigned char *)sys_call_off;
	for (i=0; i<100; i++)
	{
		if (p[i]==0xff && p[i+1]==0x14 && p[i+2]==0x85)
		{
			sys_call_table=*(int*)((int)p+i+3);
			kprintk("addr of sys_call_table: 0x%x\n", sys_call_table);
			return sys_call_table;
		}
	}
	return 0;
}

int raider_init(void)
{
	kprintk("raider init\n");
	get_sys_call_table();
	return  0;
}

void raider_exit(void)
{
	kprintk("raider exit");
}


module_init(raider_init);
module_exit(raider_exit);
