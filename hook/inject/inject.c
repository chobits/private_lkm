#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#define CMPSTR "vermagic="
//#define OLD "2.6.32.12-115.fc12.i686.PAE SMP mod_unload 686 "
//#define NEW "2.6.32.12-115.fc12.i686 SMP mod_unload 686 "
//#define OLD "2.6.32-25-generic SMP mod_unload modversions 586"
#define NEW "2.6.32.16-141.fc12.i686 SMP mod_unload 686 "

int main(int argc, char **argv)
{
	int fd, n, i;
	char buf[1024];
	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		perror("open");
		return -1;
	}
	while ((n = read(fd, buf ,1024)) > 0) {
		for (i = 0; i < n; i++) {
			if (buf[i] == 'v' 
				&& strncmp(buf + i, CMPSTR, 9) == 0) {
				strcpy(buf + i + 9, NEW);
				
				if (lseek(fd, -n, SEEK_CUR) == -1)
					perror("lseek"); 	

				if (write(fd, buf, n) != n)
					perror("write");
				printf("ok\n");
				return 0;
			}
		}
	}
	return -1;	
}
