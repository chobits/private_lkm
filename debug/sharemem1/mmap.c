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
	char *buf;
	unsigned int p = 0xdccae000 - 0xc0000000;

	fd = open("/dev/mem", O_RDWR);
	if (fd == -1) 
		per("open");

	buf = mmap(0, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, p);
	if (buf == MAP_FAILED)
		per("mmap");
	puts(buf);
	munmap(buf, PAGE_SIZE);	
	close(fd);
	return 0;
}

