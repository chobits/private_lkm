#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
int main()
{
	int fd;
	int n,i;
	unsigned char buf[128];
	fd = open("cmod_dev", O_RDONLY);
	if (fd == -1) {
		perror("open");
		return 1;
	}
	n = read(fd, buf, 6);
	if (n == -1) {
		perror("read");
		close(fd);
		return 1;
	}
	printf("read count:%d\n", n);
	for (i = 0; i < 6; i++)
		printf("%d:%d ", i, buf[i]);
	printf("\n");
	close(fd);
	return 0;
}
