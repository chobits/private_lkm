#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define PAGE_SIZE 4096

void per(char *str)
{
	perror(str);
	exit(1);
}

int main(void)
{
	int fd;
	unsigned char buf[11];
	unsigned int p =0x1ccb8000;

	buf[10] = 0;
	fd = open("/dev/mem", O_RDWR);
	if (fd == -1) 
		per("open");
	if (p != lseek(fd, p, SEEK_SET))
		per("lseek");
	if (read(fd, buf, 10) < 0)
		per("read");
	printf("%x\n", (unsigned int)(0xff & buf[0]));
	printf("%x\n", (unsigned int)(0xff & buf[1]));
	return 0;
}

