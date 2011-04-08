#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	struct stat sbuf;
	if (argc != 2) {
		fprintf(stderr, "<usage> stat filename\n");
		exit(1);
	}	
	
	if (stat(argv[1], &sbuf) == -1) {
		perror("stat");
		exit(1);
	}
	
	printf("st_dev:%u\n", sbuf.st_dev);
	printf("st_ino:%u\n", sbuf.st_ino);
	printf("st_mode:%u\n", sbuf.st_mode);
	if (S_ISDIR(sbuf.st_mode))
		printf("\tis dir\n");
	else
		printf("\tnot dir\n");
	printf("st_uid:%u\n", sbuf.st_uid);
	printf("st_gid:%u\n", sbuf.st_gid);
	printf("st_size:%u\n", sbuf.st_size);
	printf("st_blksize:%u\n", sbuf.st_blksize);

	return 0;

}
