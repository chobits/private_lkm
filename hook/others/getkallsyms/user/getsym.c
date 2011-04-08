#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void per(char *str)
{
	if (str)
		perror(str);
	else
		perror("");
	exit(1);
}

void usage()
{
	fprintf(stderr, "usage: getsym symbolname\n");
}

#define BUFS 512
int main(int argc, char **argv)
{
	char *symname, *find;
	char buf[BUFS];
	char tmp;
	int i, j, k;
	int fd, n, ok, seek, symlen;
	if (argc != 2)
		usage();
	symname = argv[1];
	symlen = strlen(symname);
	ok = seek = 0;
	fd = open("/proc/kallsyms", O_RDONLY);
	if (fd == -1)
		per("open");
i = j = k = 0;
	while ((n = read(fd, buf, BUFS)) > 0) {
		j += n;
		if (((find = strstr(buf, symname)) != NULL) &&
							find[-1] == ' ') {
			if (find + symlen <= buf + n) {
				if (find[symlen] == '\n')
					ok = 1;
			}
			else {
				if ((n = read(fd, &tmp, 1)) == 1) {
					if (tmp == '\n')
						ok = 1;
					//else
					//	lseek(fd, -1, SEEK_CUR);
				}
				else		
					break;
			}

			if (ok == 1)
				break;
			find = NULL;
		}
		while (n - 1 - seek > 0 && buf[n - 1 - seek] != '\n') 
			seek++;

//		printf("%d\n", seek);
		if (seek) {
//			lseek(fd, -seek, SEEK_CUR);
			i += seek;
			k++;
			seek = 0;
		}
	}
	printf("seek times:%d-seek:%d-read:%d\n", k, i, j);
	if (n == -1)
		per("read");
	if (find != NULL) {
		find = find - 11;	
		find[8] = 0;
		printf("%s: 0x%s\n", symname, find);
	}
	else
		printf("cannot find %s\n", symname);
	close(fd);
	return 0;	

}
