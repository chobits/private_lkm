#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
int main()
{
	int fd;
	char c;
	fd = open("/proc/kk", O_RDWR);
	write(fd, "a", 1);
	sleep(2);
	read(fd, &c, 1);
	printf("[user___%c___]", c);
	
	close(fd);
	
	return 0;
}
