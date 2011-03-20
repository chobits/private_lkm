#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

void perx(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

int main()
{
	/*
	 * off comes from:
	 *   # insmod k2u.ko
	 *   # dmesg | tail
	 */
	off_t off = 0x32c46000;
	char buf[512];
	char *p;
	int fd;
	
	fd = open("/dev/mem", O_RDWR);
	if (fd == -1)
		perx("open"); 
	p = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off);
	if (!p)
		perx("mmap");
	/*
	 * if CONFIG_STRICT_DEVMEM=y (char/mem.c-->range_is_allowd())
	 * some addr cannot be read or written by us!!!!!
	 */
	printf("kernel:%s", p);
	strcpy(p, "hello k2u!\n");

	munmap(p, 4096);
	close(fd);
	return 0;	
}
