#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <stdlib.h>

struct linux_dirent {
	unsigned long d_ino;
	off_t d_off;
	unsigned short d_reclen;
	char d_name[];
};

int main()
{
	int fd, ret;
	char dir[1024];
	struct linux_dirent *p;

	fd = open("/simp", O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	ret = syscall(SYS_getdents, fd, &dir, sizeof(dir));
	if (ret == -1) {
		perror("getdents");
		return -1;
	}
	p = (struct linux_dirent *)dir;
	printf("%s %lu\n", p->d_name, p->d_ino);
	p = (struct linux_dirent *)(dir + p->d_reclen);
	printf("%s\n", p->d_name, p->d_ino);
	return 0;
}
