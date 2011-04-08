#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main(int argc,char **argv)
{
	int i;
	int x;
	int y;
	int fd;
	char buf[16];

	if (argc != 2) {
		printf("usage\n");
		return -1;
	}

	fd = open(argv[1], O_WRONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	for (i = 0; i < 30; i++) {
		x = random() % 20;
		y = random() % 20;
		if (x % 2 == 0)
			x = -x;
		if (y % 2 == 0)
			y = -y;
		sprintf(buf, "%d %d 0", x, y);
		printf("%d: x:%d y:%d\n", i, x, y);
		write(fd, buf, strlen(buf));
		fsync(fd);
		sleep(1);
	}
}
